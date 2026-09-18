#ifndef IMP_IO_PNG_CODEC_HPP
#define IMP_IO_PNG_CODEC_HPP

#include "imp_io/ImageCodec.hpp"

namespace imp_io {

class PngCodec final : public ImageCodec {
public:
    PngCodec() : ImageCodec(ImageFormat::PNG) {}

private:
    DecodeResult decodeFile(const std::filesystem::path& path) const override;
    EncodeResult encodeFile(const std::filesystem::path& path, const DecodedImage& image,
                            const EncodeOptions& options) const override;
};

} // namespace imp_io

#endif
