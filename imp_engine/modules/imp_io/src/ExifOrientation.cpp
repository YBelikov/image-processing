#include "ExifOrientation.hpp"

#include <cstddef>

namespace imp_io::detail {
namespace {

constexpr std::size_t tiffHeaderSize = 8;
constexpr std::size_t ifdEntrySize = 12;
constexpr std::size_t ifdCountSize = 2;
constexpr std::size_t ifdNextSize = 4;
constexpr std::uint16_t tiffMagic = 42;
constexpr std::uint16_t orientationTag = 0x0112;
constexpr std::uint16_t shortType = 3;
constexpr std::size_t magicOffset = 2;
constexpr std::size_t ifdOffset = 4;
constexpr std::size_t typeOffset = 2;
constexpr std::size_t countOffset = 4;
constexpr std::size_t valueOffset = 8;
constexpr std::size_t orientationExifSize = tiffHeaderSize + ifdCountSize + ifdEntrySize + ifdNextSize;

enum class ByteOrder { Little, Big };

std::uint32_t readInteger(std::span<const std::uint8_t> bytes, std::size_t offset,
                          std::size_t size, ByteOrder order) {
    constexpr unsigned bitsPerByte = 8;
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < size; ++index) {
        const auto position = order == ByteOrder::Big ? index : size - index - 1;
        value = (value << bitsPerByte) | bytes[offset + position];
    }
    return value;
}

CodecError invalidExif() {
    return {CodecErrorCode::InvalidData, "Malformed EXIF orientation"};
}

} // namespace

OrientationResult readOrientation(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < tiffHeaderSize) {
        return invalidExif();
    }

    const bool little = bytes[0] == 'I' && bytes[1] == 'I';
    const bool big = bytes[0] == 'M' && bytes[1] == 'M';
    if (!little && !big) {
        return invalidExif();
    }

    const auto order = little ? ByteOrder::Little : ByteOrder::Big;
    if (readInteger(bytes, magicOffset, sizeof(std::uint16_t), order) != tiffMagic) {
        return invalidExif();
    }

    const auto start = readInteger(bytes, ifdOffset, sizeof(std::uint32_t), order);
    if (start < tiffHeaderSize || start > bytes.size() - ifdCountSize) {
        return invalidExif();
    }

    const auto count = readInteger(bytes, start, ifdCountSize, order);
    const auto entries = start + ifdCountSize;
    const auto remaining = bytes.size() - entries;
    if (remaining < ifdNextSize || count > (remaining - ifdNextSize) / ifdEntrySize) {
        return invalidExif();
    }

    std::optional<Orientation> orientation;
    for (std::size_t index = 0; index < count; ++index) {
        const auto offset = entries + index * ifdEntrySize;
        if (readInteger(bytes, offset, sizeof(std::uint16_t), order) != orientationTag) {
            continue;
        }
        if (orientation
            || readInteger(bytes, offset + typeOffset, sizeof(std::uint16_t), order) != shortType
            || readInteger(bytes, offset + countOffset, sizeof(std::uint32_t), order) != 1) {
            return invalidExif();
        }

        const auto value = readInteger(bytes, offset + valueOffset, sizeof(std::uint16_t), order);
        if (value < static_cast<unsigned>(Orientation::Identity)
            || value > static_cast<unsigned>(Orientation::Rotate90Counterclockwise)) {
            return invalidExif();
        }
        orientation = static_cast<Orientation>(value);
    }

    return orientation;
}

std::vector<std::uint8_t> writeOrientation(Orientation orientation) {
    std::vector<std::uint8_t> bytes(orientationExifSize);
    constexpr unsigned bitsPerByte = 8;
    constexpr unsigned byteMask = 0xff;
    const auto put = [&bytes](std::size_t offset, std::uint32_t value, std::size_t count) {
        for (std::size_t index = 0; index < count; ++index) {
            bytes[offset + index] = static_cast<std::uint8_t>((value >> (bitsPerByte * index)) & byteMask);
        }
    };

    bytes[0] = 'I';
    bytes[1] = 'I';
    put(magicOffset, tiffMagic, sizeof(std::uint16_t));
    put(ifdOffset, tiffHeaderSize, sizeof(std::uint32_t));
    put(tiffHeaderSize, 1, ifdCountSize);
    constexpr auto entry = tiffHeaderSize + ifdCountSize;
    put(entry, orientationTag, sizeof(std::uint16_t));
    put(entry + typeOffset, shortType, sizeof(std::uint16_t));
    put(entry + countOffset, 1, sizeof(std::uint32_t));
    put(entry + valueOffset, static_cast<unsigned>(orientation), sizeof(std::uint16_t));
    return bytes;
}

} // namespace imp_io::detail
