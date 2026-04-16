//
//  AffineTransformation.cpp
//  imp_geometry
//
//  Created by Yuriy Belikov  on 01.03.2026.
//

#include "imp_geometry/AffineTransformation.hpp"

namespace imp_geometry {

AffineTransformation::AffineTransformation(float _a00, float _a01, float _a02,
                                           float _a10, float _a11, float _a12)
    : a00(_a00), a01(_a01), a02(_a02), a10(_a10), a11(_a11), a12(_a12) {}

} // namespace imp_geometry
