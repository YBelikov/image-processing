//
//  ImageData.hpp
//  imp_io
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#ifndef ImageData_hpp
#define ImageData_hpp

#include <cstdint>
#include <vector>
#include "Pixel.hpp"

namespace imp_io {

template<typename T>
struct _ImageData {
    int width = 0;
    int height = 0;
    int channels = 3; // RGB
    std::vector<float> pixels;
    bool empty() const noexcept { return pixels.empty(); }
    std::size_t stride() const noexcept {
        return static_cast<std::size_t>(width) * channels;
    }
    _ImageData() = default;
    _ImageData(int _width, int _height, int _channels) : width{_width}, height{_height}, channels{_channels} {
        pixels.resize(width * height * channels);
    }
    
};

using ImageData = _ImageData<float>;

} // namespace imp_io

#endif /* ImageData_hpp */
