//
//  ImageReader.hpp
//  imp_io
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#ifndef ImageReader_hpp
#define ImageReader_hpp

#include "imp_io/ImageData.hpp"
#include <optional>
#include <string_view>

namespace imp_io {

class ImageReader {
public:
  /// Reads an image from disk and returns RGBA pixel data.
  /// Returns std::nullopt on failure (file not found, unsupported format, etc.)
  std::optional<ImageData> read(std::string_view path) const;
};

} // namespace imp_io

#endif /* ImageReader_hpp */
