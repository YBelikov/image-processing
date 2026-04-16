//
//  Point.hpp
//  imp_geometry
//
//  Created by Yuriy Belikov  on 01.03.2026.
//

#ifndef Point_hpp
#define Point_hpp

namespace imp_geometry {
template <typename T> struct _Point {
  _Point() = default;
  explicit _Point(T _x, T _y) noexcept : x{_x}, y{_y} {}
  _Point(const _Point &other) noexcept = default;
  _Point(_Point &&other) noexcept = default;
  _Point &operator=(const _Point &other) noexcept = default;
  _Point &operator=(_Point &&other) noexcept = default;
  T x;
  T y;
};
using Point = _Point<int>;
using PointF = _Point<float>;
} // namespace imp_geometry

#endif /* Point_hpp */
