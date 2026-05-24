#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace sl {
namespace ocr {

/**
 * WordResult
 *
 * A single recognized word with its confidence score.
 *
 * Tesseract gives per-word confidence as a float 0..100.
 * We store it as-is for filtering.
 *
 * boundingBox:
 *   [x, y, width, height] in image coordinates.
 *   Useful for generating highlighted PDFs in the future.
 */
struct WordResult
{
    std::string text;
    float       confidence { 0.0f };
    int         x { 0 }, y { 0 };
    int         width { 0 }, height { 0 };
};

/**
 * OcrResult
 *
 * Complete result of processing one image through the OCR engine.
 *
 * fullText:
 *   The entire recognized text as a single string.
 *   Suitable for direct feeding into SearchEngine.indexDocument().
 *
 * words:
 *   Per-word breakdown with confidence scores.
 *   Use for filtering, highlighting, or confidence-based decisions.
 *
 * averageConfidence:
 *   Mean confidence across all recognized words.
 *   Rule of thumb: > 80 = good quality, 60-80 = acceptable,
 *   < 60 = poor quality (image preprocessing likely needed).
 *
 * pageCount:
 *   Number of logical pages recognized (multi-page TIFF support).
 */
struct OcrResult
{
    std::string             fullText;
    std::vector<WordResult> words;
    float                   averageConfidence { 0.0f };
    int                     pageCount         { 0 };
    bool                    success           { false };
    std::string             errorMessage;
};

} // namespace ocr
} // namespace sl
