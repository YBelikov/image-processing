//
//  ImageData.hpp
//  imp_io
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#ifndef ImageData_hpp
#define ImageData_hpp

#include "Pixel.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace imp_io {

template<typename T>
struct ImageData {
// interface
public:
    bool empty() const noexcept { return pixels.empty(); }
    std::size_t stride() const noexcept {
        return static_cast<std::size_t>(width) * channels;
    }
    int area() const noexcept { return width * height; }
    ImageData() = default;
    ImageData(int _width, int _height) : width{_width}, height{_height}, channels{PixelChannels<T>::value} {
        pixels.resize(static_cast<std::size_t>(width) * height);
    }
    
// data
public:
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<T> pixels;
};

using ImageDataRGB = ImageData<PixelRGB_F>;
using ImageDataRGBA = ImageData<PixelRGBA_F>;

} // namespace imp_io

#endif /* ImageData_hpp */
