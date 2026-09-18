#ifndef IMP_IO_JPEG_DRIVER_HPP
#define IMP_IO_JPEG_DRIVER_HPP

#include "imp_io/CodecResult.hpp"

#include <filesystem>

namespace imp_io::detail {

DecodeResult readJpeg(const std::filesystem::path& path);
EncodeResult writeJpeg(const std::filesystem::path& path, const DecodedImage& image,
                       int quality);

} // namespace imp_io::detail

#endif
