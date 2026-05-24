
#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <string>

namespace sl {
namespace ocr {

/**
 * ImageBuffer
 *
 * Owns a decoded image as a flat byte array in memory.
 *
 * Memory layout:
 *   Pixels are stored in row-major order.
 *   For an RGB image: R0,G0,B0, R1,G1,B1, ... Rn,Gn,Bn
 *   For grayscale:    Y0, Y1, Y2, ... Yn
 *
 *   Total bytes = width * height * channels
 *
 *   To access pixel (x, y) in a grayscale image:
 *     uint8_t val = pixels[y * width + x];
 *
 *   To access pixel (x, y) in RGB:
 *     uint8_t r = pixels[(y * width + x) * 3 + 0];
 *     uint8_t g = pixels[(y * width + x) * 3 + 1];
 *     uint8_t b = pixels[(y * width + x) * 3 + 2];
 *
 * Ownership:
 *   vector<uint8_t> owns the pixel data.
 *   RAII — no manual memory management needed.
 *   Movable so we can return ImageBuffer from functions
 *   without copying potentially megabytes of pixel data.
 *
 * channels:
 *   1 = grayscale
 *   3 = RGB
 *   4 = RGBA
 */
struct ImageBuffer
{
    std::vector<uint8_t> pixels;
    int width    { 0 };
    int height   { 0 };
    int channels { 0 };

    ImageBuffer() = default;

    ImageBuffer(int w, int h, int c)
        : pixels(static_cast<std::size_t>(w * h * c), 0)
        , width(w)
        , height(h)
        , channels(c)
    {}

    bool empty() const { return pixels.empty(); }

    std::size_t byteSize() const { return pixels.size(); }

    /**
     * pixel — get a pointer to the start of pixel (x, y).
     * For grayscale: dereference directly.
     * For RGB: access [0], [1], [2] for R, G, B.
     */
    uint8_t* pixel(int x, int y) {
        return pixels.data() + (y * width + x) * channels;
    }
    const uint8_t* pixel(int x, int y) const {
        return pixels.data() + (y * width + x) * channels;
    }
};

} // namespace ocr
} // namespace sl
