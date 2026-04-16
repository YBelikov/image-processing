//
//  ImageProcessor.cpp
//  imp_engine
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#include "ImageProcessor.hpp"
#include "imp_io/ImageReader.hpp"
#include "imp_io/ImageWriter.hpp"
#include <iostream>

bool ImageProcessor::setup(std::string_view imagePath) {
  imp_io::ImageReader reader;
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
  imp_io::ImageWriter writer;
  bool success = writer.write(imageData_, outputPath);

  if (success) {
    std::cout << "ImageProcessor: exported to " << outputPath << std::endl;
  } else {
    std::cerr << "ImageProcessor: failed to export to " << outputPath
              << std::endl;
  }

  return success;
}
