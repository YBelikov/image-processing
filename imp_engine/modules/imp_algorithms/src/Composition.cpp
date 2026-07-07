//
//  Blend.cpp
//  imp_engine
//
//  Created by Yuriy Belikov  on 08.06.2026.
//

#include <stdio.h>
#include <cassert>
#include "imp_algorithms/Composition.hpp"

namespace imp_algorithms {

using namespace imp_io;

inline PixelRGBA_F convertPixel(const PixelRGBA_F& src) {
    return {src.r * src.a, src.g * src.a, src.b * src.a, src.a};
}


inline PixelRGBA_F revertConversion(const PixelRGBA_F& src) {
    if (src.a <= 0.f) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    return {src.r / src.a, src.g  / src.a, src.b / src.a, src.a};
}

PixelRGBA_F blendPixels(const PixelRGBA_F& src, const PixelRGBA_F& dst, BlendMode blendMode) {
    PixelRGBA_F convertedSrcPixel = convertPixel(src);
    PixelRGBA_F convertedDstPixel = convertPixel(dst);
    auto over = [](float top, float bottom, float topAlpha) {
        return top + (1 - topAlpha) * bottom;
    };
    switch (blendMode) {
        case BlendMode::Over:
            return revertConversion({over(convertedSrcPixel.r, convertedDstPixel.r, convertedSrcPixel.a), over(convertedSrcPixel.g, convertedDstPixel.g, src.a), over(convertedSrcPixel.b, convertedDstPixel.b, src.a), (convertedSrcPixel.a + (1 - convertedSrcPixel.a) * convertedDstPixel.a)});
        default:
            return revertConversion({over(convertedSrcPixel.r, convertedDstPixel.r, convertedSrcPixel.a), over(convertedSrcPixel.g, convertedDstPixel.g, src.a), over(convertedSrcPixel.b, convertedDstPixel.b, src.a), (convertedSrcPixel.a + (1 - convertedSrcPixel.a) * convertedDstPixel.a)});
    }
}

ImageDataRGBA blendImages(const ImageDataRGBA& src, const ImageDataRGBA& dst) {
    assert(src.width == dst.width && src.height == dst.height);
    ImageDataRGBA res(src.width, src.height);
    for (int i = 0; i < src.area(); ++i) {
        res.pixels[i] = blendPixels(src.pixels[i], dst.pixels[i], BlendMode::Over);
    }
    return res;
}

}
