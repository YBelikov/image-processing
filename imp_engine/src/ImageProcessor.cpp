//
//  ImageProcessor.cpp
//  imp_engine
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#include "ImageProcessor.hpp"
#include "imp_io/ImageReader.hpp"
#include "imp_io/ImageWriter.hpp"
#include <cassert>
#include <iostream>

using namespace imp_io;

namespace imp_engine {
bool ImageProcessor::setup(std::string_view imagePath) {
    ImageReader reader;
    auto result = reader.read(imagePath);

    if (!result.has_value()) {
        std::cerr << "ImageProcessor: failed to load image: " << imagePath
                  << std::endl;
        return false;
    }

    imageData_ = std::move(result.value());

    std::cout << "ImageProcessor: loaded " << imageData_.width << "x"
              << imageData_.height << " image (" << imageData_.channels
              << " channels)" << std::endl;

    return true;
}

void ImageProcessor::render() {
    // Stub — future transformation algorithms will be applied here.
    std::cout << "ImageProcessor: render (no-op)" << std::endl;
}

bool ImageProcessor::exportImage(std::string_view outputPath) {
    ImageWriter writer;
    bool success = writer.write(imageData_, outputPath);

    if (success) {
        std::cout << "ImageProcessor: exported to " << outputPath << std::endl;
    } else {
        std::cerr << "ImageProcessor: failed to export to " << outputPath
                  << std::endl;
    }

    return success;
}

float linearInterpolation(float a, float b, float t) { return a + (b - a) * t; }

imp_io::ImageData ImageProcessor::blendLayers(const imp_io::ImageData &base,
                                              const imp_io::ImageData &overlay) {
    auto bottomPixels = base.pixels;
    auto topPixels = overlay.pixels;
    ImageData result(base.width, base.height, base.channels);
    const auto heightBound = std::min(base.height, overlay.height);
    const auto widthBound = std::min(base.width, overlay.width);
    for (int y = 0; y < heightBound; ++y) {
        for (int x = 0; x < widthBound; ++x) {
            int bottomBaseIdx = (y * base.width + x) * base.channels;
            int topBaseIdx = (y * overlay.width + x) * overlay.channels;
            for (int ch = 0; ch < base.channels; ++ch) {
                result.pixels[bottomBaseIdx + ch] = linearInterpolation(base.pixels[bottomBaseIdx + ch], overlay.pixels[topBaseIdx + ch], 0.3f);
            }
        }
    }
    return std::move(result);
}

} // namespace imp_engine
