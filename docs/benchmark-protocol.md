# Benchmark Protocol

Benchmarks are useful only when they are repeatable. Use this protocol for image
processing experiments in this repo.

## Build Mode

- Use a release build.
- Record compiler, CPU, OS, and CMake preset.
- Disable debug assertions inside timed loops.
- Keep benchmark binaries separate from unit tests.

## Inputs

Use at least four image categories:

- Tiny synthetic images for sanity checks.
- Medium images around 1 to 4 megapixels.
- Large images around 12 to 48 megapixels.
- Stress images such as noise, high-contrast edges, gradients, and alpha masks.

Synthetic inputs are preferred for early research because they are reproducible
and can reveal algorithmic mistakes quickly.

## Timing

- Run one or more warmup iterations before measurement.
- Measure enough iterations to get stable median and p95 values.
- Report pixels processed per second and nanoseconds per pixel.
- Include allocation setup only when the algorithm requires allocation per call.
- Exclude image file IO unless the experiment is specifically about IO.

## Correctness

Every performance experiment should name its reference output.

Reference options:

- A direct scalar implementation.
- A tiny hand-verified expected image.
- OpenCV output with matching border and interpolation settings.
- A high-quality offline variant used as a quality target.

Record error metrics:

- Maximum absolute error.
- Mean absolute error.
- RMSE.
- PSNR when outputs are normalized.

## Experiment Log

Each experiment should include:

- Hypothesis.
- Baseline.
- Variants tested.
- Input images.
- Hardware and build settings.
- Results table.
- Notes about surprising behavior.
- Decision: keep, reject, or investigate more.

