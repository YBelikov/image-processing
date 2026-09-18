#ifndef IMP_IO_PNG_DRIVER_HPP
#define IMP_IO_PNG_DRIVER_HPP

#include "imp_io/CodecResult.hpp"

#include <filesystem>

namespace imp_io::detail {

DecodeResult readPng(const std::filesystem::path& path);
EncodeResult writePng(const std::filesystem::path& path, const DecodedImage& image);

} // namespace imp_io::detail

#endif
