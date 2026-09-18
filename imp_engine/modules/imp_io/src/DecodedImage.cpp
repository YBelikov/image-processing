#include "imp_io/DecodedImage.hpp"

#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace imp_io {
namespace {

static_assert(sizeof(float) == sizeof(std::uint32_t)
              && std::numeric_limits<float>::is_iec559,
              "DecodedImage requires IEEE 754 binary32 float");

std::size_t checkedMultiply(std::size_t left, std::size_t right) {
    if (right != 0 && left > std::numeric_limits<std::size_t>::max() / right) {
        throw std::overflow_error("Decoded image buffer size overflows size_t");
    }

    return left * right;
}

bool supportsDepth(StorageType storage, SampleDepth depth) {
    switch (storage) {
        case StorageType::UInt8:
            return depth == SampleDepth::Bits8;
        case StorageType::UInt16:
            return depth == SampleDepth::Bits16;
        case StorageType::Float32:
            return depth == SampleDepth::Bits32;
    }

    return false;
}

} // namespace

DecodedImage::DecodedImage(std::size_t width, std::size_t height, ChannelLayout layout,
                           SampleDepth depth, SampleBuffer samples, ImageMetadata metadata)
    : width_(width), height_(height), layout_(layout), depth_(depth),
      samples_(std::move(samples)), metadata_(std::move(metadata)) {
    validate();
}

StorageType DecodedImage::storageType() const noexcept {
    if (std::holds_alternative<std::vector<std::uint8_t>>(samples_)) {
        return StorageType::UInt8;
    }

    if (std::holds_alternative<std::vector<std::uint16_t>>(samples_)) {
        return StorageType::UInt16;
    }

    return StorageType::Float32;
}

void DecodedImage::validate() const {
    if (width_ == 0 || height_ == 0) {
        throw std::invalid_argument("Decoded image dimensions must be nonzero");
    }

    if (layout_ != ChannelLayout::RGB && layout_ != ChannelLayout::RGBA) {
        throw std::invalid_argument("Decoded image layout must be RGB or RGBA");
    }

    if (!supportsDepth(storageType(), depth_)) {
        throw std::invalid_argument("Unsupported decoded image storage/depth combination");
    }

    // Check every product before comparing lengths, including the byte count.
    const auto pixels = checkedMultiply(width_, height_);
    const auto count = checkedMultiply(pixels, static_cast<std::size_t>(layout_));
    std::visit([count](const auto& buffer) {
        using Sample = typename std::decay_t<decltype(buffer)>::value_type;
        checkedMultiply(count, sizeof(Sample));

        if (buffer.size() != count) {
            throw std::invalid_argument("Decoded image sample count does not match dimensions/layout");
        }
    }, samples_);
}

} // namespace imp_io
