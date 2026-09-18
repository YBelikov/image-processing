#include "JpegDriver.hpp"
#include "JpegBridge.h"
#include "../ExifOrientation.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace imp_io::detail {
namespace {

constexpr std::array<std::uint8_t, 2> jpegSignature{0xff, 0xd8};
constexpr std::size_t iccTagCountOffset = 128;
constexpr std::size_t iccTagTableStart = 132;
constexpr std::size_t iccTagEntrySize = 12;
constexpr std::size_t iccLengthOffset = 0;
constexpr std::size_t iccColorSpaceOffset = 16;
constexpr std::size_t iccSignatureOffset = 36;
constexpr std::uint32_t rgbColorSpace = 0x52474220;
constexpr std::uint32_t acspSignature = 0x61637370;

CodecError bridgeError(const ImpJpegError& error) {
    CodecErrorCode code = CodecErrorCode::DecodeFailed;
    switch (error.code) {
        case IMP_JPEG_INVALID: code = CodecErrorCode::InvalidData; break;
        case IMP_JPEG_METADATA: code = CodecErrorCode::IncompatibleMetadata; break;
        case IMP_JPEG_UNSUPPORTED_PRECISION: code = CodecErrorCode::UnsupportedPrecision; break;
        case IMP_JPEG_UNSUPPORTED_LAYOUT: code = CodecErrorCode::UnsupportedLayout; break;
        case IMP_JPEG_UNSUPPORTED_FEATURE: code = CodecErrorCode::UnsupportedFeature; break;
        case IMP_JPEG_RESOURCE: code = CodecErrorCode::ResourceLimit; break;
        case IMP_JPEG_ENCODE: code = CodecErrorCode::EncodeFailed; break;
        case IMP_JPEG_OK: break;
    }
    return {code, error.message};
}

std::uint32_t readBig32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    constexpr unsigned bitsPerByte = 8;
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < sizeof(value); ++index) {
        value = (value << bitsPerByte) | bytes[offset + index];
    }
    return value;
}

std::optional<CodecError> validateIcc(const ImageMetadata& metadata) {
    if (!metadata.iccProfile) {
        return std::nullopt;
    }

    const auto& profile = *metadata.iccProfile;
    if (profile.size() > IMP_JPEG_METADATA_LIMIT) {
        return CodecError{CodecErrorCode::ResourceLimit,
                          "JPEG ICC profile exceeds APP2 sequence capacity"};
    }
    if (profile.size() < iccTagTableStart) {
        return CodecError{CodecErrorCode::IncompatibleMetadata, "JPEG ICC profile is too short"};
    }
    if (readBig32(profile, iccLengthOffset) != profile.size()
        || readBig32(profile, iccSignatureOffset) != acspSignature
        || readBig32(profile, iccColorSpaceOffset) != rgbColorSpace) {
        return CodecError{CodecErrorCode::IncompatibleMetadata,
                          "JPEG requires a valid RGB ICC profile"};
    }

    const auto tagCount = readBig32(profile, iccTagCountOffset);
    if (tagCount > (profile.size() - iccTagTableStart) / iccTagEntrySize) {
        return CodecError{CodecErrorCode::IncompatibleMetadata,
                          "JPEG ICC tag table is truncated"};
    }
    return std::nullopt;
}

std::optional<CodecError> validateJpeg(const DecodedImage& image) {
    if (image.width() > IMP_JPEG_DIMENSION_LIMIT || image.height() > IMP_JPEG_DIMENSION_LIMIT) {
        return CodecError{CodecErrorCode::ResourceLimit,
                          "JPEG dimensions exceed the configured limit"};
    }
    if (std::get<std::vector<std::uint8_t>>(image.samples()).size() > IMP_JPEG_DATA_LIMIT) {
        return CodecError{CodecErrorCode::ResourceLimit,
                          "JPEG samples exceed the byte limit"};
    }
    if (const auto error = validateIcc(image.metadata())) {
        return error;
    }
    return std::nullopt;
}

DecodeResult decode(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return CodecError{CodecErrorCode::FileOpenFailed,
                          "Cannot open JPEG input: " + path.string()};
    }
    const auto length = file.tellg();
    if (length < 0) {
        return CodecError{CodecErrorCode::FileReadFailed,
                          "Cannot determine JPEG input length"};
    }
    if (length > static_cast<std::streamoff>(IMP_JPEG_DATA_LIMIT)) {
        return CodecError{CodecErrorCode::ResourceLimit, "JPEG file exceeds the byte limit"};
    }

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file) {
        return CodecError{CodecErrorCode::FileReadFailed, "Cannot read JPEG input"};
    }
    if (bytes.size() < jpegSignature.size()
        || !std::equal(jpegSignature.begin(), jpegSignature.end(), bytes.begin())) {
        return CodecError{CodecErrorCode::UnsupportedFormat, "Input is not a JPEG file"};
    }

    const auto release = [](ImpJpegRaster* value) { imp_jpeg_free_raster(value); delete value; };
    std::unique_ptr<ImpJpegRaster, decltype(release)> raster(new ImpJpegRaster{}, release);
    ImpJpegError error{};
    if (imp_jpeg_decode(bytes.data(), bytes.size(), raster.get(), &error) != IMP_JPEG_OK) {
        return bridgeError(error);
    }

    ImageMetadata metadata;
    if (raster->metadata.icc_size) {
        metadata.iccProfile = std::vector<std::uint8_t>(
            raster->metadata.icc, raster->metadata.icc + raster->metadata.icc_size);
    }
    if (raster->metadata.exif_size) {
        const auto orientation = readOrientation(
            {raster->metadata.exif, raster->metadata.exif_size});
        if (const auto* exifError = std::get_if<CodecError>(&orientation)) {
            return *exifError;
        }
        metadata.orientation = std::get<std::optional<Orientation>>(orientation);
    }

    return DecodedImage(raster->width, raster->height, ChannelLayout::RGB,
                        SampleDepth::Bits8,
                        std::vector<std::uint8_t>(raster->samples,
                                                  raster->samples + raster->size),
                        std::move(metadata));
}

EncodeResult encode(const std::filesystem::path& path, const DecodedImage& image,
                    int quality) {
    if (const auto error = validateJpeg(image)) {
        return *error;
    }

    const auto& samples = std::get<std::vector<std::uint8_t>>(image.samples());
    ImpJpegRaster raster{};
    raster.width = static_cast<std::uint32_t>(image.width());
    raster.height = static_cast<std::uint32_t>(image.height());
    raster.channels = static_cast<int>(image.layout());
    raster.depth = static_cast<int>(image.bitDepth());
    raster.samples = samples.data();
    raster.size = samples.size();
    std::vector<std::uint8_t> exif;
    if (image.metadata().orientation) {
        exif = writeOrientation(*image.metadata().orientation);
        raster.metadata.exif = exif.data();
        raster.metadata.exif_size = exif.size();
    }
    if (image.metadata().iccProfile) {
        raster.metadata.icc = image.metadata().iccProfile->data();
        raster.metadata.icc_size = image.metadata().iccProfile->size();
    }

    // Complete compression before opening the destination, preserving files on failure.
    const auto release = [](ImpJpegBytes* value) { imp_jpeg_free_bytes(value); delete value; };
    std::unique_ptr<ImpJpegBytes, decltype(release)> bytes(new ImpJpegBytes{}, release);
    ImpJpegError error{};
    if (imp_jpeg_encode(&raster, quality, bytes.get(), &error) != IMP_JPEG_OK) {
        return bridgeError(error);
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return CodecError{CodecErrorCode::FileOpenFailed,
                          "Cannot open JPEG output: " + path.string()};
    }
    file.write(reinterpret_cast<const char*>(bytes->bytes),
               static_cast<std::streamsize>(bytes->size));
    file.close();
    if (!file) {
        return CodecError{CodecErrorCode::FileWriteFailed,
                          "Cannot write or close JPEG output: " + path.string()};
    }
    return std::monostate{};
}

} // namespace

DecodeResult readJpeg(const std::filesystem::path& path) {
    try {
        return decode(path);
    } catch (const std::bad_alloc&) {
        return CodecError{CodecErrorCode::ResourceLimit, "Cannot allocate JPEG data"};
    } catch (const std::length_error&) {
        return CodecError{CodecErrorCode::ResourceLimit,
                          "JPEG allocation length exceeds the limit"};
    } catch (const std::overflow_error&) {
        return CodecError{CodecErrorCode::ResourceLimit, "JPEG dimensions overflow"};
    } catch (const std::invalid_argument& error) {
        return CodecError{CodecErrorCode::InvalidData, error.what()};
    }
}

EncodeResult writeJpeg(const std::filesystem::path& path, const DecodedImage& image,
                       int quality) {
    try {
        return encode(path, image, quality);
    } catch (const std::bad_alloc&) {
        return CodecError{CodecErrorCode::ResourceLimit, "Cannot allocate JPEG data"};
    } catch (const std::length_error&) {
        return CodecError{CodecErrorCode::ResourceLimit,
                          "JPEG allocation length exceeds the limit"};
    }
}

} // namespace imp_io::detail
