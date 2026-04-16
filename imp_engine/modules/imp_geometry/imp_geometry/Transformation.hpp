//
//  Transformation.hpp
//  imp_geometry
//
//  Created by Yuriy Belikov  on 01.03.2026.
//

#ifndef Transformation_hpp
#define Transformation_hpp

#include "imp_geometry/Point.hpp"

namespace imp_geometry {

template <typename PointType> class Transformation {
public:
  virtual PointType apply(const PointType &) const noexcept = 0;
  virtual ~Transformation() = default;
};

} // namespace imp_geometry

#endif /* Transformation_hpp */
