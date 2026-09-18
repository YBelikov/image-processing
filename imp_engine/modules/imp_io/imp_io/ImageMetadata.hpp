#ifndef IMP_IO_IMAGE_METADATA_HPP
#define IMP_IO_IMAGE_METADATA_HPP

#include <cstdint>
#include <optional>
#include <vector>

namespace imp_io {

// EXIF orientation values describe the display transform; samples stay unchanged.
enum class Orientation : std::uint8_t {
    Identity = 1,
    MirrorHorizontal = 2,
    Rotate180 = 3,
    MirrorVertical = 4,
    Transpose = 5,
    Rotate90Clockwise = 6,
    Transverse = 7,
    Rotate90Counterclockwise = 8
};

enum class RenderingIntent : std::uint8_t {
    Perceptual = 0,
    RelativeColorimetric = 1,
    Saturation = 2,
    AbsoluteColorimetric = 3
};

// Public value records let callers supply metadata without codec dependencies.
struct Chromaticity {
    double x;
    double y;
};

struct Chromaticities {
    Chromaticity white;
    Chromaticity red;
    Chromaticity green;
    Chromaticity blue;
};

struct PngColorDescription {
    // gAMA's image gamma, with the file's integer scale removed (45455 -> 0.45455).
    std::optional<double> gamma;
    std::optional<Chromaticities> chromaticities;
    std::optional<RenderingIntent> srgbIntent;
};

struct ImageMetadata {
    // Absence is unknown, not an assumed identity transform or color space.
    std::optional<Orientation> orientation;
    std::optional<std::vector<std::uint8_t>> iccProfile;
    std::optional<PngColorDescription> pngColor;
};

} // namespace imp_io

#endif
