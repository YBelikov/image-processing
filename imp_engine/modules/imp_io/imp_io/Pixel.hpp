//
//  Pixel.hpp
//  imp_engine
//
//  Created by Yuriy Belikov  on 01.05.2026.
//

#ifndef Pixel_hpp
#define Pixel_hpp

template<typename T>
struct _PixelRGB {
    T r, g, b, a;
};

using Pixel = _PixelRGB<float>;
#endif /* Pixel_hpp */
