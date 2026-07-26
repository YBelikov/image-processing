//
//  Composition.cpp
//  imp_engine
//
//  Created by Yuriy Belikov  on 08.06.2026.
//

#include <stdio.h>
#include <cassert>
#include "imp_algorithms/Composition.hpp"

namespace imp_algorithms {

using namespace imp_io;

inline PixelRGBA_F premultiply(const PixelRGBA_F& src) {
    return {src.r * src.a, src.g * src.a, src.b * src.a, src.a};
}


inline PixelRGBA_F unpremultiply(const PixelRGBA_F& src) {
    if (src.a <= 0.f) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    return {src.r / src.a, src.g  / src.a, src.b / src.a, src.a};
}

PixelRGBA_F composePixels(const PixelRGBA_F& src, const PixelRGBA_F& dst, PorterDuffOperator compositionOperator) {
    PixelRGBA_F convertedSrcPixel = premultiply(src);
    PixelRGBA_F convertedDstPixel = premultiply(dst);
    auto over = [](const PixelRGBA_F& top, const PixelRGBA_F& bottom) -> PixelRGBA_F {
        PixelRGBA_F res;
        res.r = top.r + (1 - top.a) * bottom.r;
        res.g = top.g + (1 - top.a) * bottom.g;
        res.b = top.b + (1 - top.a) * bottom.b;
        res.a = (1 - top.a) * bottom.a; 
        return res;
    };

    auto sourceIn = [](const PixelRGBA_F& top, const PixelRGBA_F& bottom) -> PixelRGBA_F {
        PixelRGBA_F res;
        res.r = top.r * bottom.a;
        res.g = top.g * bottom.a;
        res.b = top.b * bottom.a;
        res.a = top.a * bottom.a; 
        return res;
    };

     auto sourceOut  = [](const PixelRGBA_F& top, const PixelRGBA_F& bottom) -> PixelRGBA_F {
        PixelRGBA_F res;
        res.r = top.r * (1 - bottom.a);
        res.g = top.g * (1 - bottom.a);
        res.b = top.b * (1 - bottom.a);
        res.a = top.a * (1 - bottom.a); 
        return res;
    };

    auto sourceAtop  = [](const PixelRGBA_F& top, const PixelRGBA_F& bottom) -> PixelRGBA_F {
        PixelRGBA_F res;
        res.r = top.r * bottom.a + (1 - top.a) * bottom.r;
        res.g = top.g * bottom.a + (1 - top.a) * bottom.g;
        res.b = top.b * bottom.a + (1 - top.a) * bottom.b;
        res.a = bottom.a; 
        return res;
    };

    auto xorPixels = [](const PixelRGBA_F& top, const PixelRGBA_F& bottom) -> PixelRGBA_F {
        PixelRGBA_F res;
        res.r = (1 - bottom.a) * top.r + (1 - top.a) * bottom.r;
        res.g = (1 - bottom.a) * top.g + (1 - top.a) * bottom.g;
        res.r = (1 - bottom.a) * top.b + (1 - top.a) * bottom.b;
        res.a = (1 - bottom.a) * top.a + (1 - top.a) * bottom.a;
    };

    switch (compositionOperator) {
        case PorterDuffOperator::SourceOver:
            return unpremultiply(over(convertedSrcPixel, convertedDstPixel));
        case PorterDuffOperator::SourceIn:
            return unpremultiply(sourceIn(convertedSrcPixel, convertedDstPixel));
        case PorterDuffOperator::SourceOut:
            return unpremultiply(sourceOut(convertedSrcPixel, convertedDstPixel));
        case PorterDuffOperator::SourceAtop:
            return unpremultiply(sourceAtop(convertedSrcPixel, convertedDstPixel));
        case PorterDuffOperator::Xor:
            return unpremultiply(xorPixels(convertedSrcPixel, convertedDstPixel));
        default:
            return unpremultiply(over(convertedSrcPixel, convertedDstPixel));
        }
}

ImageDataRGBA composeImages(const ImageDataRGBA& src, const ImageDataRGBA& dst) {
    assert(src.width == dst.width && src.height == dst.height);
    ImageDataRGBA res(src.width, src.height);
    for (int i = 0; i < src.area(); ++i) {
        res.pixels[i] = composePixels(src.pixels[i], dst.pixels[i], PorterDuffOperator::SourceOver);
    }
    return res;
}

}
