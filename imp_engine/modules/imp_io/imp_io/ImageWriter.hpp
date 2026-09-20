//
//  ImageWriter.hpp
//  imp_io
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#ifndef ImageWriter_hpp
#define ImageWriter_hpp

#include "imp_io/ImageData.hpp"
#include "imp_io/PngCodec.hpp"
#include "stb_image_write.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace imp_io {

template <typename PixelType> class ImageWriter {
  public:

    /// Writes sourced PNG data to PNG, or unsourced float data to HDR.
    /// Returns true on success.
    [[nodiscard("Always check the write status")]]
    bool write(const ImageData<PixelType>& image, std::string_view path) const {
        if (!valid(image)) {
            return false;
        }

        const auto extension = getExtension(path);
        if (image.sourceFormat == ImageFormat::PNG) {
            if (extension != ".png") {
                return false;
            }
            return writePng(image, path);
        }
        if (image.sourceFormat) {
            return false;
        }
        if (extension != ".hdr") {
            return false;
        }

        const std::string pathString(path);
        std::vector<float> flat;
        flat.reserve(static_cast<std::size_t>(image.width) * image.height * image.channels);

        for (const auto& pixel : image.pixels) {
            appendInterleaved(flat, pixel);
        }
        return stbi_write_hdr(pathString.c_str(), image.width, image.height,
                              image.channels, flat.data());
    }

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

private:
    static bool valid(const ImageData<PixelType>& image) {
        if (image.width <= 0 || image.height <= 0 || image.empty()) {
            return false;
        }

        constexpr auto expectedChannels = PixelChannels<PixelType>::value;
        const auto expectedPixels = static_cast<std::size_t>(image.width)
            * static_cast<std::size_t>(image.height);
        return image.channels == expectedChannels && image.pixels.size() == expectedPixels;
    }

    template<typename Sample>
    static std::optional<std::vector<Sample>> quantize(const ImageData<PixelType>& image) {
        const auto maximum = static_cast<double>(std::numeric_limits<Sample>::max());
        std::vector<Sample> samples;
        samples.reserve(image.pixels.size() * image.channels);

        std::vector<float> channels;
        channels.reserve(static_cast<std::size_t>(image.channels));
        for (const auto& pixel : image.pixels) {
            channels.clear();
            appendInterleaved(channels, pixel);
            for (const auto value : channels) {
                if (!std::isfinite(value)) {
                    return std::nullopt;
                }

                const auto clamped = std::clamp(static_cast<double>(value), 0.0, 1.0);
                samples.push_back(static_cast<Sample>(std::round(clamped * maximum)));
            }
        }
        return samples;
    }

    static bool writePng(const ImageData<PixelType>& image, std::string_view path) {
        if (!image.sourceDepth) {
            return false;
        }

        constexpr auto layout = PixelChannels<PixelType>::value == 3
            ? ChannelLayout::RGB : ChannelLayout::RGBA;
        static_assert(PixelChannels<PixelType>::value == 3
                      || PixelChannels<PixelType>::value == 4);
        try {
            std::optional<DecodedImage> encoded;
            switch (*image.sourceDepth) {
                case SampleDepth::Bits8: {
                    auto samples = quantize<std::uint8_t>(image);
                    if (!samples) {
                        return false;
                    }
                    encoded.emplace(image.width, image.height, layout, SampleDepth::Bits8,
                                    std::move(*samples), image.metadata);
                    break;
                }
                case SampleDepth::Bits16: {
                    auto samples = quantize<std::uint16_t>(image);
                    if (!samples) {
                        return false;
                    }
                    encoded.emplace(image.width, image.height, layout, SampleDepth::Bits16,
                                    std::move(*samples), image.metadata);
                    break;
                }
                case SampleDepth::Bits32:
                    return false;
                default:
                    return false;
            }

            const PngCodec codec;
            return std::holds_alternative<std::monostate>(
                codec.encode(std::string(path), *encoded));
        } catch (const std::invalid_argument&) {
            return false;
        } catch (const std::overflow_error&) {
            return false;
        }
    }
};

} // namespace imp_io

#endif /* ImageWriter_hpp */
