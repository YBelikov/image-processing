#include "imp_io/ImageCodec.hpp"

#include <array>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <gtest/gtest.h>

using namespace imp_io;

namespace {

// A recording backend exercises the contract without opening files or codecs.
class ProbeCodec final : public ImageCodec {
public:
    explicit ProbeCodec(ImageFormat format, std::optional<CodecError> failure = {})
        : ImageCodec(format), failure_(std::move(failure)) {}

    std::size_t decodeCalls() const { return decodeCalls_; }
    std::size_t encodeCalls() const { return encodeCalls_; }
    int quality() const { return quality_; }
    const DecodedImage* input() const { return input_; }
    const std::filesystem::path& path() const { return path_; }

private:
    DecodeResult decodeFile(const std::filesystem::path& path) const override {
        ++decodeCalls_;
        path_ = path;
        if (failure_) {
            return *failure_;
        }

        ImageMetadata metadata;
        metadata.orientation = Orientation::Rotate90Clockwise;
        return DecodedImage(1, 2, ChannelLayout::RGB, SampleDepth::Bits8,
                            std::vector<std::uint8_t>{1, 2, 3, 4, 5, 6}, metadata);
    }

    EncodeResult encodeFile(const std::filesystem::path& path, const DecodedImage& image,
                             const EncodeOptions& options) const override {
        ++encodeCalls_;
        path_ = path;
        input_ = &image;
        quality_ = options.quality;
        if (failure_) {
            return *failure_;
        }

        return std::monostate{};
    }

    std::optional<CodecError> failure_;
    mutable std::size_t decodeCalls_ = 0;
    mutable std::size_t encodeCalls_ = 0;
    mutable std::filesystem::path path_;
    mutable const DecodedImage* input_ = nullptr;
    mutable int quality_ = defaultEncodeQuality;
};

DecodedImage makeImage(SampleDepth depth, ChannelLayout layout, ImageMetadata metadata = {}) {
    const auto count = static_cast<std::size_t>(layout);
    switch (depth) {
        case SampleDepth::Bits8:
            return {1, 1, layout, depth, std::vector<std::uint8_t>(count, 127), std::move(metadata)};
        case SampleDepth::Bits10:
        case SampleDepth::Bits16:
            return {1, 1, layout, depth, std::vector<std::uint16_t>(count, 513), std::move(metadata)};
        case SampleDepth::Bits32:
            return {1, 1, layout, depth, std::vector<float>(count, 12.5f), std::move(metadata)};
    }

    throw std::invalid_argument("Unsupported test image depth");
}

template<typename Result>
void expectError(const Result& result, CodecErrorCode code) {
    const auto* error = std::get_if<CodecError>(&result);
    ASSERT_NE(error, nullptr);
    EXPECT_EQ(error->code, code);
    EXPECT_FALSE(error->message.empty());
}

struct EncodingCase {
    ImageFormat format;
    SampleDepth depth;
    ChannelLayout layout;
};

} // namespace

TEST(ImageCodec, PreservesDecodedResult) {
    const ProbeCodec codec(ImageFormat::PNG);
    auto result = codec.decode("input.bin");
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(result));
    const auto image = std::get<DecodedImage>(std::move(result));

    EXPECT_EQ(codec.format(), ImageFormat::PNG);
    EXPECT_EQ(codec.decodeCalls(), 1u);
    EXPECT_EQ(codec.path(), "input.bin");
    EXPECT_EQ(image.width(), 1u);
    EXPECT_EQ(image.height(), 2u);
    EXPECT_EQ(image.metadata().orientation, Orientation::Rotate90Clockwise);
    EXPECT_EQ(std::get<std::vector<std::uint8_t>>(image.samples()),
              (std::vector<std::uint8_t>{1, 2, 3, 4, 5, 6}));
}

TEST(ImageCodec, AcceptsSupportedInputs) {
    const std::array cases{
        EncodingCase{ImageFormat::PNG, SampleDepth::Bits8, ChannelLayout::RGB},
        EncodingCase{ImageFormat::PNG, SampleDepth::Bits8, ChannelLayout::RGBA},
        EncodingCase{ImageFormat::PNG, SampleDepth::Bits16, ChannelLayout::RGB},
        EncodingCase{ImageFormat::PNG, SampleDepth::Bits16, ChannelLayout::RGBA},
        EncodingCase{ImageFormat::JPEG, SampleDepth::Bits8, ChannelLayout::RGB},
        EncodingCase{ImageFormat::HEIC, SampleDepth::Bits8, ChannelLayout::RGB},
        EncodingCase{ImageFormat::HEIC, SampleDepth::Bits8, ChannelLayout::RGBA},
        EncodingCase{ImageFormat::HEIC, SampleDepth::Bits10, ChannelLayout::RGB},
        EncodingCase{ImageFormat::HEIC, SampleDepth::Bits10, ChannelLayout::RGBA}
    };

    for (const auto& entry : cases) {
        const ProbeCodec codec(entry.format);
        const auto image = makeImage(entry.depth, entry.layout);
        const auto result = codec.encode("output.bin", image);

        EXPECT_TRUE(std::holds_alternative<std::monostate>(result));
        EXPECT_EQ(codec.encodeCalls(), 1u);
        EXPECT_EQ(codec.input(), &image);
        EXPECT_EQ(codec.path(), "output.bin");
        EXPECT_EQ(codec.quality(), defaultEncodeQuality);
    }
}

TEST(ImageCodec, RejectsPrecisionConversion) {
    const std::array cases{
        EncodingCase{ImageFormat::PNG, SampleDepth::Bits10, ChannelLayout::RGB},
        EncodingCase{ImageFormat::PNG, SampleDepth::Bits32, ChannelLayout::RGBA},
        EncodingCase{ImageFormat::JPEG, SampleDepth::Bits10, ChannelLayout::RGB},
        EncodingCase{ImageFormat::JPEG, SampleDepth::Bits16, ChannelLayout::RGB},
        EncodingCase{ImageFormat::JPEG, SampleDepth::Bits32, ChannelLayout::RGB},
        EncodingCase{ImageFormat::HEIC, SampleDepth::Bits16, ChannelLayout::RGBA},
        EncodingCase{ImageFormat::HEIC, SampleDepth::Bits32, ChannelLayout::RGB}
    };

    for (const auto& entry : cases) {
        const ProbeCodec codec(entry.format);
        const auto image = makeImage(entry.depth, entry.layout);
        expectError(codec.encode("output.bin", image), CodecErrorCode::UnsupportedPrecision);
        EXPECT_EQ(codec.encodeCalls(), 0u);
    }
}

TEST(ImageCodec, RejectsAlphaForJpeg) {
    const ProbeCodec codec(ImageFormat::JPEG);
    const auto image = makeImage(SampleDepth::Bits8, ChannelLayout::RGBA);
    expectError(codec.encode("output.jpeg", image), CodecErrorCode::UnsupportedLayout);
    EXPECT_EQ(codec.encodeCalls(), 0u);
}

TEST(ImageCodec, ValidatesLossyQuality) {
    constexpr int expectedDefaultQuality = 90;
    EXPECT_EQ(EncodeOptions{}.quality, expectedDefaultQuality);
    for (const auto format : {ImageFormat::JPEG, ImageFormat::HEIC}) {
        const ProbeCodec codec(format);
        const auto image = makeImage(SampleDepth::Bits8, ChannelLayout::RGB);
        for (const auto quality : {minEncodeQuality - 1, maxEncodeQuality + 1}) {
            expectError(codec.encode("output.bin", image, {quality}), CodecErrorCode::InvalidArgument);
        }
        EXPECT_EQ(codec.encodeCalls(), 0u);

        for (const auto quality : {minEncodeQuality, defaultEncodeQuality, maxEncodeQuality}) {
            EXPECT_TRUE(std::holds_alternative<std::monostate>(codec.encode("output.bin", image, {quality})));
            EXPECT_EQ(codec.quality(), quality);
        }
    }
}

TEST(ImageCodec, IgnoresQualityForPng) {
    const ProbeCodec codec(ImageFormat::PNG);
    const auto image = makeImage(SampleDepth::Bits16, ChannelLayout::RGBA);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(
        codec.encode("output.png", image, {minEncodeQuality - 1})));
}

TEST(ImageCodec, RejectsInvalidPathsBeforeIo) {
    const ProbeCodec codec(ImageFormat::PNG);
    const auto image = makeImage(SampleDepth::Bits8, ChannelLayout::RGB);
    std::string embeddedNul = "image";
    embeddedNul.push_back('\0');
    embeddedNul += ".png";

    for (const auto& path : {std::filesystem::path{}, std::filesystem::path{embeddedNul}}) {
        expectError(codec.decode(path), CodecErrorCode::InvalidArgument);
        expectError(codec.encode(path, image), CodecErrorCode::InvalidArgument);
    }

    EXPECT_EQ(codec.decodeCalls(), 0u);
    EXPECT_EQ(codec.encodeCalls(), 0u);
}

TEST(ImageCodec, PreservesBackendErrors) {
    constexpr auto diagnostic = "HEIC codec unavailable in this build";
    const ProbeCodec codec(ImageFormat::HEIC, CodecError{CodecErrorCode::CodecUnavailable, diagnostic});
    const auto image = makeImage(SampleDepth::Bits10, ChannelLayout::RGB);
    const auto decoded = codec.decode("input.heic");
    const auto encoded = codec.encode("output.heic", image);

    expectError(decoded, CodecErrorCode::CodecUnavailable);
    expectError(encoded, CodecErrorCode::CodecUnavailable);
    EXPECT_EQ(std::get<CodecError>(decoded).message, diagnostic);
    EXPECT_EQ(std::get<CodecError>(encoded).message, diagnostic);
}

TEST(ImageCodec, RejectsForeignMetadata) {
    ImageMetadata png;
    png.pngColor = PngColorDescription{};
    ImageMetadata nclx;
    constexpr std::uint16_t unspecifiedCodePoint = 2;
    nclx.nclxColor = NclxColorDescription{
        unspecifiedCodePoint, unspecifiedCodePoint, unspecifiedCodePoint, ColorRange::Full
    };

    for (const auto format : {ImageFormat::JPEG, ImageFormat::HEIC}) {
        const ProbeCodec codec(format);
        expectError(codec.encode("output.bin", makeImage(SampleDepth::Bits8, ChannelLayout::RGB, png)),
                    CodecErrorCode::IncompatibleMetadata);
        EXPECT_EQ(codec.encodeCalls(), 0u);
    }

    for (const auto format : {ImageFormat::PNG, ImageFormat::JPEG}) {
        const ProbeCodec codec(format);
        expectError(codec.encode("output.bin", makeImage(SampleDepth::Bits8, ChannelLayout::RGB, nclx)),
                    CodecErrorCode::IncompatibleMetadata);
        EXPECT_EQ(codec.encodeCalls(), 0u);
    }
}

TEST(ImageCodec, RejectsMalformedMetadata) {
    ImageMetadata orientation;
    orientation.orientation = static_cast<Orientation>(0);
    ImageMetadata icc;
    icc.iccProfile = std::vector<std::uint8_t>{};
    ImageMetadata gamma;
    gamma.pngColor = PngColorDescription{std::numeric_limits<double>::quiet_NaN(), {}, {}};
    ImageMetadata chromaticities;
    chromaticities.pngColor = PngColorDescription{
        {}, Chromaticities{{0.8, 0.8}, {}, {}, {}}, {}
    };
    ImageMetadata intent;
    intent.pngColor = PngColorDescription{{}, {}, static_cast<RenderingIntent>(255)};

    const ProbeCodec codec(ImageFormat::PNG);
    for (const auto& metadata : {orientation, icc, gamma, chromaticities, intent}) {
        expectError(codec.encode("output.png", makeImage(SampleDepth::Bits8, ChannelLayout::RGB, metadata)),
                    CodecErrorCode::IncompatibleMetadata);
    }
    EXPECT_EQ(codec.encodeCalls(), 0u);
}

TEST(ImageCodec, RejectsConflictingPngColor) {
    ImageMetadata iccAndSrgb;
    iccAndSrgb.iccProfile = std::vector<std::uint8_t>{1};
    iccAndSrgb.pngColor = PngColorDescription{{}, {}, RenderingIntent::Perceptual};
    ImageMetadata gammaAndSrgb;
    gammaAndSrgb.pngColor = PngColorDescription{1.0, {}, RenderingIntent::Perceptual};
    ImageMetadata primariesAndSrgb;
    primariesAndSrgb.pngColor = PngColorDescription{
        {}, Chromaticities{{0.3, 0.3}, {0.6, 0.3}, {0.3, 0.6}, {0.1, 0.1}}, RenderingIntent::Perceptual
    };

    const ProbeCodec codec(ImageFormat::PNG);
    for (const auto& metadata : {iccAndSrgb, gammaAndSrgb, primariesAndSrgb}) {
        expectError(codec.encode("output.png", makeImage(SampleDepth::Bits8, ChannelLayout::RGB, metadata)),
                    CodecErrorCode::IncompatibleMetadata);
    }
    EXPECT_EQ(codec.encodeCalls(), 0u);
}

TEST(ImageCodec, AcceptsConsistentPngColor) {
    constexpr double srgbGamma = 0.45455;
    constexpr Chromaticities srgbPrimaries{
        {0.3127, 0.3290}, {0.64, 0.33}, {0.30, 0.60}, {0.15, 0.06}
    };
    ImageMetadata metadata;
    metadata.orientation = Orientation::Rotate90Clockwise;
    metadata.pngColor = PngColorDescription{srgbGamma, srgbPrimaries, RenderingIntent::Perceptual};
    const auto image = makeImage(SampleDepth::Bits16, ChannelLayout::RGBA, metadata);
    const ProbeCodec codec(ImageFormat::PNG);

    EXPECT_TRUE(std::holds_alternative<std::monostate>(codec.encode("output.png", image)));
    EXPECT_EQ(codec.input(), &image);
    EXPECT_EQ(image.metadata().orientation, metadata.orientation);
    EXPECT_EQ(std::get<std::vector<std::uint16_t>>(image.samples()),
              (std::vector<std::uint16_t>{513, 513, 513, 513}));
}

TEST(ImageCodec, PassesSupportedColorMetadata) {
    // Opaque ICC bytes probe transport; actual profile parsing belongs to codecs.
    ImageMetadata icc;
    icc.iccProfile = std::vector<std::uint8_t>{1, 2, 3};
    icc.orientation = Orientation::MirrorHorizontal;
    for (const auto format : {ImageFormat::PNG, ImageFormat::JPEG, ImageFormat::HEIC}) {
        const ProbeCodec codec(format);
        const auto image = makeImage(SampleDepth::Bits8, ChannelLayout::RGB, icc);
        EXPECT_TRUE(std::holds_alternative<std::monostate>(codec.encode("output.bin", image)));
        EXPECT_EQ(codec.input(), &image);
    }

    constexpr std::uint16_t unknownCodePoint = 65000;
    icc.nclxColor = NclxColorDescription{
        unknownCodePoint, unknownCodePoint, unknownCodePoint, ColorRange::Limited
    };
    const ProbeCodec heic(ImageFormat::HEIC);
    const auto image = makeImage(SampleDepth::Bits10, ChannelLayout::RGBA, icc);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(heic.encode("output.heic", image)));
    EXPECT_EQ(heic.input(), &image);
}

TEST(ImageCodec, RejectsInvalidNclxRange) {
    ImageMetadata metadata;
    metadata.nclxColor = NclxColorDescription{};
    metadata.nclxColor->range = static_cast<ColorRange>(255);
    const ProbeCodec codec(ImageFormat::HEIC);
    expectError(codec.encode("output.heic", makeImage(SampleDepth::Bits10, ChannelLayout::RGB, metadata)),
                CodecErrorCode::IncompatibleMetadata);
    EXPECT_EQ(codec.encodeCalls(), 0u);
}

TEST(ImageCodec, RejectsUnknownCodecFormat) {
    EXPECT_THROW(ProbeCodec(static_cast<ImageFormat>(255)), std::invalid_argument);
}
