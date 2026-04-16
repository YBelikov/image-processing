//
//  ImageProcessor.hpp
//  imp_engine
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#ifndef ImageProcessor_hpp
#define ImageProcessor_hpp

#include "imp_io/ImageData.hpp"
#include <string_view>

class ImageProcessor {
public:
  /// Load an image from disk into the internal pixel buffer.
  /// Returns true on success.
  bool setup(std::string_view imagePath);

  /// Apply image processing algorithms (currently a stub).
  void render();

  /// Write the current pixel buffer to disk.
  /// Output format is determined by file extension.
  /// Returns true on success.
  bool exportImage(std::string_view outputPath);

private:
  imp_io::ImageData imageData_;
};

#endif /* ImageProcessor_hpp */
