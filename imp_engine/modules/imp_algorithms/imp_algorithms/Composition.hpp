#ifndef Composition_hpp
#define Composition_hpp

#include "imp_io/ImageData.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace imp_algorithms {

enum class PorterDuffOperator {
    Clear,
    Copy,
    Destination,
    SourceOver,
    SourceIn,
    SourceOut,
    SourceAtop,
    Xor
};

imp_io::ImageDataRGBA composeImages(const imp_io::ImageDataRGBA& src, const imp_io::ImageDataRGBA& dst);

} // namespace imp_algorithms

#endif /* Composition_hpp */
