#ifndef COMMON_HPP
#define COMMON_HPP
#include <cstdint>
#include <algorithm>

namespace imp_io {

enum class ColorSpace : uint8_t {
        LinearRGB = 0, 
        sRGB
};

template<typename PixelType>
    void convertToColorProfile(ColorSpace targetCP, ImageData<PixelType>& image) {
        if (targetCP == ColorSpace::LinearRGB) {
            const auto convertChannel = [](float channelValue) { 
                if (channelValue <= 0.04045f) {
                    return channelValue / 12.92f;
                } 
                return std::pow((channelValue + 0.055f) / 1.055f, 2.4f);
            };

            const auto convertPixel = [&convertChannel](PixelType& pix) {
                pix.r = convertChannel(pix.r);
                pix.g = convertChannel(pix.g);
                pix.b = convertChannel(pix.b);
            };

            for (int row = 0; row < image.height; ++row) {
                for (int col = 0; col < image.width; ++col) {
                    convertPixel(image.pixels[row * image.width + col]);
                }
            }
        }

        if (targetCP == ColorSpace::sRGB) {
            const auto convertChannel = [](float channelValue) { 
                if (channelValue <= 0.0031308f) {
                    return std::clamp(channelValue * 12.92f, 0.f, 1.f);
                } 
                return std::clamp(1.055f * std::pow(channelValue, 1/2.4f) - 0.055f, 0.f, 1.f);
            };

            const auto convertPixel = [&convertChannel](PixelType& pix) {
                pix.r = convertChannel(pix.r);
                pix.g = convertChannel(pix.g);
                pix.b = convertChannel(pix.b);
            };

            for (int row = 0; row < image.height; ++row) {
                for (int col = 0; col < image.width; ++col) {
                    convertPixel(image.pixels[row * image.width + col]);
                }
            }
        }
    }
}
#endif