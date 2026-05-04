//
//  ImageWriter.cpp
//  imp_io
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#include "imp_io/ImageWriter.hpp"

#include "stb_image_write.h"

#include <algorithm>
#include <string>
#include <string_view>

namespace imp_io {

namespace {

/// Extract lowercase file extension from path (e.g. ".png")
std::string getExtension(std::string_view path) {
    auto dot = path.rfind('.');
    if (dot == std::string_view::npos) {
        return "";
    }
    std::string ext(path.substr(dot));
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext;
}

} // namespace

bool ImageWriter::write(const ImageData &image, std::string_view path) const {
    if (image.empty()) {
        return false;
    }

    std::string pathStr(path);
    std::string ext = getExtension(path);

    int stride = static_cast<int>(image.stride());
//
//    if (ext == ".png") {
//        return stbi_write_png(pathStr.c_str(), image.width, image.height,
//                              image.channels, image.pixels.data(), stride) != 0;
//    }
//
//    if (ext == ".jpg" || ext == ".jpeg") {
//        constexpr int jpegQuality = 95;
//        return stbi_write_jpg(pathStr.c_str(), image.width, image.height,
//                              image.channels, image.pixels.data(),
//                              jpegQuality) != 0;
//    }
//
//    if (ext == ".bmp") {
//        return stbi_write_bmp(pathStr.c_str(), image.width, image.height,
//                              image.channels, image.pixels.data()) != 0;
//    }
    
    if (ext == ".hdr") {
        return stbi_write_hdr(pathStr.c_str(), image.width, image.height, image.channels, image.pixels.data());
    }

    // Unsupported format
    return false;
}

} // namespace imp_io
