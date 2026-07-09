//
//  Blend.cpp
//  imp_engine
//
//  Created by Yuriy Belikov  on 08.06.2026.
//

#include <stdio.h>
#include "imp_algorithms/Blend.hpp"

namespace imp_algorithms {

using namespace imp_io;

PixelRGBA_F convertPixel(const PixelRGBA_F& src) {
    return {src.r * src.a, src.g * src.a, src.b * src.a, src.a};
}


PixelRGBA_F revertConversion(const PixelRGBA_F& src) {
    return {src.r / src.a, src.g  / src.a, src.b / src.a, src.a};
}

PixelRGBA_F blendPixels(const PixelRGBA_F& src, const PixelRGBA_F& dst, BlendMode blendMode) {
    PixelRGBA_F convertedSrcPixel = convertPixel(src);
    PixelRGBA_F convertedDstPixel = convertPixel(dst);
    
}

ImageDataRGBA blendImages(const imp_io::ImageDataRGBA& src, const imp_io::ImageDataRGBA& dst);

}
