#pragma once

#include "ocr/ImageBuffer.h"
#include "ocr/OcrResult.h"

#include <string>
#include <memory>

// Forward-declare Tesseract types to avoid exposing
// the full Tesseract headers in our public interface.
// Callers of OcrEngine don't need to know about Tesseract.
// This is the Pimpl (pointer-to-implementation) pattern.
namespace tesseract { class TessBaseAPI; }

namespace sl {
namespace ocr {

/**
 * OcrEngine
 *
 * RAII wrapper around the Tesseract OCR engine.
 *
 * Tesseract initialization (~100-300ms) loads:
 *   - The language model (eng.traineddata ~10MB)
 *   - Neural network weights
 *   - Character classifier data
 *
 * This is expensive. We initialize ONCE in the constructor
 * and reuse the same TessBaseAPI instance for all images.
 *
 * Pimpl pattern (pointer-to-implementation):
 *   We forward-declare TessBaseAPI and store it as a
 *   unique_ptr<tesseract::TessBaseAPI> in the .cpp file.
 *   This means:
 *     1. The Tesseract headers are not included in OcrEngine.h
 *     2. Code that #includes OcrEngine.h does NOT need to find
 *        Tesseract headers at compile time
 *     3. Changing the Tesseract implementation doesn't cause
 *        recompilation of every file that includes OcrEngine.h
 *
 * Pimpl is a C++ technique to reduce compilation dependencies
 * and hide implementation details. It's a standard idiom in
 * professional codebases and worth knowing for interviews.
 *
 * Thread safety:
 *   TessBaseAPI is NOT thread-safe. Do not share one OcrEngine
 *   across threads. For multi-threaded OCR, create one OcrEngine
 *   per thread. (We'll address this in a later phase.)
 */
class OcrEngine
{
public:
    /**
     * Constructor — initializes Tesseract.
     *
     * @param tessDataPath  path to tessdata/ directory
     *                      (e.g., "/usr/share/tesseract-ocr/4.00/tessdata")
     * @param language      language code (e.g., "eng", "fra", "deu")
     * @throws              std::runtime_error if initialization fails
     */
    explicit OcrEngine(
        const std::string& tessDataPath = "",
        const std::string& language     = "eng"
    );

    /**
     * Destructor — calls TessBaseAPI::End() to release all resources.
     * Defined in .cpp because TessBaseAPI is incomplete in header.
     */
    ~OcrEngine();

    // Non-copyable: Tesseract API state cannot be duplicated
    OcrEngine(const OcrEngine&)            = delete;
    OcrEngine& operator=(const OcrEngine&) = delete;

    // Movable: transfer ownership of TessBaseAPI
    OcrEngine(OcrEngine&&)            = default;
    OcrEngine& operator=(OcrEngine&&) = default;

    // ─────────────────────────────────────────
    // Recognition
    // ─────────────────────────────────────────

    /**
     * processImage — run OCR on a preprocessed ImageBuffer.
     *
     * The image should already be:
     *   - Single channel (grayscale or binary)
     *   - High contrast
     *   - Sufficient resolution (text height >= 20px recommended)
     *
     * For raw images, use processImageWithPreprocessing().
     *
     * @param image  preprocessed grayscale/binary ImageBuffer
     * @return       OcrResult with text and per-word confidence
     */
    OcrResult processImage(const ImageBuffer& image);

    /**
     * processImageWithPreprocessing — load, preprocess, and OCR.
     *
     * Convenience method that runs the full pipeline:
     *   ImageBuffer → runFullPipeline() → processImage()
     *
     * @param image  raw loaded image (any channel count)
     * @return       OcrResult
     */
    OcrResult processImageWithPreprocessing(const ImageBuffer& image);

    /**
     * processFile — load a file from disk and run OCR on it.
     *
     * @param filepath  path to image file
     * @return          OcrResult
     */
    OcrResult processFile(const std::string& filepath);

    // ─────────────────────────────────────────
    // Configuration
    // ─────────────────────────────────────────

    /**
     * setConfidenceThreshold — words below this confidence (0..100)
     * are excluded from fullText but remain in the words vector.
     * Default: 60.0f
     */
    void setConfidenceThreshold(float threshold);

    /**
     * setPageSegMode — Tesseract page segmentation mode.
     * Common values:
     *   PSM_SINGLE_BLOCK (6) — default, treat as single text block
     *   PSM_AUTO (3)         — fully automatic page segmentation
     *   PSM_SINGLE_LINE (7)  — single line of text
     *   PSM_SINGLE_WORD (8)  — single word
     */
    void setPageSegMode(int mode);

    /**
     * isInitialized — returns true if Tesseract loaded successfully.
     */
    bool isInitialized() const;

    /**
     * getVersion — returns Tesseract library version string.
     */
    std::string getVersion() const;

private:
    // Pimpl: unique_ptr to the Tesseract API object.
    // Requires a custom deleter because the destructor needs
    // TessBaseAPI to be a complete type — defined in the .cpp.
    struct TessDeleter {
        void operator()(tesseract::TessBaseAPI* api) const;
    };

    std::unique_ptr<tesseract::TessBaseAPI, TessDeleter> m_tess;
    float  m_confidenceThreshold { 60.0f };
    bool   m_initialized         { false };

    /**
     * extractResults — pull text and confidence from Tesseract
     * after recognition, build OcrResult.
     */
    OcrResult extractResults();
};

} // namespace ocr
} // namespace sl
