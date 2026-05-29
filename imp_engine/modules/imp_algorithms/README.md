# imp_algorithms

This module is intended for reusable image-processing algorithms and their
benchmarks.

Start small:

- Color conversion.
- Sampling and interpolation.
- Convolution kernels.
- Geometric warps.
- Histogram operations.
- Edge detection.

Keep experimental prototypes in `experiments/` until the API feels stable. Move
algorithms here when they have tests, clear inputs and outputs, and at least one
reference comparison.

Suggested future layout:

```text
imp_algorithms/
  Color.hpp
  Convolution.hpp
  Sampling.hpp
  Warp.hpp
src/
  Color.cpp
  Convolution.cpp
tests/
  test_color.cpp
  test_convolution.cpp
benchmarks/
  bench_color.cpp
  bench_convolution.cpp
```

