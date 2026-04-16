//
//  ImageWriter.hpp
//  imp_io
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#ifndef ImageWriter_hpp
#define ImageWriter_hpp

#include "imp_io/ImageData.hpp"
#include <string_view>

namespace imp_io {

class ImageWriter {
public:
  /// Writes image data to disk. Format is determined by file extension.
  /// Supported: .png, .jpg/.jpeg, .bmp
  /// Returns true on success.
  bool write(const ImageData &image, std::string_view path) const;
};

} // namespace imp_io

#endif /* ImageWriter_hpp */
