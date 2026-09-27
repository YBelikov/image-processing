//
//  main.cpp
//  imp_engine
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#include "ImageProcessor.hpp"
#include "imp_io/ImageReader.hpp"
#include "imp_io/ImageWriter.hpp"
#include "imp_io/Pixel.hpp"
#include "imp_algorithms/Composition.hpp"

#include <iostream>

using namespace imp_engine;
using namespace imp_io;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: imp_engine <input_image> <output_image>"
                  << std::endl;
        return 1;
    }
    const char *inputPath = argv[1];
    const char *outputPath = argv[2];
    imp_io::ImageReader<PixelRGBA_F> reader;
    auto baseData = reader.read(inputPath);
    auto overlayData = reader.read("/Users/ybelikov/Downloads/sample1.png");
    if (!overlayData.has_value() || !baseData.has_value()) {
        return 1;
    }
    imp_io::ImageWriter<PixelRGBA_F> writer;
    auto result = imp_algorithms::composeImages(*overlayData, *baseData, imp_algorithms::PorterDuffOperator::Destination);
    result.sourceFormat = baseData->sourceFormat;
    result.sourceDepth = baseData->sourceDepth;
    result.metadata = baseData->metadata;
    bool writeStatus = writer.write(result, outputPath);
    if (!writeStatus) {
        std::cout << "Failed to produce a composition\n";
    } else {
        std::cout << "Success";
    }
    return 0;
}
