#include "imp_geometry/AffineTransformation.hpp"
#include "imp_geometry/Point.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace imp_geometry;

// ── Translation ──────────────────────────────────────────────

TEST(ImpGeometry, MovePoint) {
  Point p(5, 6);
  AffineTransformation tr(1, 0, 4, 0, 1, 5);
  Point transformed = tr.apply(p);
  EXPECT_EQ(transformed.x, 9);
  EXPECT_EQ(transformed.y, 11);
}

// ── Rotation (integer-exact via 90° multiples) ───────────────

TEST(ImpGeometry, RotatePoint) {
  // 90° CCW about origin: [0, -1, 0; 1, 0, 0]
  AffineTransformation rot90(0, -1, 0, 1, 0, 0);
  Point p(3, 4);
  Point r = rot90.apply(p);
  EXPECT_EQ(r.x, -4);
  EXPECT_EQ(r.y, 3);
}

TEST(ImpGeometry, Rotate180) {
  AffineTransformation rot180(-1, 0, 0, 0, -1, 0);
  Point r = rot180.apply(Point(3, 4));
  EXPECT_EQ(r.x, -3);
  EXPECT_EQ(r.y, -4);
}

TEST(ImpGeometry, Rotate270) {
  // 270° CCW (= 90° CW): [0, 1, 0; -1, 0, 0]
  AffineTransformation rot270(0, 1, 0, -1, 0, 0);
  Point r = rot270.apply(Point(3, 4));
  EXPECT_EQ(r.x, 4);
  EXPECT_EQ(r.y, -3);
}

// ── Rotation with float precision via template apply ─────────

TEST(ImpGeometry, Rotate45_ApplyF) {
  float angle = static_cast<float>(M_PI / 4.0);
  float c = std::cos(angle);
  float s = std::sin(angle);
  AffineTransformation rot45(c, -s, 0, s, c, 0);
  PointF r = rot45.apply(PointF(1.0f, 0.0f));
  float expected = std::cos(angle); // ≈ 0.7071
  EXPECT_NEAR(r.x, expected, 1e-5f);
  EXPECT_NEAR(r.y, expected, 1e-5f);
}

// ── Composite: translation + rotation ────────────────────────

TEST(ImpGeometry, MoveAndRotate) {
  // 90° CCW + translate (2, 3): [0, -1, 2; 1, 0, 3]
  AffineTransformation tr(0, -1, 2, 1, 0, 3);
  Point r = tr.apply(Point(3, 4));
  EXPECT_EQ(r.x, -2);
  EXPECT_EQ(r.y, 6);
}

// ── Reflections ──────────────────────────────────────────────

TEST(ImpGeometry, FlipHorizontal) {
  // Reflect across Y axis: [-1, 0, 0; 0, 1, 0]
  AffineTransformation flip(-1, 0, 0, 0, 1, 0);
  Point r = flip.apply(Point(5, 7));
  EXPECT_EQ(r.x, -5);
  EXPECT_EQ(r.y, 7);
}

TEST(ImpGeometry, FlipVertical) {
  // Reflect across X axis: [1, 0, 0; 0, -1, 0]
  AffineTransformation flip(1, 0, 0, 0, -1, 0);
  Point r = flip.apply(Point(5, 7));
  EXPECT_EQ(r.x, 5);
  EXPECT_EQ(r.y, -7);
}

// ── Scaling ──────────────────────────────────────────────────

TEST(ImpGeometry, ScalePoint) {
  AffineTransformation scale(3, 0, 0, 0, 3, 0);
  Point r = scale.apply(Point(2, 5));
  EXPECT_EQ(r.x, 6);
  EXPECT_EQ(r.y, 15);
}

// ── Identity ─────────────────────────────────────────────────

TEST(ImpGeometry, IdentityTransform) {
  AffineTransformation id(1, 0, 0, 0, 1, 0);
  Point p(42, -17);
  Point r = id.apply(p);
  EXPECT_EQ(r.x, p.x);
  EXPECT_EQ(r.y, p.y);
}

// ── Zero transform ───────────────────────────────────────────

TEST(ImpGeometry, ZeroTransformMapsToOrigin) {
  AffineTransformation zero(0, 0, 0, 0, 0, 0);
  EXPECT_FLOAT_EQ(zero.a00, 0.0f);
  EXPECT_FLOAT_EQ(zero.a11, 0.0f);
  Point r = zero.apply(Point(99, 99));
  EXPECT_EQ(r.x, 0);
  EXPECT_EQ(r.y, 0);
}
