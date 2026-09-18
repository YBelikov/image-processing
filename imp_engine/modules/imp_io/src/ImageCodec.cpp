#include "imp_io/ImageCodec.hpp"

#include <cmath>
#include <optional>
#include <stdexcept>

namespace imp_io {
namespace {

// PNG metadata uses fixed-point values; compare sRGB declarations at that precision.
constexpr double pngMetadataScale = 100000.0;
constexpr double srgbGamma = 0.45455;
constexpr Chromaticities srgbChromaticities{
    {0.31270, 0.32900}, {0.64000, 0.33000}, {0.30000, 0.60000}, {0.15000, 0.06000}
};

bool validPath(const std::filesystem::path& path) {
    const auto& native = path.native();
    return !native.empty() && native.find(std::filesystem::path::value_type{}) == native.npos;
}

bool supportsDepth(ImageFormat format, SampleDepth depth) {
    switch (format) {
        case ImageFormat::PNG:
            return depth == SampleDepth::Bits8 || depth == SampleDepth::Bits16;
        case ImageFormat::JPEG:
            return depth == SampleDepth::Bits8;
    }

    return false;
}

CodecError metadataError(const char* message) {
    return {CodecErrorCode::IncompatibleMetadata, message};
}

bool validChromaticity(const Chromaticity& point) {
    return std::isfinite(point.x) && std::isfinite(point.y)
        && point.x >= 0.0 && point.y >= 0.0 && point.x + point.y <= 1.0;
}

bool samePngValue(double left, double right) {
    return std::round(left * pngMetadataScale) == std::round(right * pngMetadataScale);
}

bool sameChromaticity(const Chromaticity& left, const Chromaticity& right) {
    return samePngValue(left.x, right.x) && samePngValue(left.y, right.y);
}

std::optional<CodecError> validatePngColor(const ImageMetadata& metadata) {
    if (!metadata.pngColor) {
        return std::nullopt;
    }

    const auto& color = *metadata.pngColor;
    if (color.gamma && (!std::isfinite(*color.gamma) || *color.gamma <= 0.0)) {
        return metadataError("PNG image gamma must be finite and positive");
    }

    if (color.chromaticities) {
        const auto& points = *color.chromaticities;
        if (!validChromaticity(points.white) || !validChromaticity(points.red)
            || !validChromaticity(points.green) || !validChromaticity(points.blue)) {
            return metadataError("PNG chromaticities must be finite xy coordinates within the unit triangle");
        }
    }

    if (!color.srgbIntent) {
        return std::nullopt;
    }

    if (*color.srgbIntent > RenderingIntent::AbsoluteColorimetric) {
        return metadataError("Invalid PNG sRGB rendering intent");
    }

    if (metadata.iccProfile) {
        return metadataError("Supply either ICC or PNG sRGB metadata, not both");
    }

    if (color.gamma && !samePngValue(*color.gamma, srgbGamma)) {
        return metadataError("PNG gamma contradicts the supplied sRGB description");
    }

    if (color.chromaticities) {
        const auto& points = *color.chromaticities;
        if (!sameChromaticity(points.white, srgbChromaticities.white)
            || !sameChromaticity(points.red, srgbChromaticities.red)
            || !sameChromaticity(points.green, srgbChromaticities.green)
            || !sameChromaticity(points.blue, srgbChromaticities.blue)) {
            return metadataError("PNG chromaticities contradict the supplied sRGB description");
        }
    }

    return std::nullopt;
}

std::optional<CodecError> validateMetadata(ImageFormat format, const ImageMetadata& metadata) {
    if (metadata.orientation
        && (*metadata.orientation < Orientation::Identity
            || *metadata.orientation > Orientation::Rotate90Counterclockwise)) {
        return metadataError("Invalid orientation value");
    }

    if (metadata.iccProfile && metadata.iccProfile->empty()) {
        return metadataError("An explicitly supplied ICC profile must not be empty");
    }

    if (metadata.pngColor && format != ImageFormat::PNG) {
        return metadataError("PNG color descriptions require PNG output");
    }

    return validatePngColor(metadata);
}

} // namespace

ImageCodec::ImageCodec(ImageFormat format) : format_(format) {
    switch (format_) {
        case ImageFormat::PNG:
        case ImageFormat::JPEG:
            return;
    }

    throw std::invalid_argument("Unsupported codec format");
}

DecodeResult ImageCodec::decode(const std::filesystem::path& path) const {
    if (!validPath(path)) {
        return CodecError{CodecErrorCode::InvalidArgument, "Input path must be nonempty and contain no NUL"};
    }

    return decodeFile(path);
}

EncodeResult ImageCodec::encode(const std::filesystem::path& path, const DecodedImage& image,
                                const EncodeOptions& options) const {
    if (!validPath(path)) {
        return CodecError{CodecErrorCode::InvalidArgument, "Output path must be nonempty and contain no NUL"};
    }

    const auto validation = validateEncoding(image, options);
    if (std::holds_alternative<CodecError>(validation)) {
        return validation;
    }

    return encodeFile(path, image, options);
}

EncodeResult ImageCodec::validateEncoding(const DecodedImage& image,
                                         const EncodeOptions& options) const {
    if (format_ == ImageFormat::JPEG
        && (options.quality < minEncodeQuality || options.quality > maxEncodeQuality)) {
        return CodecError{CodecErrorCode::InvalidArgument, "JPEG quality must be between 1 and 100"};
    }

    if (format_ == ImageFormat::JPEG && image.layout() == ChannelLayout::RGBA) {
        return CodecError{CodecErrorCode::UnsupportedLayout, "JPEG requires RGB; prepare alpha explicitly"};
    }

    if (!supportsDepth(format_, image.bitDepth())) {
        return CodecError{CodecErrorCode::UnsupportedPrecision, "Sample depth is unsupported by the output format"};
    }

    if (const auto error = validateMetadata(format_, image.metadata())) {
        return *error;
    }

    return std::monostate{};
}

} // namespace imp_io
