#include "imp_io/PngCodec.hpp"

#include <array>
#include <chrono>
#include <fstream>
#include <iterator>
#include <limits>
#include <random>
#include <string>
#include <system_error>
#include <gtest/gtest.h>

using namespace imp_io;

namespace {

const std::vector<std::uint8_t> rgb8{0, 17, 255, 128, 64, 1, 2, 3, 4, 250, 251, 252};
const std::vector<std::uint8_t> rgba8{0, 17, 255, 0, 128, 64, 1, 127, 2, 3, 4, 255, 250, 251, 252, 33};
const std::vector<std::uint16_t> rgb16{0, 1, 257, 513, 32769, 65535, 42, 4660, 43981, 65534, 256, 258};
const std::vector<std::uint16_t> rgba16{
    0, 1, 257, 0, 513, 32769, 65535, 32768, 42, 4660, 43981, 65535, 65534, 256, 258, 513
};

std::filesystem::path fixture(const char* name) {
    return std::filesystem::path(IMP_IO_FIXTURE_DIR) / name;
}

std::vector<std::uint8_t> readBytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

template<typename Result>
void expectError(const Result& result, CodecErrorCode code) {
    const auto* error = std::get_if<CodecError>(&result);
    ASSERT_NE(error, nullptr);
    EXPECT_EQ(error->code, code) << error->message;
    EXPECT_FALSE(error->message.empty());
}

class PngTest : public ::testing::Test {
protected:
    const PngCodec& codec() const { return codec_; }
    std::filesystem::path output() const { return directory_ / "output.png"; }

    template<typename T>
    void expectSamples(const char* name, std::size_t width, std::size_t height,
                       ChannelLayout layout, SampleDepth depth, const std::vector<T>& expected) {
        SCOPED_TRACE(name);
        const auto decoded = codec_.decode(fixture(name));
        ASSERT_TRUE(std::holds_alternative<DecodedImage>(decoded));
        const auto& image = std::get<DecodedImage>(decoded);
        EXPECT_EQ(image.width(), width);
        EXPECT_EQ(image.height(), height);
        EXPECT_EQ(image.layout(), layout);
        EXPECT_EQ(image.bitDepth(), depth);
        ASSERT_TRUE(std::holds_alternative<std::vector<T>>(image.samples()));
        EXPECT_EQ(std::get<std::vector<T>>(image.samples()), expected);
    }

private:
    void SetUp() override {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        directory_ = std::filesystem::temp_directory_path()
            / ("imp_png_" + std::to_string(stamp) + "_" + std::to_string(std::random_device{}()));
        std::error_code error;
        created_ = std::filesystem::create_directory(directory_, error);
        ASSERT_TRUE(created_) << error.message();
    }

    void TearDown() override {
        if (created_) {
            std::error_code ignored;
            std::filesystem::remove_all(directory_, ignored);
        }
    }

    PngCodec codec_;
    std::filesystem::path directory_;
    bool created_ = false;
};

TEST_F(PngTest, DecodesExactRgbAndRgba) {
    expectSamples("rgb8.png", 2, 2, ChannelLayout::RGB, SampleDepth::Bits8, rgb8);
    expectSamples("rgba8.png", 2, 2, ChannelLayout::RGBA, SampleDepth::Bits8, rgba8);
    expectSamples("rgb16.png", 2, 2, ChannelLayout::RGB, SampleDepth::Bits16, rgb16);
    expectSamples("rgba16.png", 2, 2, ChannelLayout::RGBA, SampleDepth::Bits16, rgba16);
}

TEST_F(PngTest, DecodesAdam7WithoutLoss) {
    expectSamples("rgb16_adam7.png", 2, 2, ChannelLayout::RGB, SampleDepth::Bits16, rgb16);
    expectSamples("rgba8_adam7.png", 2, 2, ChannelLayout::RGBA, SampleDepth::Bits8, rgba8);
}

TEST_F(PngTest, ExpandsPackedGray) {
    expectSamples<std::uint8_t>("gray1.png", 4, 1, ChannelLayout::RGB, SampleDepth::Bits8,
                                {0, 0, 0, 255, 255, 255, 255, 255, 255, 0, 0, 0});
    expectSamples<std::uint8_t>("gray2.png", 4, 1, ChannelLayout::RGB, SampleDepth::Bits8,
                                {0, 0, 0, 85, 85, 85, 170, 170, 170, 255, 255, 255});
    expectSamples<std::uint8_t>("gray4.png", 4, 1, ChannelLayout::RGB, SampleDepth::Bits8,
                                {0, 0, 0, 17, 17, 17, 136, 136, 136, 255, 255, 255});
}

TEST_F(PngTest, ExpandsGrayTransparency) {
    expectSamples<std::uint8_t>("gray8_trns.png", 4, 1, ChannelLayout::RGBA, SampleDepth::Bits8,
                                {0, 0, 0, 255, 17, 17, 17, 255, 128, 128, 128, 0, 255, 255, 255, 255});
    expectSamples<std::uint16_t>("gray16_trns.png", 4, 1, ChannelLayout::RGBA, SampleDepth::Bits16,
                                 {1, 1, 1, 65535, 257, 257, 257, 0, 513, 513, 513, 65535,
                                  65535, 65535, 65535, 65535});
    expectSamples<std::uint8_t>("gray_alpha8.png", 2, 1, ChannelLayout::RGBA, SampleDepth::Bits8,
                                {17, 17, 17, 0, 128, 128, 128, 255});
    expectSamples<std::uint16_t>("gray_alpha16.png", 2, 1, ChannelLayout::RGBA, SampleDepth::Bits16,
                                 {257, 257, 257, 17, 513, 513, 513, 65535});
}

TEST_F(PngTest, ExpandsPaletteAndRgbKeys) {
    expectSamples<std::uint8_t>("palette2_trns.png", 4, 1, ChannelLayout::RGBA, SampleDepth::Bits8,
                                {1, 2, 3, 0, 17, 64, 128, 127, 255, 0, 42, 255, 0, 255, 1, 255});
    expectSamples<std::uint8_t>("rgb8_trns.png", 2, 2, ChannelLayout::RGBA, SampleDepth::Bits8,
                                {0, 17, 255, 255, 128, 64, 1, 0, 2, 3, 4, 255, 250, 251, 252, 255});
}

TEST_F(PngTest, ReadsColorWithoutTransforms) {
    expectSamples("color_orientation.png", 2, 2, ChannelLayout::RGB, SampleDepth::Bits16, rgb16);
    const auto result = codec().decode(fixture("color_orientation.png"));
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(result));
    const auto& metadata = std::get<DecodedImage>(result).metadata();
    EXPECT_EQ(metadata.orientation, Orientation::Rotate90Clockwise);
    ASSERT_TRUE(metadata.pngColor);
    constexpr double srgbGamma = 0.45455;
    EXPECT_EQ(metadata.pngColor->gamma, srgbGamma);
    EXPECT_EQ(metadata.pngColor->srgbIntent, RenderingIntent::Saturation);
    ASSERT_TRUE(metadata.pngColor->chromaticities);
    EXPECT_DOUBLE_EQ(metadata.pngColor->chromaticities->white.x, 0.3127);
    EXPECT_DOUBLE_EQ(metadata.pngColor->chromaticities->red.y, 0.33);
    EXPECT_DOUBLE_EQ(metadata.pngColor->chromaticities->green.x, 0.30);
    EXPECT_DOUBLE_EQ(metadata.pngColor->chromaticities->blue.y, 0.06);
    expectSamples("gamma_only.png", 2, 2, ChannelLayout::RGB, SampleDepth::Bits8, rgb8);
}

TEST_F(PngTest, ReadsExifAfterImageData) {
    const auto result = codec().decode(fixture("orientation_after.png"));
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(result));
    const auto& image = std::get<DecodedImage>(result);
    EXPECT_EQ(image.metadata().orientation, Orientation::Rotate90Counterclockwise);
    EXPECT_EQ(std::get<std::vector<std::uint8_t>>(image.samples()), rgb8);
}

TEST_F(PngTest, PreservesIccBytes) {
    const auto profile = readBytes(fixture("fixture_rgb.icc"));
    ASSERT_FALSE(profile.empty());
    const auto decoded = codec().decode(fixture("icc_rgb8.png"));
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(decoded));
    const auto& image = std::get<DecodedImage>(decoded);
    ASSERT_TRUE(image.metadata().iccProfile);
    EXPECT_EQ(*image.metadata().iccProfile, profile);
    EXPECT_EQ(std::get<std::vector<std::uint8_t>>(image.samples()), rgb8);
    ASSERT_TRUE(std::holds_alternative<std::monostate>(codec().encode(output(), image)));
    const auto restored = codec().decode(output());
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(restored));
    EXPECT_EQ(std::get<DecodedImage>(restored).metadata().iccProfile, image.metadata().iccProfile);
}

TEST_F(PngTest, RoundTripsEverySampleLayout) {
    for (const auto name : {"rgb8.png", "rgba8.png", "rgb16.png", "rgba16.png"}) {
        SCOPED_TRACE(name);
        const auto decoded = codec().decode(fixture(name));
        ASSERT_TRUE(std::holds_alternative<DecodedImage>(decoded));
        const auto& original = std::get<DecodedImage>(decoded);
        const auto samples = original.samples();
        ASSERT_TRUE(std::holds_alternative<std::monostate>(codec().encode(output(), original)));
        EXPECT_EQ(original.samples(), samples);
        const auto restored = codec().decode(output());
        ASSERT_TRUE(std::holds_alternative<DecodedImage>(restored));
        const auto& image = std::get<DecodedImage>(restored);
        EXPECT_EQ(image.width(), original.width());
        EXPECT_EQ(image.height(), original.height());
        EXPECT_EQ(image.layout(), original.layout());
        EXPECT_EQ(image.bitDepth(), original.bitDepth());
        EXPECT_EQ(image.samples(), samples);
        EXPECT_FALSE(image.metadata().orientation);
        EXPECT_FALSE(image.metadata().iccProfile);
        EXPECT_FALSE(image.metadata().pngColor);
    }
}

TEST_F(PngTest, WritesSuppliedColorAndExif) {
    const auto result = codec().decode(fixture("color_orientation.png"));
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(result));
    const auto& original = std::get<DecodedImage>(result);
    ASSERT_TRUE(std::holds_alternative<std::monostate>(codec().encode(output(), original)));
    const auto restored = codec().decode(output());
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(restored));
    const auto& image = std::get<DecodedImage>(restored);
    EXPECT_EQ(image.samples(), original.samples());
    EXPECT_EQ(image.metadata().orientation, original.metadata().orientation);
    ASSERT_TRUE(image.metadata().pngColor);
    EXPECT_EQ(image.metadata().pngColor->gamma, original.metadata().pngColor->gamma);
    EXPECT_EQ(image.metadata().pngColor->srgbIntent, original.metadata().pngColor->srgbIntent);
    ASSERT_TRUE(image.metadata().pngColor->chromaticities);
    EXPECT_EQ(image.metadata().pngColor->chromaticities->white.x,
              original.metadata().pngColor->chromaticities->white.x);
}

TEST_F(PngTest, KeepsAllOrientationsUnapplied) {
    const std::array orientations{
        Orientation::Identity, Orientation::MirrorHorizontal, Orientation::Rotate180,
        Orientation::MirrorVertical, Orientation::Transpose, Orientation::Rotate90Clockwise,
        Orientation::Transverse, Orientation::Rotate90Counterclockwise
    };
    for (const auto orientation : orientations) {
        ImageMetadata metadata;
        metadata.orientation = orientation;
        const DecodedImage image(4, 1, ChannelLayout::RGB, SampleDepth::Bits8, rgb8, metadata);
        ASSERT_TRUE(std::holds_alternative<std::monostate>(codec().encode(output(), image)));
        const auto result = codec().decode(output());
        ASSERT_TRUE(std::holds_alternative<DecodedImage>(result));
        const auto& restored = std::get<DecodedImage>(result);
        EXPECT_EQ(restored.width(), 4u);
        EXPECT_EQ(restored.height(), 1u);
        EXPECT_EQ(restored.metadata().orientation, orientation);
        EXPECT_EQ(restored.samples(), image.samples());
    }
}

TEST_F(PngTest, RejectsMalformedFiles) {
    for (const auto name : {"bad_crc.png", "truncated.png", "bad_icc.png", "bad_exif.png", "bad_exif_offset.png"}) {
        SCOPED_TRACE(name);
        expectError(codec().decode(fixture(name)), CodecErrorCode::InvalidData);
    }
    expectError(codec().decode(fixture("not_png.bin")), CodecErrorCode::UnsupportedFormat);
    expectError(codec().decode(fixture("animation.png")), CodecErrorCode::UnsupportedFeature);
    expectError(codec().decode(fixture("extended_color.png")), CodecErrorCode::UnsupportedFeature);
    expectError(codec().decode(fixture("oversized.png")), CodecErrorCode::ResourceLimit);
}

TEST_F(PngTest, ReportsFileOpenFailures) {
    expectError(codec().decode(output()), CodecErrorCode::FileOpenFailed);
    const DecodedImage image(2, 2, ChannelLayout::RGB, SampleDepth::Bits8, rgb8);
    expectError(codec().encode(output() / "missing" / "image.png", image), CodecErrorCode::FileOpenFailed);
    expectError(codec().encode(output().parent_path(), image), CodecErrorCode::FileOpenFailed);
}

TEST_F(PngTest, ReportsWriteFailure) {
    const std::filesystem::path fullDevice("/dev/full");
    if (!std::filesystem::exists(fullDevice)) {
        GTEST_SKIP() << "No failing-write device on this platform";
    }
    const DecodedImage image(2, 2, ChannelLayout::RGB, SampleDepth::Bits8, rgb8);
    expectError(codec().encode(fullDevice, image), CodecErrorCode::FileWriteFailed);
}

TEST_F(PngTest, PreservesFileOnInvalidMetadata) {
    const std::string sentinel = "existing file";
    {
        std::ofstream file(output(), std::ios::binary);
        file << sentinel;
    }
    ImageMetadata metadata;
    metadata.iccProfile = std::vector<std::uint8_t>{1, 2, 3};
    const DecodedImage image(2, 2, ChannelLayout::RGB, SampleDepth::Bits8, rgb8, metadata);
    expectError(codec().encode(output(), image), CodecErrorCode::IncompatibleMetadata);
    const auto bytes = readBytes(output());
    EXPECT_EQ(std::string(bytes.begin(), bytes.end()), sentinel);
}

TEST_F(PngTest, RejectsUnrepresentableGamma) {
    for (const auto gamma : {std::numeric_limits<double>::min(), std::numeric_limits<double>::max()}) {
        ImageMetadata metadata;
        metadata.pngColor = PngColorDescription{gamma, {}, {}};
        const DecodedImage image(2, 2, ChannelLayout::RGB, SampleDepth::Bits8, rgb8, metadata);
        expectError(codec().encode(output(), image), CodecErrorCode::IncompatibleMetadata);
        EXPECT_FALSE(std::filesystem::exists(output()));
    }
}

TEST_F(PngTest, IgnoresQualityWithoutMutation) {
    const DecodedImage image(2, 2, ChannelLayout::RGB, SampleDepth::Bits16, rgb16);
    ASSERT_TRUE(std::holds_alternative<std::monostate>(codec().encode(output(), image)));
    const auto expected = readBytes(output());
    ASSERT_TRUE(std::holds_alternative<std::monostate>(codec().encode(output(), image, {0})));
    EXPECT_EQ(readBytes(output()), expected);
    EXPECT_EQ(std::get<std::vector<std::uint16_t>>(image.samples()), rgb16);
}

} // namespace
