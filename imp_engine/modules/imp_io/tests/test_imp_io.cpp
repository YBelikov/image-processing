#include "imp_io/ImageData.hpp"
#include "imp_io/ImageReader.hpp"
#include "imp_io/ImageWriter.hpp"
#include "imp_io/PngCodec.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <gtest/gtest.h>

using namespace imp_io;

namespace {

std::filesystem::path pngFixture(const char* name) {
    return std::filesystem::path(IMP_IO_FIXTURE_DIR) / name;
}

std::filesystem::path tempFile(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}

static ImageDataRGB makeSyntheticRgbImage(int width, int height) {
    ImageDataRGB img(width, height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto index = static_cast<std::size_t>(y) * width + x;
            img.pixels[index] = {
                static_cast<float>(x) / static_cast<float>(width),
                static_cast<float>(y) / static_cast<float>(height),
                static_cast<float>(x + y) / static_cast<float>(width + height)
            };
        }
    }

    return img;
}

} // namespace

TEST(ImpIo, ImageDataEmpty) {
    ImageDataRGB img;
    EXPECT_TRUE(img.empty());
    EXPECT_EQ(img.channels, 0);
}

TEST(ImpIo, ImageDataNotEmpty) {
    ImageDataRGB img = makeSyntheticRgbImage(2, 2);
    EXPECT_FALSE(img.empty());
    EXPECT_EQ(img.width, 2);
    EXPECT_EQ(img.height, 2);
    EXPECT_EQ(img.channels, 3);
    EXPECT_EQ(img.stride(), 6u);
    EXPECT_EQ(img.pixels.size(), 4u);
}

TEST(ImpIo, HdrRoundTripDimensions) {
    const char* tmpPath = "/tmp/imp_io_test_roundtrip.hdr";

    ImageDataRGB original = makeSyntheticRgbImage(8, 8);

    ImageWriter<PixelRGB_F> writer;
    ASSERT_TRUE(writer.write(original, tmpPath));

    ImageReader<PixelRGB_F> reader;
    auto loaded = reader.read(tmpPath);
    ASSERT_TRUE(loaded.has_value());

    EXPECT_EQ(loaded->width, original.width);
    EXPECT_EQ(loaded->height, original.height);
    EXPECT_EQ(loaded->channels, original.channels);
    EXPECT_EQ(loaded->pixels.size(), original.pixels.size());

    std::remove(tmpPath);
}

TEST(ImpIo, ReadNonexistentFile) {
    ImageReader<PixelRGB_F> reader;
    auto result = reader.read("/tmp/nonexistent_image_12345.hdr");
    EXPECT_FALSE(result.has_value());
}

TEST(ImpIo, WriteEmptyImage) {
    ImageWriter<PixelRGB_F> writer;
    ImageDataRGB empty;
    EXPECT_FALSE(writer.write(empty, "/tmp/should_not_exist.hdr"));
}

TEST(ImpIo, WriteUnsupportedFormat) {
    ImageWriter<PixelRGB_F> writer;
    ImageDataRGB img = makeSyntheticRgbImage(2, 2);
    EXPECT_FALSE(writer.write(img, "/tmp/test.png"));
}

TEST(ImpIo, ReadsPngAsNormalizedFloat) {
    ImageReader<PixelRGBA_F> reader;
    const auto image = reader.read(pngFixture("rgba16.png").string());
    ASSERT_TRUE(image);
    ASSERT_EQ(image->pixels.size(), 4u);
    EXPECT_EQ(image->sourceFormat, ImageFormat::PNG);
    EXPECT_EQ(image->sourceDepth, SampleDepth::Bits16);
    EXPECT_EQ(image->width, 2);
    EXPECT_EQ(image->height, 2);
    EXPECT_FLOAT_EQ(image->pixels[0].r, 0.0f);
    EXPECT_FLOAT_EQ(image->pixels[0].g, 1.0f / 65535.0f);
    EXPECT_FLOAT_EQ(image->pixels[0].b, 257.0f / 65535.0f);
    EXPECT_FLOAT_EQ(image->pixels[0].a, 0.0f);
    EXPECT_FLOAT_EQ(image->pixels[1].a, 32768.0f / 65535.0f);
    EXPECT_FLOAT_EQ(image->pixels[2].a, 1.0f);
}

TEST(ImpIo, PreservesPngDepthSamplesAndMetadata) {
    const auto output = tempFile("imp_io_png_metadata.png");
    std::filesystem::remove(output);

    ImageReader<PixelRGB_F> reader;
    const auto image = reader.read(pngFixture("color_orientation.png").string());
    ASSERT_TRUE(image);
    ASSERT_EQ(image->sourceFormat, ImageFormat::PNG);
    ASSERT_EQ(image->sourceDepth, SampleDepth::Bits16);
    ASSERT_EQ(image->metadata.orientation, Orientation::Rotate90Clockwise);

    ImageWriter<PixelRGB_F> writer;
    ASSERT_TRUE(writer.write(*image, output.string()));

    const PngCodec codec;
    const auto original = codec.decode(pngFixture("color_orientation.png"));
    const auto restored = codec.decode(output);
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(original));
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(restored));
    const auto& expected = std::get<DecodedImage>(original);
    const auto& actual = std::get<DecodedImage>(restored);
    EXPECT_EQ(actual.bitDepth(), expected.bitDepth());
    EXPECT_EQ(actual.samples(), expected.samples());
    EXPECT_EQ(actual.metadata().orientation, expected.metadata().orientation);
    ASSERT_TRUE(actual.metadata().pngColor);
    ASSERT_TRUE(expected.metadata().pngColor);
    EXPECT_EQ(actual.metadata().pngColor->gamma, expected.metadata().pngColor->gamma);
    EXPECT_EQ(actual.metadata().pngColor->srgbIntent,
              expected.metadata().pngColor->srgbIntent);

    std::filesystem::remove(output);
}

TEST(ImpIo, DetectsPngFromContents) {
    const auto disguised = tempFile("imp_io_png_signature.bin");
    std::filesystem::copy_file(pngFixture("rgb8.png"), disguised,
                               std::filesystem::copy_options::overwrite_existing);

    ImageReader<PixelRGB_F> reader;
    const auto image = reader.read(disguised.string());
    ASSERT_TRUE(image);
    EXPECT_EQ(image->sourceFormat, ImageFormat::PNG);
    EXPECT_EQ(image->sourceDepth, SampleDepth::Bits8);

    std::filesystem::remove(disguised);
}

TEST(ImpIo, ValidatesPngFloatOutput) {
    const auto output = tempFile("imp_io_png_float_output.png");
    std::filesystem::remove(output);

    ImageReader<PixelRGB_F> reader;
    auto image = reader.read(pngFixture("rgb8.png").string());
    ASSERT_TRUE(image);

    ImageWriter<PixelRGB_F> writer;
    EXPECT_FALSE(writer.write(*image, tempFile("imp_io_wrong_format.hdr").string()));

    image->pixels[0] = {-1.0f, 2.0f, 0.5f};
    ASSERT_TRUE(writer.write(*image, output.string()));
    const auto decoded = PngCodec{}.decode(output);
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(decoded));
    const auto& samples = std::get<std::vector<std::uint8_t>>(
        std::get<DecodedImage>(decoded).samples());
    EXPECT_EQ(samples[0], 0);
    EXPECT_EQ(samples[1], 255);
    EXPECT_EQ(samples[2], 128);

    image->pixels[0].r = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(writer.write(*image, output.string()));

    std::filesystem::remove(output);
}
