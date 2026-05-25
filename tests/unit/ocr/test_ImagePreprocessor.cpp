#include <gtest/gtest.h>
#include "ocr/ImagePreprocessor.h"
#include "ocr/ImageBuffer.h"

#include <algorithm>
#include <numeric>

using namespace sl::ocr;

// ─────────────────────────────────────────────────────────────────────
// Helper: create a test image with known pixel values
// ─────────────────────────────────────────────────────────────────────

static ImageBuffer makeGrayscale(int w, int h, uint8_t fill = 128)
{
    ImageBuffer buf(w, h, 1);
    std::fill(buf.pixels.begin(), buf.pixels.end(), fill);
    return buf;
}

static ImageBuffer makeRgb(int w, int h,
                            uint8_t r, uint8_t g, uint8_t b)
{
    ImageBuffer buf(w, h, 3);
    for (int i = 0; i < w * h; ++i) {
        buf.pixels[static_cast<std::size_t>(i * 3 + 0)] = r;
        buf.pixels[static_cast<std::size_t>(i * 3 + 1)] = g;
        buf.pixels[static_cast<std::size_t>(i * 3 + 2)] = b;
    }
    return buf;
}

// ─────────────────────────────────────────────────────────────────────
// toGrayscale tests
// ─────────────────────────────────────────────────────────────────────

TEST(ImagePreprocessorTest, GrayscaleInputReturnsCopy)
{
    ImageBuffer gray = makeGrayscale(10, 10, 100);
    ImageBuffer result = ImagePreprocessor::toGrayscale(gray);

    EXPECT_EQ(result.channels, 1);
    EXPECT_EQ(result.width,    10);
    EXPECT_EQ(result.height,   10);
    EXPECT_EQ(result.pixels[0], 100u);
}

TEST(ImagePreprocessorTest, RgbToGrayscaleProducesOneChannel)
{
    ImageBuffer rgb = makeRgb(8, 8, 200, 100, 50);
    ImageBuffer gray = ImagePreprocessor::toGrayscale(rgb);

    EXPECT_EQ(gray.channels, 1);
    EXPECT_EQ(gray.byteSize(), 8u * 8u);
}

TEST(ImagePreprocessorTest, GrayscaleOfPureWhiteIsNearWhite)
{
    // Pure white RGB: (255, 255, 255)
    // Y = 0.299*255 + 0.587*255 + 0.114*255 = 255
    ImageBuffer white = makeRgb(4, 4, 255, 255, 255);
    ImageBuffer gray  = ImagePreprocessor::toGrayscale(white);
    EXPECT_EQ(gray.pixels[0], 255u);
}

TEST(ImagePreprocessorTest, GrayscaleOfPureBlackIsZero)
{
    ImageBuffer black = makeRgb(4, 4, 0, 0, 0);
    ImageBuffer gray  = ImagePreprocessor::toGrayscale(black);
    EXPECT_EQ(gray.pixels[0], 0u);
}

TEST(ImagePreprocessorTest, WrongChannelCountThrows)
{
    ImageBuffer bad(4, 4, 2);  // 2-channel image — not supported
    EXPECT_THROW(
        ImagePreprocessor::toGrayscale(bad),
        std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────
// binarize tests
// ─────────────────────────────────────────────────────────────────────

TEST(ImagePreprocessorTest, BinarizeProducesOnlyZeroAndMax)
{
    // Create a gradient image: pixels from 0 to 255
    ImageBuffer gradient(256, 1, 1);
    for (int i = 0; i < 256; ++i) {
        gradient.pixels[static_cast<std::size_t>(i)] =
            static_cast<uint8_t>(i);
    }

    ImageBuffer binary = ImagePreprocessor::binarize(gradient);

    // Every pixel must be exactly 0 or 255 — nothing in between
    for (uint8_t px : binary.pixels) {
        EXPECT_TRUE(px == 0u || px == 255u)
            << "Binarized pixel has unexpected value: "
            << static_cast<int>(px);
    }
}

TEST(ImagePreprocessorTest, AllWhiteImageBinarizesToWhite)
{
    // All pixels = 255 → Otsu can't split → all remain 255
    ImageBuffer white = makeGrayscale(10, 10, 255);
    ImageBuffer binary = ImagePreprocessor::binarize(white);
    for (uint8_t px : binary.pixels) {
        EXPECT_EQ(px, 255u);
    }
}

TEST(ImagePreprocessorTest, BinarizeRequiresGrayscale)
{
    ImageBuffer rgb = makeRgb(4, 4, 128, 128, 128);
    EXPECT_THROW(
        ImagePreprocessor::binarize(rgb),
        std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────
// scale tests
// ─────────────────────────────────────────────────────────────────────

TEST(ImagePreprocessorTest, Scale2xDoublesDimensions)
{
    ImageBuffer src = makeGrayscale(10, 10, 200);
    ImageBuffer scaled = ImagePreprocessor::scale(src, 2.0f);

    EXPECT_EQ(scaled.width,  20);
    EXPECT_EQ(scaled.height, 20);
    EXPECT_EQ(scaled.channels, 1);
}

TEST(ImagePreprocessorTest, Scale05xHalvesDimensions)
{
    ImageBuffer src = makeGrayscale(20, 20, 100);
    ImageBuffer scaled = ImagePreprocessor::scale(src, 0.5f);

    EXPECT_EQ(scaled.width,  10);
    EXPECT_EQ(scaled.height, 10);
}

TEST(ImagePreprocessorTest, ScalePreservesPixelValues)
{
    // Solid color image — all pixels should remain same after scaling
    ImageBuffer src = makeGrayscale(4, 4, 150);
    ImageBuffer scaled = ImagePreprocessor::scale(src, 2.0f);

    for (uint8_t px : scaled.pixels) {
        EXPECT_EQ(px, 150u);
    }
}

TEST(ImagePreprocessorTest, ZeroScaleFactorThrows)
{
    ImageBuffer src = makeGrayscale(4, 4, 100);
    EXPECT_THROW(
        ImagePreprocessor::scale(src, 0.0f),
        std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────
// normalizeContrast tests
// ─────────────────────────────────────────────────────────────────────

TEST(ImagePreprocessorTest, NormalizeStretchesHistogram)
{
    // Image with pixels in range [50, 150]
    ImageBuffer src(4, 1, 1);
    src.pixels[0] = 50;
    src.pixels[1] = 100;
    src.pixels[2] = 125;
    src.pixels[3] = 150;

    ImageBuffer normalized = ImagePreprocessor::normalizeContrast(src);

    // Min (50) → 0, Max (150) → 255
    EXPECT_EQ(normalized.pixels[0], 0u);
    EXPECT_EQ(normalized.pixels[3], 255u);
    // Values in between should be stretched proportionally
    EXPECT_GT(normalized.pixels[1], 0u);
    EXPECT_LT(normalized.pixels[1], 255u);
}

TEST(ImagePreprocessorTest, NormalizeFlatImageUnchanged)
{
    // All same value — nothing to stretch
    ImageBuffer src = makeGrayscale(5, 5, 128);
    ImageBuffer normalized = ImagePreprocessor::normalizeContrast(src);

    // Should return unchanged (all same value)
    for (uint8_t px : normalized.pixels) {
        EXPECT_EQ(px, 128u);
    }
}

// ─────────────────────────────────────────────────────────────────────
// Full pipeline test
// ─────────────────────────────────────────────────────────────────────

TEST(ImagePreprocessorTest, FullPipelineProducesGrayscaleBinaryOutput)
{
    // Start with an RGB image
    ImageBuffer rgb = makeRgb(20, 20, 180, 180, 180);

    ImageBuffer result = ImagePreprocessor::runFullPipeline(rgb, 1.0f);

    // Result should be single channel
    EXPECT_EQ(result.channels, 1);

    // Result should be binary (only 0 or 255)
    for (uint8_t px : result.pixels) {
        EXPECT_TRUE(px == 0u || px == 255u);
    }
}
