//
//  ImageReader.cpp
//  imp_io
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#include "imp_io/ImageReader.hpp"

#include "stb_image.h"

#include <string>

namespace imp_io {

std::optional<ImageData> ImageReader::read(std::string_view path) const {
  // stbi_load requires a null-terminated string
  std::string pathStr(path);

  int width = 0, height = 0, channelsInFile = 0;
  constexpr int desiredChannels = 3; // always request RGBA

  float *data = stbi_loadf(pathStr.c_str(), &width, &height,
                                  &channelsInFile, desiredChannels);

  if (!data) {
    return std::nullopt;
  }

  const std::size_t dataSize =
      static_cast<std::size_t>(width) * height * desiredChannels;

  ImageData image;
  image.width = width;
  image.height = height;
  image.channels = desiredChannels;
  image.pixels.assign(data, data + dataSize);

  stbi_image_free(data);

  return image;
}

} // namespace imp_io
