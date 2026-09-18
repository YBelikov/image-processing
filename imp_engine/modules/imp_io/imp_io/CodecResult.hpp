#ifndef IMP_IO_CODEC_RESULT_HPP
#define IMP_IO_CODEC_RESULT_HPP

#include "imp_io/DecodedImage.hpp"

#include <string>
#include <variant>

namespace imp_io {

enum class CodecErrorCode {
    InvalidArgument,
    UnsupportedFormat,
    UnsupportedPrecision,
    UnsupportedLayout,
    IncompatibleMetadata,
    UnsupportedFeature,
    CodecUnavailable,
    FileOpenFailed,
    FileReadFailed,
    FileWriteFailed,
    InvalidData,
    ResourceLimit,
    DecodeFailed,
    EncodeFailed
};

// Public result records carry a stable category and a contextual diagnostic.
struct CodecError {
    CodecErrorCode code;
    std::string message;
};

using DecodeResult = std::variant<DecodedImage, CodecError>;
using EncodeResult = std::variant<std::monostate, CodecError>;

} // namespace imp_io

#endif
