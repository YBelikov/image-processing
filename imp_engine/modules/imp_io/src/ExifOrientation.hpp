#ifndef IMP_IO_EXIF_ORIENTATION_HPP
#define IMP_IO_EXIF_ORIENTATION_HPP

#include "imp_io/CodecResult.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace imp_io::detail {

using OrientationResult = std::variant<std::optional<Orientation>, CodecError>;

OrientationResult readOrientation(std::span<const std::uint8_t> bytes);
std::vector<std::uint8_t> writeOrientation(Orientation orientation);

} // namespace imp_io::detail

#endif
