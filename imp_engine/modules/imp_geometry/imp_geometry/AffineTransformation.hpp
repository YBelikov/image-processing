//
//  AffineTransformation.hpp
//  imp_geometry
//
//  Created by Yuriy Belikov  on 01.03.2026.
//

#ifndef AffineTransformation_hpp
#define AffineTransformation_hpp

#include "imp_geometry/Transformation.hpp"

namespace imp_geometry {

class AffineTransformation {
public:
  AffineTransformation() = default;
  explicit AffineTransformation(float _a00, float _a01, float _a02, float _a10,
                                float _a11, float _a12);

  template <typename T> _Point<T> apply(const _Point<T> &p) const noexcept {
    return _Point<T>(static_cast<T>(p.x * a00 + p.y * a01 + a02),
                     static_cast<T>(p.x * a10 + p.y * a11 + a12));
  }

  float a00, a01, a02, a10, a11, a12;
};

} // namespace imp_geometry
#endif /* AffineTransformation_hpp */
