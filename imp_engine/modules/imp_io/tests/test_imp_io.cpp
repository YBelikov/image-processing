#include "imp_io/ImageData.hpp"
#include "imp_io/ImageReader.hpp"
#include "imp_io/ImageWriter.hpp"

#include <cstdio>
#include <gtest/gtest.h>

using namespace imp_io;

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
