//
//  ImageReader.hpp
//  imp_io
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#ifndef ImageReader_hpp
#define ImageReader_hpp

#include "imp_io/ImageData.hpp"
#include "stb_image.h"

#include <optional>
#include <string>
#include <string_view>

namespace imp_io {

template <typename PixelType>
class ImageReader {
public:
    
  /// Reads an image from disk and returns float pixel data.
  /// Returns std::nullopt on failure (file not found, unsupported format, etc.)
    std::optional<ImageData<PixelType>> read(std::string_view path) const {
        std::string pathStr(path);

        int width = 0;
        int height = 0;
        int channelsInFile = 0;
        constexpr int desiredChannels = PixelChannels<PixelType>::value;

        float* data = stbi_loadf(
            pathStr.c_str(),
            &width,
            &height,
            &channelsInFile,
            desiredChannels
        );

        if (!data) {
            return std::nullopt;
        }

        ImageData<PixelType> image(width, height);

        for (std::size_t i = 0; i < image.pixels.size(); ++i) {
            const std::size_t base = i * desiredChannels;
            image.pixels[i] = pixelFromInterleaved(data + base, static_cast<PixelType*>(nullptr));
        }

        stbi_image_free(data);
        return image;
    }
};


} // namespace imp_io

#endif /* ImageReader_hpp */
