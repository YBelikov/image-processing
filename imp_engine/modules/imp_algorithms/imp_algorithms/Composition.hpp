#ifndef Blend_hpp
#define Blend_hpp

#include "imp_io/ImageData.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace imp_algorithms {

enum class BlendMode {
    Over,
    Normal,
    Add,
    Multiply,
    Screen,
    Difference
};

imp_io::ImageDataRGBA blendImages(const imp_io::ImageDataRGBA& src, const imp_io::ImageDataRGBA& dst);

} // namespace imp_algorithms

#endif /* Blend_hpp */
