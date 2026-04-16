#include "imp_io/ImageData.hpp"
#include "imp_io/ImageReader.hpp"
#include "imp_io/ImageWriter.hpp"
#include <cstdio>
#include <gtest/gtest.h>

using namespace imp_io;

// ── Helper: create a small synthetic RGBA image ──────────────

static ImageData makeSyntheticImage(int width, int height) {
  ImageData img;
  img.width = width;
  img.height = height;
  img.channels = 4;
  img.pixels.resize(static_cast<std::size_t>(width) * height * 4);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      std::size_t idx = (static_cast<std::size_t>(y) * width + x) * 4;
      img.pixels[idx + 0] = static_cast<uint8_t>(x % 256);       // R
      img.pixels[idx + 1] = static_cast<uint8_t>(y % 256);       // G
      img.pixels[idx + 2] = static_cast<uint8_t>((x + y) % 256); // B
      img.pixels[idx + 3] = 255;                                 // A
    }
  }
  return img;
}

// ── ImageData ────────────────────────────────────────────────

TEST(ImpIo, ImageDataEmpty) {
  ImageData img;
  EXPECT_TRUE(img.empty());
  EXPECT_EQ(img.channels, 4);
}

TEST(ImpIo, ImageDataNotEmpty) {
  ImageData img = makeSyntheticImage(2, 2);
  EXPECT_FALSE(img.empty());
  EXPECT_EQ(img.width, 2);
  EXPECT_EQ(img.height, 2);
  EXPECT_EQ(img.stride(), 8u);
  EXPECT_EQ(img.pixels.size(), 16u);
}

// ── PNG round-trip ───────────────────────────────────────────

TEST(ImpIo, PngRoundTrip) {
  const char *tmpPath = "/tmp/imp_io_test_roundtrip.png";

  ImageData original = makeSyntheticImage(8, 8);

  ImageWriter writer;
  ASSERT_TRUE(writer.write(original, tmpPath));

  ImageReader reader;
  auto loaded = reader.read(tmpPath);
  ASSERT_TRUE(loaded.has_value());

  EXPECT_EQ(loaded->width, original.width);
  EXPECT_EQ(loaded->height, original.height);
  EXPECT_EQ(loaded->channels, original.channels);
  EXPECT_EQ(loaded->pixels, original.pixels);

  std::remove(tmpPath);
}

// ── BMP round-trip ───────────────────────────────────────────

TEST(ImpIo, BmpRoundTrip) {
  const char *tmpPath = "/tmp/imp_io_test_roundtrip.bmp";

  ImageData original = makeSyntheticImage(4, 4);

  ImageWriter writer;
  ASSERT_TRUE(writer.write(original, tmpPath));

  ImageReader reader;
  auto loaded = reader.read(tmpPath);
  ASSERT_TRUE(loaded.has_value());

  EXPECT_EQ(loaded->width, original.width);
  EXPECT_EQ(loaded->height, original.height);
  EXPECT_EQ(loaded->pixels, original.pixels);

  std::remove(tmpPath);
}

// ── JPEG round-trip (lossy — check dimensions only) ─────────

TEST(ImpIo, JpegRoundTrip) {
  const char *tmpPath = "/tmp/imp_io_test_roundtrip.jpg";

  ImageData original = makeSyntheticImage(16, 16);

  ImageWriter writer;
  ASSERT_TRUE(writer.write(original, tmpPath));

  ImageReader reader;
  auto loaded = reader.read(tmpPath);
  ASSERT_TRUE(loaded.has_value());

  // JPEG is lossy, so we only verify dimensions
  EXPECT_EQ(loaded->width, original.width);
  EXPECT_EQ(loaded->height, original.height);
  EXPECT_EQ(loaded->channels, 4);
  EXPECT_FALSE(loaded->empty());

  std::remove(tmpPath);
}

// ── Error cases ──────────────────────────────────────────────

TEST(ImpIo, ReadNonexistentFile) {
  ImageReader reader;
  auto result = reader.read("/tmp/nonexistent_image_12345.png");
  EXPECT_FALSE(result.has_value());
}

TEST(ImpIo, WriteEmptyImage) {
  ImageWriter writer;
  ImageData empty;
  EXPECT_FALSE(writer.write(empty, "/tmp/should_not_exist.png"));
}

TEST(ImpIo, WriteUnsupportedFormat) {
  ImageWriter writer;
  ImageData img = makeSyntheticImage(2, 2);
  EXPECT_FALSE(writer.write(img, "/tmp/test.heic"));
}
