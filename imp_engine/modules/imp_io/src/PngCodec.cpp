#include "imp_io/PngCodec.hpp"
#include "png/PngDriver.hpp"

namespace imp_io {

DecodeResult PngCodec::decodeFile(const std::filesystem::path& path) const {
    return detail::readPng(path);
}

EncodeResult PngCodec::encodeFile(const std::filesystem::path& path, const DecodedImage& image,
                                 const EncodeOptions&) const {
    return detail::writePng(path, image);
}

} // namespace imp_io
