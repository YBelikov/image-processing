#include "imp_algorithms/Composition.hpp"

#include <gtest/gtest.h>

using namespace imp_algorithms;
using namespace imp_io;

TEST(Composition, SourceOver) {
    ImageDataRGBA src(4, 2);
    ImageDataRGBA dst(2, 3);

    for (int i = 0; i < src.area(); ++i) {
        src.pixels[i] = {i / 8.0f, 0.0f, 0.0f, 0.5f};
    }
    for (int i = 0; i < dst.area(); ++i) {
        dst.pixels[i] = {0.0f, 0.0f, i / 6.0f, 0.5f};
    }

    auto result = composeImages(src, dst, PorterDuffOperator::SourceOver);

    ASSERT_EQ(result.width, 2);
    ASSERT_EQ(result.height, 2);
    ASSERT_EQ(result.pixels.size(), 4);

    // Each overlapping pixel must match composition of the same pixel pair alone.
    for (int y = 0; y < result.height; ++y) {
        for (int x = 0; x < result.width; ++x) {
            ImageDataRGBA srcPixel(1, 1);
            ImageDataRGBA dstPixel(1, 1);
            srcPixel.pixels[0] = src.pixels[y * src.width + x];
            dstPixel.pixels[0] = dst.pixels[y * dst.width + x];
            const auto expected = composeImages(srcPixel, dstPixel, PorterDuffOperator::SourceOver).pixels[0];
            const auto& actual = result.pixels[y * result.width + x];
            EXPECT_FLOAT_EQ(actual.r, expected.r);
            EXPECT_FLOAT_EQ(actual.g, expected.g);
            EXPECT_FLOAT_EQ(actual.b, expected.b);
            EXPECT_FLOAT_EQ(actual.a, expected.a);
        }
    }
}
