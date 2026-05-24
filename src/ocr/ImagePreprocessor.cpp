#include "ocr/ImagePreprocessor.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <cmath>
#include <numeric>

namespace sl {
namespace ocr {

// ─────────────────────────────────────────────────────────────────────
// toGrayscale
//
// ITU-R BT.601 luminance:  Y = 0.299R + 0.587G + 0.114B
//
// We use integer arithmetic with fixed-point scaling to avoid
// floating-point ops in the inner loop (performance):
//   Y ≈ (299*R + 587*G + 114*B) / 1000
//
// This is equivalent but avoids float multiplication per pixel.
// For a 2000x2000 image = 4M pixels, this matters.
// ─────────────────────────────────────────────────────────────────────

ImageBuffer ImagePreprocessor::toGrayscale(const ImageBuffer& src)
{
    if (src.channels == 1) {
        // Already grayscale — return a copy
        return src;
    }

    if (src.channels != 3 && src.channels != 4) {
        throw std::invalid_argument(
            "toGrayscale: expected 1, 3, or 4 channel image, got " +
            std::to_string(src.channels)
        );
    }

    ImageBuffer dst(src.width, src.height, 1);

    const uint8_t* srcPtr = src.pixels.data();
    uint8_t*       dstPtr = dst.pixels.data();

    int totalPixels = src.width * src.height;

    for (int i = 0; i < totalPixels; ++i) {
        uint8_t r = srcPtr[i * src.channels + 0];
        uint8_t g = srcPtr[i * src.channels + 1];
        uint8_t b = srcPtr[i * src.channels + 2];
        // RGBA: channel 3 is alpha — we ignore it

        // Fixed-point luminance: scale by 1000, use integer multiply
        uint32_t y = 299u * r + 587u * g + 114u * b;
        dstPtr[i] = static_cast<uint8_t>(y / 1000u);
    }

    return dst;
}

// ─────────────────────────────────────────────────────────────────────
// binarize — Otsu's global thresholding
//
// Algorithm:
//   1. Compute 256-bin histogram of pixel intensities
//   2. For each possible threshold T (0..255):
//      a. Split pixels into class 0 (≤T) and class 1 (>T)
//      b. Compute mean and weight (fraction of pixels) of each class
//      c. Compute between-class variance:
//         σ²_B = w0 * w1 * (μ0 - μ1)²
//      d. Track the T that maximizes σ²_B
//   3. Apply threshold T to produce binary image
//
// Complexity: O(256) for histogram scan + O(n) for application
//   = O(n) overall where n = pixel count
//
// The key insight: maximizing between-class variance is equivalent
// to minimizing within-class variance (the two sum to total variance).
// Otsu proved this in 1979. It's still the standard approach.
// ─────────────────────────────────────────────────────────────────────

ImageBuffer ImagePreprocessor::binarize(const ImageBuffer& src)
{
    if (src.channels != 1) {
        throw std::invalid_argument("binarize: requires grayscale (1ch) image");
    }

    const int totalPixels = src.width * src.height;
    if (totalPixels == 0) return src;

    // Step 1: Build histogram
    std::array<uint32_t, 256> hist{};
    hist.fill(0);
    for (uint8_t val : src.pixels) {
        ++hist[val];
    }

    // Step 2: Find optimal threshold via Otsu's method
    // We iterate all 256 possible thresholds.
    // For each, compute between-class variance.

    double totalSum = 0.0;
    for (int i = 0; i < 256; ++i) {
        totalSum += static_cast<double>(i) * hist[i];
    }

    double sumBackground   = 0.0;
    uint32_t wBackground   = 0;
    double maxVariance     = 0.0;
    int    threshold       = 128;  // fallback

    for (int t = 0; t < 256; ++t) {
        wBackground += hist[static_cast<std::size_t>(t)];
        if (wBackground == 0) continue;

        uint32_t wForeground =
            static_cast<uint32_t>(totalPixels) - wBackground;
        if (wForeground == 0) break;

        sumBackground +=
            static_cast<double>(t) * hist[static_cast<std::size_t>(t)];

        double meanBackground = sumBackground / wBackground;
        double meanForeground =
            (totalSum - sumBackground) / wForeground;

        // Between-class variance (without the 1/N² scaling,
        // since we only need to compare relative values)
        double diff = meanBackground - meanForeground;
        double variance =
            static_cast<double>(wBackground) *
            static_cast<double>(wForeground) *
            diff * diff;

        if (variance > maxVariance) {
            maxVariance = variance;
            threshold   = t;
        }
    }

    // Step 3: Apply threshold
    ImageBuffer dst(src.width, src.height, 1);
    for (int i = 0; i < totalPixels; ++i) {
        dst.pixels[static_cast<std::size_t>(i)] =
            (src.pixels[static_cast<std::size_t>(i)] <= threshold) ? 0u : 255u;
    }

    return dst;
}

// ─────────────────────────────────────────────────────────────────────
// removeNoise — 3x3 median filter
//
// For each pixel, collect its 3x3 neighborhood (9 values),
// sort them, take the middle value (index 4).
//
// Edge pixels (x < 1, x > w-2, y < 1, y > h-2) are copied
// unchanged — we don't have a full 3x3 neighborhood for them.
//
// Sorting 9 elements: we use std::sort on a stack array.
// std::sort on 9 elements typically uses insertion sort
// (small-size optimization) — extremely fast.
//
// Alternative: partial_sort or nth_element for median,
// but for n=9, std::sort is fine.
// ─────────────────────────────────────────────────────────────────────

ImageBuffer ImagePreprocessor::removeNoise(const ImageBuffer& src)
{
    if (src.channels != 1) {
        throw std::invalid_argument("removeNoise: requires 1-channel image");
    }

    ImageBuffer dst(src.width, src.height, 1);

    // Copy borders unchanged
    // Inner pixels: apply median filter
    for (int y = 0; y < src.height; ++y) {
        for (int x = 0; x < src.width; ++x) {
            if (x == 0 || x == src.width  - 1 ||
                y == 0 || y == src.height - 1)
            {
                // Border pixel — copy as-is
                dst.pixels[static_cast<std::size_t>(y * src.width + x)] =
                    src.pixels[static_cast<std::size_t>(y * src.width + x)];
                continue;
            }

            // Collect 3x3 neighborhood
            std::array<uint8_t, 9> neighborhood{};
            int idx = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = x + dx;
                    int ny = y + dy;
                    neighborhood[static_cast<std::size_t>(idx++)] =
                        src.pixels[static_cast<std::size_t>(
                            ny * src.width + nx)];
                }
            }

            // Sort and take median (index 4 of 9)
            std::sort(neighborhood.begin(), neighborhood.end());
            dst.pixels[static_cast<std::size_t>(y * src.width + x)] =
                neighborhood[4];
        }
    }

    return dst;
}

// ─────────────────────────────────────────────────────────────────────
// scale — nearest-neighbor interpolation
//
// For each output pixel (ox, oy), find the corresponding
// source pixel using the inverse mapping:
//   sx = floor(ox / factor)
//   sy = floor(oy / factor)
//
// Nearest-neighbor: sample the single closest source pixel.
// No blending, no filtering. Sharp edges preserved.
//
// Why nearest-neighbor for text?
//   Text is binary (black/white). Bilinear interpolation would
//   create gray anti-aliased pixels at edges, which confuse
//   Tesseract's character classifier expecting clean binary images.
// ─────────────────────────────────────────────────────────────────────

ImageBuffer ImagePreprocessor::scale(const ImageBuffer& src, float factor)
{
    if (factor <= 0.0f) {
        throw std::invalid_argument("scale: factor must be positive");
    }

    int newWidth  = static_cast<int>(src.width  * factor);
    int newHeight = static_cast<int>(src.height * factor);

    if (newWidth <= 0 || newHeight <= 0) {
        throw std::invalid_argument("scale: resulting dimensions are zero");
    }

    ImageBuffer dst(newWidth, newHeight, src.channels);

    for (int oy = 0; oy < newHeight; ++oy) {
        for (int ox = 0; ox < newWidth; ++ox) {
            // Inverse map: output → source
            int sx = static_cast<int>(ox / factor);
            int sy = static_cast<int>(oy / factor);

            // Clamp to valid source range
            sx = std::min(sx, src.width  - 1);
            sy = std::min(sy, src.height - 1);

            // Copy all channels
            for (int c = 0; c < src.channels; ++c) {
                dst.pixels[static_cast<std::size_t>(
                    (oy * newWidth + ox) * src.channels + c)] =
                src.pixels[static_cast<std::size_t>(
                    (sy * src.width  + sx) * src.channels + c)];
            }
        }
    }

    return dst;
}

// ─────────────────────────────────────────────────────────────────────
// normalizeContrast — histogram stretching
//
// Find the minimum and maximum pixel value in the image.
// Linearly remap so min → 0, max → 255.
//
// Edge case: if min == max (all pixels identical), return unchanged.
// ─────────────────────────────────────────────────────────────────────

ImageBuffer ImagePreprocessor::normalizeContrast(const ImageBuffer& src)
{
    if (src.channels != 1) {
        throw std::invalid_argument(
            "normalizeContrast: requires grayscale image");
    }

    if (src.pixels.empty()) return src;

    // Find min and max in one pass
    auto [minIt, maxIt] = std::minmax_element(
        src.pixels.begin(), src.pixels.end());

    uint8_t minVal = *minIt;
    uint8_t maxVal = *maxIt;

    if (minVal == maxVal) {
        // Flat image — return copy unchanged
        return src;
    }

    ImageBuffer dst(src.width, src.height, 1);

    float range = static_cast<float>(maxVal - minVal);

    for (std::size_t i = 0; i < src.pixels.size(); ++i) {
        float normalized =
            (static_cast<float>(src.pixels[i]) - minVal) / range * 255.0f;
        dst.pixels[i] = static_cast<uint8_t>(
            std::min(255.0f, std::max(0.0f, normalized)));
    }

    return dst;
}

// ─────────────────────────────────────────────────────────────────────
// runFullPipeline
// ─────────────────────────────────────────────────────────────────────

ImageBuffer ImagePreprocessor::runFullPipeline(
    const ImageBuffer& src,
    float              upscaleFactor
)
{
    // Stage 1: convert to grayscale
    ImageBuffer gray = toGrayscale(src);

    // Stage 2: normalize contrast (before binarization)
    ImageBuffer normalized = normalizeContrast(gray);

    // Stage 3: scale up (before binarization — more pixels = better Otsu)
    ImageBuffer scaled = (upscaleFactor != 1.0f)
        ? scale(normalized, upscaleFactor)
        : std::move(normalized);

    // Stage 4: Otsu binarization
    ImageBuffer binary = binarize(scaled);

    // Stage 5: median noise removal
    ImageBuffer clean = removeNoise(binary);

    return clean;
}

} // namespace ocr
} // namespace sl
