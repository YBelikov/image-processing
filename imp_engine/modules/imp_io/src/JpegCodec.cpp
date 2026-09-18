#include "imp_io/JpegCodec.hpp"
#include "jpeg/JpegDriver.hpp"

namespace imp_io {

DecodeResult JpegCodec::decodeFile(const std::filesystem::path& path) const {
    return detail::readJpeg(path);
}

EncodeResult JpegCodec::encodeFile(const std::filesystem::path& path,
                                  const DecodedImage& image,
                                  const EncodeOptions& options) const {
    return detail::writeJpeg(path, image, options.quality);
}

} // namespace imp_io
