# Image Processing Lab

This repository is a C++ image-processing playground for learning algorithms,
building experiments, and doing performance research.

The current engine lives in `imp_engine/` and is organized around small modules:

- `imp_io`: image loading and writing.
- `imp_geometry`: points and affine transformations.
- `imp_algorithms`: future home for reusable algorithms.

## Building

The engine uses CMake, Conan, and Ninja. From `imp_engine/`:

```sh
./configure_arm64.sh Debug
cmake --build --preset ninja-arm64-debug
ctest --preset ninja-arm64-debug
```

For macOS universal artifacts:

```sh
./build_universal_ninja.sh Debug
```

To rebuild and merge one universal target:

```sh
./build_universal_ninja.sh Debug imp_io
```

The merged `arm64`/`x86_64` outputs are written under
`imp_engine/build_ninja/universal-debug/`.

## Research Track

Start here:

- [Research roadmap](docs/research-roadmap.md)
- [Benchmark protocol](docs/benchmark-protocol.md)
- [Experiment notebook](experiments/README.md)

## Suggested Next Step

The most useful first implementation milestone is a tiny `imp_algorithms`
library with:

- grayscale conversion,
- nearest and bilinear sampling,
- tests over synthetic images,
- a benchmark runner that reports megapixels per second.

That gives every future idea a baseline for correctness, speed, and image
quality.
