//
//  ImageProcessor.hpp
//  imp_engine
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#ifndef ImageProcessor_hpp
#define ImageProcessor_hpp

#include "imp_io/ImageData.hpp"
#include "imp_io/ImageWriter.hpp"
#include "imp_io/ImageReader.hpp"

#include <string_view>
#include <iostream>

namespace imp_engine {

template<typename PixelType>
class ImageProcessor {
  public:
    /// Load an image from disk into the internal pixel buffer.
    /// Returns true on success.
    bool setup(std::string_view imagePath) {
      imp_io::ImageReader<PixelType> reader;
      auto result = reader.read(imagePath);
      if (!result.has_value()) {
          std::cerr << "ImageProcessor: failed to load image: " << imagePath
                    << std::endl;
          return false;
      }
      imageData_ = std::move(result.value());
      return true;
    }

    /// Apply image processing algorithms (currently a stub).
    void render() { return; }

    /// Write the current pixel buffer to disk.
    /// Output format is determined by file extension.
    /// Returns true on success.
    bool exportImage(std::string_view outputPath) {
      imp_io::ImageWriter<PixelType> writer;
      bool success = writer.write(imageData_, outputPath);

      if (success) {
          std::cout << "ImageProcessor: exported to " << outputPath << std::endl;
      } else {
          std::cerr << "ImageProcessor: failed to export to " << outputPath
                    << std::endl;
      }
      return success;
    }

    imp_io::ImageData<PixelType> blendLayers(const imp_io::ImageData<PixelType> &bottom,
                                             const imp_io::ImageData<PixelType> &top) {
        return imp_io::ImageData<PixelType>();
    }

  private:
    imp_io::ImageData<PixelType> imageData_;
};

#endif /* ImageProcessor_hpp */
}
