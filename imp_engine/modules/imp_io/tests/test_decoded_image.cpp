#include "imp_io/DecodedImage.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <gtest/gtest.h>

using namespace imp_io;

namespace {

constexpr std::array layouts{ChannelLayout::RGB, ChannelLayout::RGBA};

template<typename T>
void checkSamples(SampleDepth depth, StorageType storage, const std::vector<T>& values) {
    for (const auto layout : layouts) {
        const auto channels = static_cast<std::size_t>(layout);
        // Two rows expose padding or channel-order mistakes.
        std::vector<T> buffer;
        for (std::size_t index = 0; index < 2 * channels; ++index) {
            buffer.push_back(values[index % values.size()]);
        }

        const DecodedImage image(1, 2, layout, depth, buffer);
        EXPECT_EQ(image.width(), 1u);
        EXPECT_EQ(image.height(), 2u);
        EXPECT_EQ(image.layout(), layout);
        EXPECT_EQ(image.bitDepth(), depth);
        EXPECT_EQ(image.storageType(), storage);
        EXPECT_EQ(std::get<std::vector<T>>(image.samples()), buffer);
    }
}

} // namespace

TEST(DecodedImage, PreservesSupportedSamples) {
    checkSamples<std::uint8_t>(SampleDepth::Bits8, StorageType::UInt8,
                              {0, 17, 128, std::numeric_limits<std::uint8_t>::max()});
    checkSamples<std::uint16_t>(SampleDepth::Bits16, StorageType::UInt16,
                               {0, 257, 32769, std::numeric_limits<std::uint16_t>::max()});
    checkSamples<float>(SampleDepth::Bits32, StorageType::Float32, {-0.5f, 0.0f, 0.25f, 12.5f});
}

TEST(DecodedImage, OwnsSamplesAndCopies) {
    std::vector<std::uint8_t> source{1, 2, 3};
    const DecodedImage image(1, 1, ChannelLayout::RGB, SampleDepth::Bits8, source);
    source[0] = 99;

    const auto copy = image;
    const auto& original = std::get<std::vector<std::uint8_t>>(image.samples());
    const auto& copied = std::get<std::vector<std::uint8_t>>(copy.samples());
    EXPECT_EQ(original, (std::vector<std::uint8_t>{1, 2, 3}));
    EXPECT_EQ(copied, original);
    EXPECT_NE(copied.data(), original.data());
}

TEST(DecodedImage, AcceptsMovedSamples) {
    std::vector<std::uint16_t> source{0, 513, std::numeric_limits<std::uint16_t>::max()};
    const auto expected = source;
    DecodedImage image(1, 1, ChannelLayout::RGB, SampleDepth::Bits16, std::move(source));
    const auto moved = std::move(image);

    EXPECT_EQ(std::get<std::vector<std::uint16_t>>(moved.samples()), expected);
    EXPECT_EQ(moved.bitDepth(), SampleDepth::Bits16);
}

TEST(DecodedImage, LeavesUnknownMetadataAbsent) {
    const DecodedImage image(1, 1, ChannelLayout::RGB, SampleDepth::Bits8,
                             std::vector<std::uint8_t>{1, 2, 3});

    EXPECT_FALSE(image.metadata().orientation.has_value());
    EXPECT_FALSE(image.metadata().iccProfile.has_value());
    EXPECT_FALSE(image.metadata().pngColor.has_value());
}

TEST(DecodedImage, OwnsMetadataWithoutTransforms) {
    constexpr double imageGamma = 0.45455;
    ImageMetadata metadata;
    metadata.orientation = Orientation::Rotate90Clockwise;
    metadata.iccProfile = std::vector<std::uint8_t>{1, 2, 3, 4};
    metadata.pngColor = PngColorDescription{
        imageGamma,
        Chromaticities{{0.3127, 0.3290}, {0.64, 0.33}, {0.30, 0.60}, {0.15, 0.06}},
        RenderingIntent::RelativeColorimetric
    };
    const std::vector<std::uint8_t> samples{1, 2, 3, 4, 5, 6};
    const DecodedImage image(2, 1, ChannelLayout::RGB, SampleDepth::Bits8, samples, metadata);
    metadata.iccProfile->front() = 99;

    const auto& stored = image.metadata();
    EXPECT_EQ(image.width(), 2u);
    EXPECT_EQ(image.height(), 1u);
    EXPECT_EQ(std::get<std::vector<std::uint8_t>>(image.samples()), samples);
    EXPECT_EQ(stored.orientation, Orientation::Rotate90Clockwise);
    ASSERT_TRUE(stored.iccProfile.has_value());
    EXPECT_EQ(*stored.iccProfile, (std::vector<std::uint8_t>{1, 2, 3, 4}));
    ASSERT_TRUE(stored.pngColor.has_value());
    EXPECT_EQ(stored.pngColor->gamma, imageGamma);
    EXPECT_EQ(stored.pngColor->srgbIntent, RenderingIntent::RelativeColorimetric);
    ASSERT_TRUE(stored.pngColor->chromaticities.has_value());
    EXPECT_DOUBLE_EQ(stored.pngColor->chromaticities->white.x, 0.3127);
    EXPECT_DOUBLE_EQ(stored.pngColor->chromaticities->red.y, 0.33);
    EXPECT_DOUBLE_EQ(stored.pngColor->chromaticities->green.y, 0.60);
    EXPECT_DOUBLE_EQ(stored.pngColor->chromaticities->blue.y, 0.06);
}

TEST(DecodedImage, RejectsZeroDimensions) {
    const std::vector<std::uint8_t> empty;
    EXPECT_THROW(DecodedImage(0, 1, ChannelLayout::RGB, SampleDepth::Bits8, empty),
                 std::invalid_argument);
    EXPECT_THROW(DecodedImage(1, 0, ChannelLayout::RGB, SampleDepth::Bits8, empty),
                 std::invalid_argument);
}

TEST(DecodedImage, RejectsUnsupportedLayout) {
    EXPECT_THROW(DecodedImage(1, 1, static_cast<ChannelLayout>(2), SampleDepth::Bits8,
                              std::vector<std::uint8_t>{1, 2}), std::invalid_argument);
}

TEST(DecodedImage, RejectsWrongSampleCount) {
    for (const auto layout : layouts) {
        const auto channels = static_cast<std::size_t>(layout);
        for (const auto count : {std::size_t{0}, channels - 1, channels + 1}) {
            EXPECT_THROW(DecodedImage(1, 1, layout, SampleDepth::Bits8,
                                      std::vector<std::uint8_t>(count)), std::invalid_argument);
        }
    }
}

TEST(DecodedImage, RejectsUnsupportedDepths) {
    struct InvalidDepths {
        SampleBuffer samples;
        std::vector<SampleDepth> depths;
    };
    constexpr auto zeroDepth = static_cast<SampleDepth>(0);
    constexpr auto unsupportedDepth = static_cast<SampleDepth>(10);
    const std::array cases{
        InvalidDepths{std::vector<std::uint8_t>(3),
                      {SampleDepth::Bits16, SampleDepth::Bits32,
                       zeroDepth, unsupportedDepth}},
        InvalidDepths{std::vector<std::uint16_t>(3),
                      {SampleDepth::Bits8, SampleDepth::Bits32, zeroDepth, unsupportedDepth}},
        InvalidDepths{std::vector<float>(3),
                      {SampleDepth::Bits8, SampleDepth::Bits16,
                       zeroDepth, unsupportedDepth}}
    };

    for (const auto& entry : cases) {
        for (const auto depth : entry.depths) {
            EXPECT_THROW(DecodedImage(1, 1, ChannelLayout::RGB, depth, entry.samples),
                         std::invalid_argument);
        }
    }
}

TEST(DecodedImage, RejectsPixelCountOverflow) {
    constexpr auto maxSize = std::numeric_limits<std::size_t>::max();
    EXPECT_THROW(DecodedImage(maxSize, 2, ChannelLayout::RGB, SampleDepth::Bits8,
                              std::vector<std::uint8_t>{}), std::overflow_error);
}

TEST(DecodedImage, RejectsSampleCountOverflow) {
    constexpr auto maxSize = std::numeric_limits<std::size_t>::max();
    EXPECT_THROW(DecodedImage(maxSize, 1, ChannelLayout::RGBA, SampleDepth::Bits8,
                              std::vector<std::uint8_t>{}), std::overflow_error);
}

TEST(DecodedImage, RejectsByteCountOverflow) {
    constexpr auto maxSize = std::numeric_limits<std::size_t>::max();
    constexpr auto channels = static_cast<std::size_t>(ChannelLayout::RGB);
    EXPECT_THROW(DecodedImage(maxSize / channels, 1, ChannelLayout::RGB, SampleDepth::Bits16,
                              std::vector<std::uint16_t>{}), std::overflow_error);
    EXPECT_THROW(DecodedImage(maxSize / channels, 1, ChannelLayout::RGB, SampleDepth::Bits32,
                              std::vector<float>{}), std::overflow_error);
}

TEST(DecodedImage, LeavesFloatPolicyToEncoder) {
    const DecodedImage image(1, 1, ChannelLayout::RGB, SampleDepth::Bits32,
                             std::vector<float>{-1.0f, std::numeric_limits<float>::infinity(),
                                                std::numeric_limits<float>::quiet_NaN()});
    const auto& samples = std::get<std::vector<float>>(image.samples());

    EXPECT_EQ(samples[0], -1.0f);
    EXPECT_TRUE(std::isinf(samples[1]));
    EXPECT_TRUE(std::isnan(samples[2]));
}
