//
//  Pixel.hpp
//  imp_engine
//
//  Created by Yuriy Belikov  on 01.05.2026.
//

#ifndef Pixel_hpp
#define Pixel_hpp

#include <vector>

namespace imp_io {

template <typename T>
struct _PixelRGBA {
    T r, g, b, a;
};

template <typename T>
struct _PixelRGB {
    T r, g, b;
};

using PixelRGB_F = _PixelRGB<float>;
using PixelRGBA_F = _PixelRGBA<float>;


template<typename T>
struct PixelChannels;

template<>
struct PixelChannels<PixelRGB_F> {
    static constexpr int value = 3;
};

template<>
struct PixelChannels<PixelRGBA_F> {
    static constexpr int value = 4;
};

inline PixelRGB_F& operator /= (PixelRGB_F& lhs, float rhs) {
    lhs.r /= rhs;
    lhs.g /= rhs;
    lhs.b /= rhs;
    return lhs;
}

inline PixelRGB_F operator / (const PixelRGB_F& lhs, float rhs) {
    return { lhs.r / rhs, lhs.g / rhs, lhs.b / rhs };
}

inline PixelRGB_F operator + (const PixelRGB_F& lhs, const PixelRGB_F& rhs) {
    return {lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b};
}

template<typename U>
inline PixelRGB_F operator + (const PixelRGB_F& lhs, U val) {
    return { lhs.r + val, lhs.g + val, lhs.b + val };
}

template<typename U>
inline PixelRGB_F operator * (const PixelRGB_F& lhs, U val) {
    return { lhs.r * val, lhs.g * val, lhs.b * val };
}

inline PixelRGBA_F& operator /= (PixelRGBA_F& lhs, float rhs) {
    lhs.r /= rhs;
    lhs.g /= rhs;
    lhs.b /= rhs;
    lhs.a /= rhs;
    return lhs;
}

inline PixelRGBA_F operator / (const PixelRGBA_F& lhs, float rhs) {
    return { lhs.r / rhs, lhs.g / rhs, lhs.b / rhs, lhs.a / rhs };
}

inline PixelRGBA_F operator + (const PixelRGBA_F& lhs, const PixelRGBA_F& rhs) {
    return {lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b, lhs.a + rhs.a};
}

template<typename U>
inline PixelRGBA_F operator + (const PixelRGBA_F& lhs, U val) {
    return { lhs.r + val, lhs.g + val, lhs.b + val, lhs.a + val };
}

template<typename U>
inline PixelRGBA_F operator * (const PixelRGBA_F& lhs, U val) {
    return { lhs.r * val, lhs.g * val, lhs.b * val, lhs.a * val };
}

inline PixelRGB_F pixelFromInterleaved(const float* data, PixelRGB_F*) {
    return {data[0], data[1], data[2]};
}

inline PixelRGBA_F pixelFromInterleaved(const float* data, PixelRGBA_F*) {
    return {data[0], data[1], data[2], data[3]};
}

inline void appendInterleaved(std::vector<float>& data, const PixelRGB_F& pixel) {
    data.push_back(pixel.r);
    data.push_back(pixel.g);
    data.push_back(pixel.b);
}

inline void appendInterleaved(std::vector<float>& data, const PixelRGBA_F& pixel) {
    data.push_back(pixel.r);
    data.push_back(pixel.g);
    data.push_back(pixel.b);
    data.push_back(pixel.a);
}

} // namespace imp_io
#endif /* Pixel_hpp */
