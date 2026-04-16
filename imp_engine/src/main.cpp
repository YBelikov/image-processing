//
//  main.cpp
//  imp_engine
//
//  Created by Yuriy Belikov on 09.03.2026.
//

#include "ImageProcessor.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: imp_engine <input_image> <output_image>" << std::endl;
    return 1;
  }

  const char *inputPath = argv[1];
  const char *outputPath = argv[2];

  ImageProcessor processor;

  if (!processor.setup(inputPath)) {
    return 1;
  }

  processor.render();

  if (!processor.exportImage(outputPath)) {
    return 1;
  }

  std::cout << "Done." << std::endl;
  return 0;
}
