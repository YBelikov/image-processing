#ifndef IMP_IO_IMAGE_CODEC_HPP
#define IMP_IO_IMAGE_CODEC_HPP

#include "imp_io/CodecResult.hpp"
#include "imp_io/ImageFormat.hpp"

#include <filesystem>

namespace imp_io {

inline constexpr int defaultEncodeQuality = 90;
inline constexpr int minEncodeQuality = 1;
inline constexpr int maxEncodeQuality = 100;

struct EncodeOptions {
    // JPEG only. PNG ignores quality. Depth always comes from the image.
    int quality = defaultEncodeQuality;
};

// File-based contract for future codecs; no codec-library types cross this API.
class ImageCodec {
public:
    virtual ~ImageCodec() = default;

    ImageFormat format() const noexcept { return format_; }

    // Samples keep their precision and stored orientation; no color correction.
    [[nodiscard]] DecodeResult decode(const std::filesystem::path& path) const;

    // Validate before backend I/O. Success means writing and closing succeeded.
    [[nodiscard]] EncodeResult encode(const std::filesystem::path& path,
                                      const DecodedImage& image,
                                      const EncodeOptions& options = {}) const;

protected:
    explicit ImageCodec(ImageFormat format);

private:
    EncodeResult validateEncoding(const DecodedImage& image,
                                  const EncodeOptions& options) const;

    // Implementations return expected file/codec failures as CodecError.
    virtual DecodeResult decodeFile(const std::filesystem::path& path) const = 0;
    virtual EncodeResult encodeFile(const std::filesystem::path& path,
                                    const DecodedImage& image,
                                    const EncodeOptions& options) const = 0;

    const ImageFormat format_;
};

} // namespace imp_io

#endif
