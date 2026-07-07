# Image Processing Research Roadmap

This repo can grow into a compact laboratory for learning image processing,
testing algorithms, and measuring performance honestly. The current shape is a
good seed: `imp_io` handles image loading/writing, `imp_geometry` contains basic
transforms, and `imp_algorithms` is ready to become the home for experiments.

## Research Principles

- Prefer small, measurable experiments over large rewrites.
- Keep correctness tests and performance benchmarks separate.
- Save experiment notes next to the code that produced them.
- Compare against a simple baseline before optimizing.
- Track image quality, runtime, memory traffic, and implementation complexity.
- Treat OpenCV as both a reference implementation and a competitor.

## Phase 1: Foundations

Goal: make the engine a reliable playground.

- Stabilize image storage around one explicit pixel model.
- Decide whether the core representation is byte, float, planar, interleaved, or
  supports several views.
- Add synthetic image generators for gradients, checkerboards, impulses, ramps,
  edges, and noise fields.
- Add golden tests for tiny images where expected pixels can be inspected by
  hand.
- Add a benchmark executable with repeatable inputs and warmup iterations.

Suggested experiments:

- Nearest, bilinear, bicubic, and Lanczos sampling.
- Box blur, separable Gaussian blur, and integral-image blur.
- RGB, grayscale, HSV, YCbCr, and linear-light conversions.
- Histogram, cumulative histogram, equalization, and CLAHE.
- Sobel, Scharr, Laplacian, Canny-like edge detection.

## Phase 2: Geometry And Resampling

Goal: understand the cost and quality tradeoffs of moving pixels.

- Implement inverse-mapped affine warps.
- Compare border modes: clamp, mirror, wrap, constant, transparent.
- Compare interpolation kernels by visual quality and runtime.
- Measure aliasing under downscale and ringing under upscale.
- Explore tile-based processing for cache locality.

Research questions:

- When does bicubic visibly beat bilinear enough to justify its cost?
- How much speed is gained by precomputing horizontal weights?
- What image sizes cross cache boundaries on your machine?
- Can a separable transform pipeline reduce memory traffic?

## Phase 3: Performance Engineering

Goal: develop a serious intuition for modern CPU image processing.

- Benchmark scalar loops, compiler auto-vectorization, and explicit SIMD.
- Compare interleaved RGB against planar channel storage.
- Measure branchless border handling versus padded images.
- Explore thread tiling with row chunks, 2D tiles, and task queues.
- Track memory bandwidth, allocation count, and cache behavior.

Suggested metrics:

- Megapixels per second.
- Nanoseconds per pixel.
- Bytes read/written per output pixel.
- Peak and average absolute error against reference output.
- PSNR or SSIM where visual fidelity matters.

## Phase 4: Advanced Algorithms

Goal: implement algorithms where quality and speed both matter.

- Edge-aware filters: bilateral, guided filter, domain transform.
- Denoising: median, non-local means subset, wavelet thresholding.
- Retargeting: seam carving and energy maps.
- Feature detection: Harris, FAST-like corners, image pyramids.
- Frequency domain: convolution via FFT, sharpening, deconvolution.
- Tone mapping: HDR exposure, local contrast, filmic curves.
- Segmentation: thresholding, watershed, graph-cut-inspired prototypes.

## Phase 5: Innovative Directions

Goal: turn the repo into a place for original research.

- Adaptive kernels that choose interpolation quality per region.
- Learned or heuristic filter scheduling based on image statistics.
- Hybrid CPU pipelines that fuse multiple operations into one pass.
- Quality/performance search that auto-tunes tile sizes and kernel variants.
- Perceptual error metrics that guide fast approximate algorithms.
- Progressive algorithms that produce preview-quality output first, then refine.

## Repository Shape

Recommended structure:

```text
imp_engine/
  modules/
    imp_algorithms/
      imp_algorithms/
      src/
      tests/
      benchmarks/
experiments/
  000-template.md
  README.md
docs/
  benchmark-protocol.md
  research-roadmap.md
```

## First Milestones

1. Finish the typed pixel transition in `imp_io`.
2. Add a minimal `imp_algorithms` library with one grayscale conversion.
3. Add tests for synthetic 2x2 and 3x3 images.
4. Add a benchmark runner for grayscale and blur.
5. Add affine image warp with nearest and bilinear sampling.
6. Compare output against OpenCV for correctness and speed.

