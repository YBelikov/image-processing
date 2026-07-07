//
//  ImageWriter.hpp
//  imp_io
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#ifndef ImageWriter_hpp
#define ImageWriter_hpp

#include "imp_io/ImageData.hpp"
#include "stb_image_write.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace imp_io {

template <typename PixelType> class ImageWriter {
  public:

    /// Writes image data to disk. Format is determined by file extension.
    /// Supported: .png, .jpg/.jpeg, .bmp
    /// Returns true on success.
    [[nodiscard("Always check the write status")]]
    bool write(const ImageData<PixelType>& image, std::string_view path) const {
        if (image.empty()) {
            return false;
        }
        std::string pathStr(path);
        std::string ext = getExtension(path);
        std::vector<float> flat;
        flat.reserve(static_cast<std::size_t>(image.width) * image.height * image.channels);

        for (const auto& p : image.pixels) {
            appendInterleaved(flat, p);
        }
        if (ext == ".hdr") {
            return stbi_write_hdr(pathStr.c_str(), image.width, image.height,
                                  image.channels, flat.data());
        }
        // Unsupported format
        return false;
    }
    
    /// Extract lowercase file extension from path (e.g. ".png")
    std::string getExtension(std::string_view path) const {
        auto dot = path.rfind('.');
        if (dot == std::string_view::npos) {
            return "";
        }
        std::string ext(path.substr(dot));
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return ext;
    }
};

} // namespace imp_io

#endif /* ImageWriter_hpp */
