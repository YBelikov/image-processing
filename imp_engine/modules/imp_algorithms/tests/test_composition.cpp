#include "imp_algorithms/Composition.hpp"
#include "imp_io/ImageWriter.hpp"

#include <gtest/gtest.h>

using namespace imp_algorithms;
using namespace imp_io;

TEST(Composition, SourceOver) {
    ImageDataRGBA src(1500, 1500);
    ImageDataRGBA dst(1500, 1500);

    for (int i = 0; i < src.area(); ++i) {
        src.pixels[i] = {1.0f, 0.0f, 0.0f, 1.f};
        dst.pixels[i] = {0.0f, 0.0f, 1.0f, 1.f};
    }

    auto result = composeImages(src, dst);

    ImageWriter<PixelRGBA_F> writer;
    ASSERT_TRUE(writer.write(src, "/tmp/imp_composition_src.hdr"));
    ASSERT_TRUE(writer.write(dst, "/tmp/imp_composition_dst.hdr"));
    ASSERT_TRUE(writer.write(result, "/tmp/imp_composition_result.hdr"));

    ASSERT_EQ(result.width, 1500);
    ASSERT_EQ(result.height, 1500);
    ASSERT_EQ(result.pixels.size(), 1500 * 1500);
}
