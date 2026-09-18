#ifndef IMP_IO_DECODED_IMAGE_HPP
#define IMP_IO_DECODED_IMAGE_HPP

#include "imp_io/ImageMetadata.hpp"

#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace imp_io {

enum class ChannelLayout : std::uint8_t { RGB = 3, RGBA = 4 };
enum class StorageType { UInt8, UInt16, Float32 };
enum class SampleDepth : std::uint8_t { Bits8 = 8, Bits16 = 16, Bits32 = 32 };

using SampleBuffer = std::variant<
    std::vector<std::uint8_t>,
    std::vector<std::uint16_t>,
    std::vector<float>>;

// Owned, interleaved rows without padding. Samples are neither normalized nor
// color-corrected; orientation is unapplied. RGBA alpha is straight (unassociated).
class DecodedImage {
public:
    // Throws invalid_argument for invalid data and overflow_error for size overflow.
    DecodedImage(std::size_t width, std::size_t height, ChannelLayout layout,
                 SampleDepth depth, SampleBuffer samples, ImageMetadata metadata = {});

    std::size_t width() const noexcept { return width_; }
    std::size_t height() const noexcept { return height_; }
    ChannelLayout layout() const noexcept { return layout_; }
    SampleDepth bitDepth() const noexcept { return depth_; }
    StorageType storageType() const noexcept;
    const SampleBuffer& samples() const noexcept { return samples_; }
    const ImageMetadata& metadata() const noexcept { return metadata_; }

private:
    void validate() const;

    std::size_t width_;
    std::size_t height_;
    ChannelLayout layout_;
    SampleDepth depth_;
    SampleBuffer samples_;
    ImageMetadata metadata_;
};

} // namespace imp_io

#endif
