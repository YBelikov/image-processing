//
//  main.cpp
//  imp_engine
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#include "ImageProcessor.hpp"
#include "imp_io/ImageReader.hpp"
#include "imp_io/ImageWriter.hpp"

#include <iostream>

using namespace imp_engine;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: imp_engine <input_image> <output_image>"
                  << std::endl;
        return 1;
    }
    const char *inputPath = argv[1];
    const char *outputPath = argv[2];
    ImageProcessor processor;
    imp_io::ImageReader reader;
    auto baseData = reader.read(inputPath);
    auto overlayData = reader.read("/Users/ybelikov/Downloads/png-clipart-light-blue-light-blue-background-blue-angle.png");
    if (!overlayData.has_value() || !baseData.has_value()) {
        return 1;
    }
    auto result = processor.blendLayers(baseData.value(), overlayData.value());
    imp_io::ImageWriter writer;
    writer.write(result, outputPath);
    std::cout << "Done." << std::endl;
    return 0;
}
