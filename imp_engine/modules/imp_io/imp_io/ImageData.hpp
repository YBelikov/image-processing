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

namespace imp_io {

struct ImageData {
  int width = 0;
  int height = 0;
  int channels = 4; // RGBA

  std::vector<uint8_t> pixels;

  bool empty() const noexcept { return pixels.empty(); }

  std::size_t stride() const noexcept {
    return static_cast<std::size_t>(width) * channels;
  }
};

} // namespace imp_io

#endif /* ImageData_hpp */
