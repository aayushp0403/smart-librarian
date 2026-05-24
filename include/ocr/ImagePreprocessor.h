#pragma once

#include "ocr/ImageBuffer.h"

namespace sl {
namespace ocr {

/**
 * ImagePreprocessor
 *
 * Transforms raw images into forms that Tesseract can read
 * with higher accuracy.
 *
 * Why preprocessing matters:
 *   Tesseract was trained on clean, high-contrast black-on-white
 *   text. Real scans have:
 *     - Color backgrounds (distracting, wastes processing)
 *     - Low contrast (gray text on gray background)
 *     - Noise (scanner artifacts, dust specks)
 *     - Skew (slightly tilted documents)
 *
 *   Each preprocessing step removes one source of noise.
 *   A good preprocessing pipeline can improve OCR accuracy
 *   from 70% to 95%+ on typical document scans.
 *
 * All operations produce a new ImageBuffer (immutable inputs).
 * This makes the pipeline composable and testable:
 *   auto gray    = ImagePreprocessor::toGrayscale(original);
 *   auto binary  = ImagePreprocessor::binarize(gray);
 *   auto cleaned = ImagePreprocessor::removeNoise(binary);
 */
class ImagePreprocessor
{
public:
    /**
     * toGrayscale — convert RGB/RGBA image to single-channel grayscale.
     *
     * Algorithm: ITU-R BT.601 luminance formula
     *   Y = 0.299*R + 0.587*G + 0.114*B
     *
     * These weights reflect human perception:
     *   Green contributes most to perceived brightness (~59%)
     *   Red contributes ~30%, Blue only ~11%
     *
     * Why not simple average (R+G+B)/3?
     *   The average treats all channels equally, but human eyes
     *   are most sensitive to green light. The weighted formula
     *   produces perceptually correct brightness values.
     *
     * If input is already grayscale (1 channel), returns a copy.
     *
     * @param  src  RGB (3ch) or RGBA (4ch) image
     * @return      new grayscale (1ch) ImageBuffer
     */
    static ImageBuffer toGrayscale(const ImageBuffer& src);

    /**
     * binarize — convert grayscale to black-and-white using
     * Otsu's thresholding algorithm.
     *
     * Otsu's method finds the threshold T that minimizes the
     * weighted intra-class variance of the two pixel classes
     * (foreground/background).
     *
     * Result:
     *   pixels <= T  →  0   (black = text)
     *   pixels >  T  →  255 (white = background)
     *
     * Why Otsu over a fixed threshold (e.g., 128)?
     *   A fixed threshold fails on images with varying background
     *   brightness. A lightly scanned page might have background
     *   at 200 — a threshold of 128 would mark it all as black.
     *   Otsu adapts to the actual image histogram.
     *
     * @param  src  grayscale (1ch) image
     * @return      binary (1ch) image with values 0 or 255
     */
    static ImageBuffer binarize(const ImageBuffer& src);

    /**
     * removeNoise — remove isolated small pixel groups.
     *
     * Algorithm: 3x3 median filter.
     *   For each pixel, replace its value with the median of its
     *   3x3 neighborhood (9 pixels).
     *
     * Why median and not mean (blur)?
     *   Mean blurring spreads noise into neighbors, making edges fuzzy.
     *   Median filtering removes isolated noise pixels while preserving
     *   sharp text edges. Critical for OCR — blurry text characters
     *   become ambiguous.
     *
     * @param  src  binary or grayscale (1ch) image
     * @return      new denoised image
     */
    static ImageBuffer removeNoise(const ImageBuffer& src);

    /**
     * scale — resize image by a factor.
     *
     * Tesseract works best on images where text height is
     * approximately 30-50 pixels. Small text scanned at 72 DPI
     * may have character heights of only 8-10 pixels — too small.
     * Scaling up 2x dramatically improves recognition on small text.
     *
     * Algorithm: nearest-neighbor interpolation (fast, no blurring).
     * For upscaling text images, nearest-neighbor preserves the
     * hard edges of characters better than bilinear interpolation.
     *
     * @param src     source image
     * @param factor  scale factor (2.0 = double size)
     * @return        new scaled image
     */
    static ImageBuffer scale(const ImageBuffer& src, float factor);

    /**
     * normalizeContrast — stretch the histogram so the darkest
     * pixel becomes 0 and the brightest becomes 255.
     *
     * Formula for each pixel p:
     *   p_out = (p - min) * 255 / (max - min)
     *
     * Why this helps:
     *   A faded photocopy might have pixel range 80-200.
     *   After normalization: range 0-255. Much higher contrast.
     *   Tesseract's character classifier works better with
     *   high-contrast, full-range images.
     *
     * @param  src  grayscale (1ch) image
     * @return      contrast-normalized image
     */
    static ImageBuffer normalizeContrast(const ImageBuffer& src);

    /**
     * runFullPipeline — apply the complete preprocessing sequence.
     *
     * Order matters:
     *   1. toGrayscale     (reduce channels first)
     *   2. normalizeContrast (before binarization for better threshold)
     *   3. scale           (before binarization for better noise removal)
     *   4. binarize        (Otsu on high-contrast scaled image)
     *   5. removeNoise     (on binary image)
     *
     * @param src  original loaded image (any channel count)
     * @param upscaleFactor  scale factor before binarization (default 2.0)
     * @return      preprocessed binary image ready for Tesseract
     */
    static ImageBuffer runFullPipeline(const ImageBuffer& src,
                                       float upscaleFactor = 2.0f);
};

} // namespace ocr
} // namespace sl
