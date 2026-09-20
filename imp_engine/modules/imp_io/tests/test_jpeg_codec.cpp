#include "imp_io/JpegCodec.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <system_error>
#include <vector>
#include <gtest/gtest.h>

using namespace imp_io;

namespace {

constexpr std::size_t imageWidth = 16;
constexpr std::size_t imageHeight = 12;
constexpr int fixtureTolerance = 17;
constexpr int encodeTolerance = 32;
constexpr std::uint8_t markerPrefix = 0xff;
constexpr std::uint8_t baselineSof = 0xc0;
constexpr std::uint8_t progressiveSof = 0xc2;
constexpr std::uint8_t scanMarker = 0xda;
constexpr std::uint8_t endMarker = 0xd9;

std::filesystem::path fixture(const char* name) {
    return std::filesystem::path(IMP_IO_JPEG_FIXTURE_DIR) / name;
}

std::vector<std::uint8_t> readBytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

std::vector<std::uint8_t> rgbSamples() {
    std::vector<std::uint8_t> samples;
    samples.reserve(imageWidth * imageHeight * 3);
    for (std::size_t y = 0; y < imageHeight; ++y) {
        for (std::size_t x = 0; x < imageWidth; ++x) {
            samples.push_back(static_cast<std::uint8_t>((x * 13 + y * 3) % 256));
            samples.push_back(static_cast<std::uint8_t>((x * 5 + y * 17) % 256));
            samples.push_back(static_cast<std::uint8_t>((x * 11 + y * 7) % 256));
        }
    }
    return samples;
}

std::vector<std::uint8_t> grayAsRgb() {
    std::vector<std::uint8_t> samples;
    samples.reserve(imageWidth * imageHeight * 3);
    for (std::size_t y = 0; y < imageHeight; ++y) {
        for (std::size_t x = 0; x < imageWidth; ++x) {
            const auto value = static_cast<std::uint8_t>((x * 11 + y * 13) % 256);
            samples.insert(samples.end(), 3, value);
        }
    }
    return samples;
}

double mse(const std::vector<std::uint8_t>& left, const std::vector<std::uint8_t>& right) {
    double sum = 0.0;
    for (std::size_t index = 0; index < left.size(); ++index) {
        const auto difference = static_cast<double>(left[index]) - right[index];
        sum += difference * difference;
    }
    return sum / static_cast<double>(left.size());
}

void expectNear(const std::vector<std::uint8_t>& actual,
                const std::vector<std::uint8_t>& expected, int tolerance) {
    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t index = 0; index < actual.size(); ++index) {
        EXPECT_LE(std::abs(static_cast<int>(actual[index]) - expected[index]), tolerance)
            << "sample " << index;
    }
}

template<typename Result>
void expectError(const Result& result, CodecErrorCode code) {
    const auto* error = std::get_if<CodecError>(&result);
    ASSERT_NE(error, nullptr);
    EXPECT_EQ(error->code, code) << error->message;
    EXPECT_FALSE(error->message.empty());
}

struct SofInfo {
    std::uint8_t marker = 0;
    std::vector<std::uint8_t> sampling;
};

SofInfo readSof(const std::vector<std::uint8_t>& bytes) {
    constexpr std::size_t signatureSize = 2;
    constexpr std::size_t markerHeaderSize = 4;
    constexpr std::size_t sofFixedSize = 6;
    constexpr std::size_t componentSize = 3;
    SofInfo info;
    for (std::size_t offset = signatureSize; offset + markerHeaderSize <= bytes.size();) {
        if (bytes[offset] != markerPrefix) {
            ++offset;
            continue;
        }
        while (offset < bytes.size() && bytes[offset] == markerPrefix) {
            ++offset;
        }
        if (offset >= bytes.size()) {
            break;
        }
        const auto marker = bytes[offset++];
        if (marker == scanMarker || marker == endMarker) {
            break;
        }
        if (offset + 2 > bytes.size()) {
            break;
        }
        const auto length = static_cast<std::size_t>((bytes[offset] << 8) | bytes[offset + 1]);
        if (length < 2 || offset + length > bytes.size()) {
            break;
        }
        if (marker == baselineSof || marker == progressiveSof) {
            const auto payload = offset + 2;
            if (length < sofFixedSize + 2 || payload + sofFixedSize > bytes.size()) {
                break;
            }
            const auto components = bytes[payload + 5];
            if (length != sofFixedSize + 2 + components * componentSize) {
                break;
            }
            info.marker = marker;
            for (std::size_t index = 0; index < components; ++index) {
                info.sampling.push_back(bytes[payload + sofFixedSize + index * componentSize + 1]);
            }
            return info;
        }
        offset += length;
    }
    return info;
}

class JpegTest : public ::testing::Test {
protected:
    const JpegCodec& codec() const { return codec_; }
    std::filesystem::path output(const char* name = "output.jpg") const {
        return directory_ / name;
    }

private:
    void SetUp() override {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        directory_ = std::filesystem::temp_directory_path()
            / ("imp_jpeg_" + std::to_string(stamp) + "_" + std::to_string(std::random_device{}()));
        std::error_code error;
        created_ = std::filesystem::create_directory(directory_, error);
        ASSERT_TRUE(created_) << error.message();
    }

    void TearDown() override {
        if (created_) {
            std::error_code ignored;
            std::filesystem::remove_all(directory_, ignored);
        }
    }

    JpegCodec codec_;
    std::filesystem::path directory_;
    bool created_ = false;
};

TEST_F(JpegTest, DecodesBaselineAndProgressive) {
    const auto expected = rgbSamples();
    for (const auto name : {"baseline.jpg", "progressive.jpg"}) {
        SCOPED_TRACE(name);
        const auto result = codec().decode(fixture(name));
        ASSERT_TRUE(std::holds_alternative<DecodedImage>(result));
        const auto& image = std::get<DecodedImage>(result);
        EXPECT_EQ(image.width(), imageWidth);
        EXPECT_EQ(image.height(), imageHeight);
        EXPECT_EQ(image.layout(), ChannelLayout::RGB);
        EXPECT_EQ(image.bitDepth(), SampleDepth::Bits8);
        expectNear(std::get<std::vector<std::uint8_t>>(image.samples()), expected,
                   fixtureTolerance);
    }
    EXPECT_EQ(readSof(readBytes(fixture("baseline.jpg"))).marker, baselineSof);
    EXPECT_EQ(readSof(readBytes(fixture("progressive.jpg"))).marker, progressiveSof);
}

TEST_F(JpegTest, ExpandsGrayscaleToRgb) {
    const auto result = codec().decode(fixture("grayscale.jpg"));
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(result));
    const auto& image = std::get<DecodedImage>(result);
    EXPECT_EQ(image.layout(), ChannelLayout::RGB);
    const auto& actual = std::get<std::vector<std::uint8_t>>(image.samples());
    expectNear(actual, grayAsRgb(), fixtureTolerance);
    for (std::size_t index = 0; index < actual.size(); index += 3) {
        EXPECT_EQ(actual[index], actual[index + 1]);
        EXPECT_EQ(actual[index], actual[index + 2]);
    }
}

TEST_F(JpegTest, PreservesIccAndOrientation) {
    const auto expectedIcc = readBytes(fixture("large_rgb.icc"));
    const auto result = codec().decode(fixture("metadata.jpg"));
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(result));
    const auto& image = std::get<DecodedImage>(result);
    EXPECT_EQ(image.metadata().orientation, Orientation::Rotate90Clockwise);
    ASSERT_TRUE(image.metadata().iccProfile);
    EXPECT_EQ(*image.metadata().iccProfile, expectedIcc);
    expectNear(std::get<std::vector<std::uint8_t>>(image.samples()), rgbSamples(),
               fixtureTolerance);
}

TEST_F(JpegTest, ReadsBigEndianOrientation) {
    const auto result = codec().decode(fixture("orientation_big.jpg"));
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(result));
    const auto& image = std::get<DecodedImage>(result);
    EXPECT_EQ(image.metadata().orientation, Orientation::Rotate90Counterclockwise);
    EXPECT_EQ(image.width(), imageWidth);
    EXPECT_EQ(image.height(), imageHeight);
}

TEST_F(JpegTest, RejectsUnsupportedInputs) {
    expectError(codec().decode(fixture("cmyk.jpg")), CodecErrorCode::UnsupportedLayout);
    expectError(codec().decode(fixture("ycck.jpg")), CodecErrorCode::UnsupportedLayout);
    expectError(codec().decode(fixture("precision12.jpg")), CodecErrorCode::UnsupportedPrecision);
    expectError(codec().decode(fixture("extended.jpg")), CodecErrorCode::UnsupportedFeature);
    expectError(codec().decode(fixture("arithmetic.jpg")), CodecErrorCode::UnsupportedFeature);
}

TEST_F(JpegTest, RejectsMalformedFiles) {
    expectError(codec().decode(fixture("not_jpeg.bin")), CodecErrorCode::UnsupportedFormat);
    expectError(codec().decode(fixture("truncated.jpg")), CodecErrorCode::InvalidData);
    expectError(codec().decode(fixture("bad_icc.jpg")), CodecErrorCode::InvalidData);
    expectError(codec().decode(fixture("bad_exif.jpg")), CodecErrorCode::InvalidData);
}

TEST_F(JpegTest, EncodesBaselineRgb444) {
    const DecodedImage image(imageWidth, imageHeight, ChannelLayout::RGB,
                             SampleDepth::Bits8, rgbSamples());
    ASSERT_TRUE(std::holds_alternative<std::monostate>(codec().encode(output(), image)));
    const auto info = readSof(readBytes(output()));
    EXPECT_EQ(info.marker, baselineSof);
    EXPECT_EQ(info.sampling, (std::vector<std::uint8_t>{0x11, 0x11, 0x11}));
    const auto decoded = codec().decode(output());
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(decoded));
    const auto& restored = std::get<DecodedImage>(decoded);
    EXPECT_EQ(restored.width(), imageWidth);
    EXPECT_EQ(restored.height(), imageHeight);
    expectNear(std::get<std::vector<std::uint8_t>>(restored.samples()), rgbSamples(),
               encodeTolerance);
}

TEST_F(JpegTest, UsesQualityAndDefaultNinety) {
    const auto source = rgbSamples();
    const DecodedImage image(imageWidth, imageHeight, ChannelLayout::RGB,
                             SampleDepth::Bits8, source);
    ASSERT_TRUE(std::holds_alternative<std::monostate>(codec().encode(output("default.jpg"), image)));
    ASSERT_TRUE(std::holds_alternative<std::monostate>(codec().encode(output("ninety.jpg"), image, {90})));
    EXPECT_EQ(readBytes(output("default.jpg")), readBytes(output("ninety.jpg")));

    ASSERT_TRUE(std::holds_alternative<std::monostate>(codec().encode(output("low.jpg"), image, {10})));
    ASSERT_TRUE(std::holds_alternative<std::monostate>(codec().encode(output("high.jpg"), image, {100})));
    const auto low = codec().decode(output("low.jpg"));
    const auto high = codec().decode(output("high.jpg"));
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(low));
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(high));
    const auto& lowSamples = std::get<std::vector<std::uint8_t>>(
        std::get<DecodedImage>(low).samples());
    const auto& highSamples = std::get<std::vector<std::uint8_t>>(
        std::get<DecodedImage>(high).samples());
    EXPECT_LT(mse(highSamples, source), mse(lowSamples, source));
}

TEST_F(JpegTest, WritesIccAndOrientation) {
    ImageMetadata metadata;
    metadata.orientation = Orientation::Transverse;
    metadata.iccProfile = readBytes(fixture("large_rgb.icc"));
    const auto source = rgbSamples();
    const DecodedImage image(imageWidth, imageHeight, ChannelLayout::RGB,
                             SampleDepth::Bits8, source, metadata);
    ASSERT_TRUE(std::holds_alternative<std::monostate>(codec().encode(output(), image)));
    EXPECT_EQ(image.samples(), SampleBuffer{source});

    const auto result = codec().decode(output());
    ASSERT_TRUE(std::holds_alternative<DecodedImage>(result));
    const auto& restored = std::get<DecodedImage>(result);
    EXPECT_EQ(restored.metadata().orientation, Orientation::Transverse);
    EXPECT_EQ(restored.metadata().iccProfile, metadata.iccProfile);
    expectNear(std::get<std::vector<std::uint8_t>>(restored.samples()), source,
               encodeTolerance);
}

TEST_F(JpegTest, RejectsIncompatibleOutputWithoutTruncation) {
    const std::string sentinel = "existing file";
    {
        std::ofstream file(output(), std::ios::binary);
        file << sentinel;
    }

    const DecodedImage rgba(1, 1, ChannelLayout::RGBA, SampleDepth::Bits8,
                            std::vector<std::uint8_t>{1, 2, 3, 4});
    expectError(codec().encode(output(), rgba), CodecErrorCode::UnsupportedLayout);
    const auto afterRgba = readBytes(output());
    EXPECT_EQ(std::string(afterRgba.begin(), afterRgba.end()), sentinel);

    ImageMetadata metadata;
    metadata.iccProfile = std::vector<std::uint8_t>{1, 2, 3};
    const DecodedImage invalidIcc(imageWidth, imageHeight, ChannelLayout::RGB,
                                  SampleDepth::Bits8, rgbSamples(), metadata);
    expectError(codec().encode(output(), invalidIcc), CodecErrorCode::IncompatibleMetadata);
    const auto bytes = readBytes(output());
    EXPECT_EQ(std::string(bytes.begin(), bytes.end()), sentinel);
}

TEST_F(JpegTest, ReportsFileFailures) {
    expectError(codec().decode(output()), CodecErrorCode::FileOpenFailed);
    const DecodedImage image(imageWidth, imageHeight, ChannelLayout::RGB,
                             SampleDepth::Bits8, rgbSamples());
    expectError(codec().encode(output() / "missing" / "image.jpg", image),
                CodecErrorCode::FileOpenFailed);
    expectError(codec().encode(output().parent_path(), image), CodecErrorCode::FileOpenFailed);
}

TEST_F(JpegTest, ReportsWriteFailure) {
    const std::filesystem::path fullDevice("/dev/full");
    if (!std::filesystem::exists(fullDevice)) {
        GTEST_SKIP() << "No failing-write device on this platform";
    }
    const DecodedImage image(imageWidth, imageHeight, ChannelLayout::RGB,
                             SampleDepth::Bits8, rgbSamples());
    expectError(codec().encode(fullDevice, image), CodecErrorCode::FileWriteFailed);
}

} // namespace
