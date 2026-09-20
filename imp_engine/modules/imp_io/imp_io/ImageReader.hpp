//
//  ImageReader.hpp
//  imp_io
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#ifndef ImageReader_hpp
#define ImageReader_hpp

#include "imp_io/ImageData.hpp"
#include "imp_io/PngCodec.hpp"
#include "stb_image.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace imp_io {

template <typename PixelType>
class ImageReader {
public:
    /// Reads an image from disk and returns float pixel data.
    /// Returns std::nullopt on failure (file not found, unsupported format, etc.)
    std::optional<ImageData<PixelType>> read(std::string_view path) const {
        const std::string pathString(path);
        const PngCodec png;
        auto decoded = png.decode(pathString);
        if (const auto* image = std::get_if<DecodedImage>(&decoded)) {
            return fromPng(*image);
        }

        const auto& error = std::get<CodecError>(decoded);
        if (error.code != CodecErrorCode::UnsupportedFormat) {
            return std::nullopt;
        }

        return readFloat(pathString);
    }

private:
    template<typename Sample>
    static bool copyPng(const DecodedImage& source, ImageData<PixelType>& output) {
        if constexpr (!std::is_same_v<Sample, std::uint8_t>
                      && !std::is_same_v<Sample, std::uint16_t>) {
            return false;
        } else {
            const auto& samples = std::get<std::vector<Sample>>(source.samples());
            const auto sourceChannels = static_cast<std::size_t>(source.layout());
            const auto maximum = static_cast<float>(std::numeric_limits<Sample>::max());

            for (std::size_t index = 0; index < output.pixels.size(); ++index) {
                const auto offset = index * sourceChannels;
                float channels[4]{
                    samples[offset] / maximum,
                    samples[offset + 1] / maximum,
                    samples[offset + 2] / maximum,
                    1.0f
                };
                if (source.layout() == ChannelLayout::RGBA) {
                    channels[3] = samples[offset + 3] / maximum;
                }

                output.pixels[index] = pixelFromInterleaved(
                    channels, static_cast<PixelType*>(nullptr));
            }
            return true;
        }
    }

    static std::optional<ImageData<PixelType>> fromPng(const DecodedImage& source) {
        ImageData<PixelType> output(static_cast<int>(source.width()),
                                    static_cast<int>(source.height()));
        const auto copied = std::visit([&source, &output](const auto& samples) {
            using Sample = typename std::decay_t<decltype(samples)>::value_type;
            return copyPng<Sample>(source, output);
        }, source.samples());
        if (!copied) {
            return std::nullopt;
        }

        output.sourceFormat = ImageFormat::PNG;
        output.sourceDepth = source.bitDepth();
        output.metadata = source.metadata();
        return output;
    }

    static std::optional<ImageData<PixelType>> readFloat(const std::string& path) {
        int width = 0;
        int height = 0;
        int channelsInFile = 0;
        constexpr int desiredChannels = PixelChannels<PixelType>::value;

        float* data = stbi_loadf(
            path.c_str(),
            &width,
            &height,
            &channelsInFile,
            desiredChannels
        );

        if (!data) {
            return std::nullopt;
        }

        ImageData<PixelType> image(width, height);

        for (std::size_t i = 0; i < image.pixels.size(); ++i) {
            const std::size_t base = i * desiredChannels;
            image.pixels[i] = pixelFromInterleaved(data + base, static_cast<PixelType*>(nullptr));
        }

        stbi_image_free(data);
        return image;
    }
};


} // namespace imp_io

#endif /* ImageReader_hpp */
