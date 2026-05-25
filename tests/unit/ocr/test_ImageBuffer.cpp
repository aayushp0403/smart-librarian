#include <gtest/gtest.h>
#include "ocr/ImageBuffer.h"

using namespace sl::ocr;

TEST(ImageBufferTest, DefaultConstructorProducesEmptyBuffer)
{
    ImageBuffer buf;
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.width,    0);
    EXPECT_EQ(buf.height,   0);
    EXPECT_EQ(buf.channels, 0);
}

TEST(ImageBufferTest, ConstructedBufferHasCorrectSize)
{
    ImageBuffer buf(100, 50, 3);  // 100x50 RGB
    EXPECT_FALSE(buf.empty());
    EXPECT_EQ(buf.width,     100);
    EXPECT_EQ(buf.height,    50);
    EXPECT_EQ(buf.channels,  3);
    EXPECT_EQ(buf.byteSize(), 100u * 50u * 3u);
}

TEST(ImageBufferTest, PixelsInitializedToZero)
{
    ImageBuffer buf(10, 10, 1);
    for (uint8_t px : buf.pixels) {
        EXPECT_EQ(px, 0u);
    }
}

TEST(ImageBufferTest, PixelAccessorReturnsCorrectPointer)
{
    ImageBuffer buf(4, 4, 1);  // 4x4 grayscale

    // Set pixel (2, 3) to 128
    *buf.pixel(2, 3) = 128;

    // Verify via direct array access
    // Pixel at (x=2, y=3) in row-major = index 3*4 + 2 = 14
    EXPECT_EQ(buf.pixels[14], 128u);
}

TEST(ImageBufferTest, RgbPixelAccessorAddressesCorrectChannel)
{
    ImageBuffer buf(4, 4, 3);  // 4x4 RGB

    uint8_t* px = buf.pixel(1, 1);
    px[0] = 255;  // R
    px[1] = 128;  // G
    px[2] = 0;    // B

    // Index: (1*4 + 1) * 3 = 15
    EXPECT_EQ(buf.pixels[15], 255u);  // R
    EXPECT_EQ(buf.pixels[16], 128u);  // G
    EXPECT_EQ(buf.pixels[17], 0u);    // B
}
