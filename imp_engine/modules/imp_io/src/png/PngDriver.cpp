#include "PngDriver.hpp"
#include "PngBridge.h"
#include "../ExifOrientation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace imp_io::detail {
namespace {

constexpr std::array<unsigned char, 8> pngSignature{137, 80, 78, 71, 13, 10, 26, 10};
constexpr double pngFixedScale = 100000.0;
CodecError bridgeError(const ImpPngError& error) {
    CodecErrorCode code = CodecErrorCode::DecodeFailed;
    switch (error.code) {
        case IMP_PNG_INVALID: code = CodecErrorCode::InvalidData; break;
        case IMP_PNG_METADATA: code = CodecErrorCode::IncompatibleMetadata; break;
        case IMP_PNG_UNSUPPORTED: code = CodecErrorCode::UnsupportedFeature; break;
        case IMP_PNG_RESOURCE: code = CodecErrorCode::ResourceLimit; break;
        case IMP_PNG_ENCODE: code = CodecErrorCode::EncodeFailed; break;
        case IMP_PNG_OK: break;
    }
    return {code, error.message};
}

ImageMetadata readMetadata(const ImpPngMetadata& source) {
    ImageMetadata metadata;
    if (source.icc_size) {
        metadata.iccProfile = std::vector<std::uint8_t>(source.icc, source.icc + source.icc_size);
    }
    if (source.fields) {
        PngColorDescription color;
        if (source.fields & IMP_PNG_GAMMA) {
            color.gamma = source.gamma;
        }
        if (source.fields & IMP_PNG_CHROMATICITIES) {
            const auto* xy = source.xy;
            color.chromaticities = Chromaticities{{xy[0], xy[1]}, {xy[2], xy[3]},
                                                   {xy[4], xy[5]}, {xy[6], xy[7]}};
        }
        if (source.fields & IMP_PNG_SRGB) {
            color.srgbIntent = static_cast<RenderingIntent>(source.intent);
        }
        metadata.pngColor = color;
    }
    return metadata;
}

ImpPngMetadata writeMetadata(const ImageMetadata& metadata) {
    ImpPngMetadata target{};
    if (metadata.iccProfile) {
        target.icc = metadata.iccProfile->data();
        target.icc_size = metadata.iccProfile->size();
    }
    if (!metadata.pngColor) {
        return target;
    }
    const auto& color = *metadata.pngColor;
    if (color.gamma) {
        target.fields |= IMP_PNG_GAMMA;
        target.gamma = *color.gamma;
    }
    if (color.chromaticities) {
        target.fields |= IMP_PNG_CHROMATICITIES;
        const auto& points = *color.chromaticities;
        const std::array xy{points.white.x, points.white.y, points.red.x, points.red.y,
                            points.green.x, points.green.y, points.blue.x, points.blue.y};
        std::copy(xy.begin(), xy.end(), target.xy);
    }
    if (color.srgbIntent) {
        target.fields |= IMP_PNG_SRGB;
        target.intent = static_cast<int>(*color.srgbIntent);
    }
    return target;
}

std::optional<CodecError> validatePng(const DecodedImage& image) {
    if (image.width() > IMP_PNG_DIMENSION_LIMIT || image.height() > IMP_PNG_DIMENSION_LIMIT) {
        return CodecError{CodecErrorCode::ResourceLimit, "PNG dimensions exceed the configured limit"};
    }
    if (image.metadata().iccProfile && image.metadata().iccProfile->size() > IMP_PNG_METADATA_LIMIT) {
        return CodecError{CodecErrorCode::ResourceLimit, "PNG ICC profile exceeds the byte limit"};
    }
    const auto& color = image.metadata().pngColor;
    if (color && color->gamma) {
        const auto scaled = *color->gamma * pngFixedScale;
        if (!std::isfinite(scaled) || std::round(scaled) < 1
            || std::round(scaled) > std::numeric_limits<std::int32_t>::max()) {
            return CodecError{CodecErrorCode::IncompatibleMetadata, "PNG gamma is not representable by libpng"};
        }
    }
    return std::nullopt;
}

DecodeResult decode(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return CodecError{CodecErrorCode::FileOpenFailed, "Cannot open PNG input: " + path.string()};
    }
    const auto length = file.tellg();
    if (length < 0) {
        return CodecError{CodecErrorCode::FileReadFailed, "Cannot determine PNG input length"};
    }
    if (length > static_cast<std::streamoff>(IMP_PNG_DATA_LIMIT)) {
        return CodecError{CodecErrorCode::ResourceLimit, "PNG file exceeds the byte limit"};
    }
    std::vector<unsigned char> bytes(static_cast<std::size_t>(length));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file) {
        return CodecError{CodecErrorCode::FileReadFailed, "Cannot read PNG input"};
    }
    if (bytes.size() < pngSignature.size()) {
        return CodecError{CodecErrorCode::InvalidData, "Truncated PNG signature"};
    }
    if (!std::equal(pngSignature.begin(), pngSignature.end(), bytes.begin())) {
        return CodecError{CodecErrorCode::UnsupportedFormat, "Input is not a PNG file"};
    }

    const auto release = [](ImpPngRaster* value) { imp_png_free_raster(value); delete value; };
    std::unique_ptr<ImpPngRaster, decltype(release)> raster(new ImpPngRaster{}, release);
    ImpPngError error{};
    if (imp_png_decode(bytes.data(), bytes.size(), raster.get(), &error) != IMP_PNG_OK) {
        return bridgeError(error);
    }
    auto metadata = readMetadata(raster->metadata);
    if (raster->metadata.exif_size) {
        const auto orientation = readOrientation({raster->metadata.exif, raster->metadata.exif_size});
        if (const auto* error = std::get_if<CodecError>(&orientation)) {
            return *error;
        }
        metadata.orientation = std::get<std::optional<Orientation>>(orientation);
    }
    const auto layout = static_cast<ChannelLayout>(raster->channels);
    if (raster->depth == static_cast<int>(SampleDepth::Bits8)) {
        return DecodedImage(raster->width, raster->height, layout, SampleDepth::Bits8,
                            std::vector<std::uint8_t>(raster->samples, raster->samples + raster->size),
                            std::move(metadata));
    }
    std::vector<std::uint16_t> samples(raster->size / sizeof(std::uint16_t));
    std::memcpy(samples.data(), raster->samples, raster->size);
    return DecodedImage(raster->width, raster->height, layout, SampleDepth::Bits16,
                        std::move(samples), std::move(metadata));
}

EncodeResult encode(const std::filesystem::path& path, const DecodedImage& image) {
    if (const auto error = validatePng(image)) {
        return *error;
    }
    ImpPngRaster raster{};
    raster.width = static_cast<std::uint32_t>(image.width());
    raster.height = static_cast<std::uint32_t>(image.height());
    raster.depth = static_cast<int>(image.bitDepth());
    raster.channels = static_cast<int>(image.layout());
    raster.metadata = writeMetadata(image.metadata());
    std::visit([&raster](const auto& samples) {
        raster.samples = reinterpret_cast<const unsigned char*>(samples.data());
        raster.size = samples.size() * sizeof(typename std::decay_t<decltype(samples)>::value_type);
    }, image.samples());
    if (raster.size > IMP_PNG_DATA_LIMIT) {
        return CodecError{CodecErrorCode::ResourceLimit, "PNG samples exceed the byte limit"};
    }
    auto exif = writeOrientation(image.metadata().orientation.value_or(Orientation::Identity));
    if (image.metadata().orientation) {
        raster.metadata.exif = exif.data();
        raster.metadata.exif_size = exif.size();
    }

    // Finish encoding before opening the destination, preserving files on validation failure.
    const auto release = [](ImpPngBytes* value) { imp_png_free_bytes(value); delete value; };
    std::unique_ptr<ImpPngBytes, decltype(release)> bytes(new ImpPngBytes{}, release);
    ImpPngError error{};
    if (imp_png_encode(&raster, bytes.get(), &error) != IMP_PNG_OK) {
        return bridgeError(error);
    }
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return CodecError{CodecErrorCode::FileOpenFailed, "Cannot open PNG output: " + path.string()};
    }
    file.write(reinterpret_cast<const char*>(bytes->bytes), static_cast<std::streamsize>(bytes->size));
    file.close();
    if (!file) {
        return CodecError{CodecErrorCode::FileWriteFailed, "Cannot write or close PNG output: " + path.string()};
    }
    return std::monostate{};
}

} // namespace

DecodeResult readPng(const std::filesystem::path& path) {
    try {
        return decode(path);
    } catch (const std::bad_alloc&) {
        return CodecError{CodecErrorCode::ResourceLimit, "Cannot allocate PNG data"};
    } catch (const std::length_error&) {
        return CodecError{CodecErrorCode::ResourceLimit, "PNG allocation length exceeds the limit"};
    } catch (const std::overflow_error&) {
        return CodecError{CodecErrorCode::ResourceLimit, "PNG dimensions overflow"};
    } catch (const std::invalid_argument& error) {
        return CodecError{CodecErrorCode::InvalidData, error.what()};
    }
}

EncodeResult writePng(const std::filesystem::path& path, const DecodedImage& image) {
    try {
        return encode(path, image);
    } catch (const std::bad_alloc&) {
        return CodecError{CodecErrorCode::ResourceLimit, "Cannot allocate PNG data"};
    } catch (const std::length_error&) {
        return CodecError{CodecErrorCode::ResourceLimit, "PNG allocation length exceeds the limit"};
    }
}

} // namespace imp_io::detail
