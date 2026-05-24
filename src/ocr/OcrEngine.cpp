#include "ocr/OcrEngine.h"
#include "ocr/ImageLoader.h"
#include "ocr/ImagePreprocessor.h"

// Include Tesseract headers HERE, not in the .h
// This keeps the Pimpl boundary clean.
#include <tesseract/baseapi.h>
#include <tesseract/resultiterator.h>
#include <leptonica/allheaders.h>

#include <stdexcept>
#include <cstdlib>
#include <numeric>

namespace sl {
namespace ocr {

// ─────────────────────────────────────────────────────────────────────
// Custom deleter for Pimpl unique_ptr
//
// This is defined in the .cpp where TessBaseAPI is a complete type.
// The header only sees the forward declaration.
// Attempting to define this in the header would fail because
// unique_ptr's destructor needs a complete type to call delete.
// ─────────────────────────────────────────────────────────────────────

void OcrEngine::TessDeleter::operator()(tesseract::TessBaseAPI* api) const
{
    if (api) {
        api->End();     // Release Tesseract internal resources
        delete api;     // Free the object itself
    }
}

// ─────────────────────────────────────────────────────────────────────
// Constructor
//
// TessBaseAPI::Init() parameters:
//   1. datapath  — directory containing *.traineddata files
//                  Pass nullptr to use TESSDATA_PREFIX environment var
//   2. language  — language code: "eng", "fra", etc.
//   3. oem       — OCR Engine Mode:
//                  OEM_DEFAULT (3) = use best available (LSTM if available)
//                  OEM_LSTM_ONLY (1) = use neural network engine
//                  OEM_TESSERACT_ONLY (0) = use classic engine
//
// Returns -1 on failure (not exception — C API convention).
// We convert to an exception to fit our C++ error handling model.
// ─────────────────────────────────────────────────────────────────────

OcrEngine::OcrEngine(
    const std::string& tessDataPath,
    const std::string& language
)
{
    // Allocate TessBaseAPI via our custom deleter unique_ptr
    m_tess.reset(new tesseract::TessBaseAPI());

    const char* dataPath = tessDataPath.empty()
        ? nullptr                      // use TESSDATA_PREFIX env var
        : tessDataPath.c_str();

    int result = m_tess->Init(
        dataPath,
        language.c_str(),
        tesseract::OEM_DEFAULT
    );

    if (result != 0) {
        m_tess.reset();  // release before throwing
        throw std::runtime_error(
            "OcrEngine: Tesseract initialization failed. "
            "Check that tessdata is installed and TESSDATA_PREFIX is set.\n"
            "  Install with: sudo apt install tesseract-ocr tesseract-ocr-eng\n"
            "  Or set: export TESSDATA_PREFIX=/path/to/tessdata/"
        );
    }

    // Set default page segmentation mode:
    // PSM_AUTO_OSD (1) would detect orientation, but is slower.
    // PSM_AUTO (3) fully automatic, good for general documents.
    m_tess->SetPageSegMode(tesseract::PSM_AUTO);

    m_initialized = true;
}

// ─────────────────────────────────────────────────────────────────────
// Destructor
//
// The unique_ptr<TessBaseAPI, TessDeleter> destructor calls
// TessDeleter::operator() which calls End() then delete.
// We define ~OcrEngine() in the .cpp so the compiler sees
// the complete type of TessBaseAPI when it needs to instantiate
// the unique_ptr destructor.
// ─────────────────────────────────────────────────────────────────────

OcrEngine::~OcrEngine() = default;

// ─────────────────────────────────────────────────────────────────────
// processImage
//
// Tesseract's image input API:
//   SetImage(imagedata, width, height, bytes_per_pixel, bytes_per_line)
//
//   bytes_per_pixel: 1 for grayscale, 3 for RGB
//   bytes_per_line:  width * bytes_per_pixel (row stride)
//                    Important: must match the actual buffer layout
//
// Why bytes_per_line and not just width?
//   Images can be padded for alignment. bytes_per_line lets Tesseract
//   skip padding bytes between rows. Our ImageBuffer has no padding
//   (vector is tightly packed), so bytes_per_line = width * channels.
//
// GetUTF8Text() returns a char* allocated with new[].
// We MUST call delete[] on it — or better, wrap it immediately.
// ─────────────────────────────────────────────────────────────────────

OcrResult OcrEngine::processImage(const ImageBuffer& image)
{
    if (!m_initialized) {
        OcrResult result;
        result.success      = false;
        result.errorMessage = "OcrEngine not initialized";
        return result;
    }

    if (image.empty()) {
        OcrResult result;
        result.success      = false;
        result.errorMessage = "Empty image buffer";
        return result;
    }

    if (image.channels != 1 && image.channels != 3) {
        OcrResult result;
        result.success      = false;
        result.errorMessage = "Image must be grayscale (1ch) or RGB (3ch)";
        return result;
    }

    // Give Tesseract the raw pixel data.
    // Tesseract does NOT take ownership — we keep the ImageBuffer alive.
    m_tess->SetImage(
        image.pixels.data(),
        image.width,
        image.height,
        image.channels,                    // bytes per pixel
        image.width * image.channels       // bytes per line (no padding)
    );

    return extractResults();
}

// ─────────────────────────────────────────────────────────────────────
// processImageWithPreprocessing
// ─────────────────────────────────────────────────────────────────────

OcrResult OcrEngine::processImageWithPreprocessing(const ImageBuffer& image)
{
    ImageBuffer processed = ImagePreprocessor::runFullPipeline(image);
    return processImage(processed);
}

// ─────────────────────────────────────────────────────────────────────
// processFile
// ─────────────────────────────────────────────────────────────────────

OcrResult OcrEngine::processFile(const std::string& filepath)
{
    if (!ImageLoader::isSupported(filepath)) {
        OcrResult result;
        result.success      = false;
        result.errorMessage = "Unsupported image format: " + filepath;
        return result;
    }

    ImageBuffer image = ImageLoader::load(filepath);
    return processImageWithPreprocessing(image);
}

// ─────────────────────────────────────────────────────────────────────
// extractResults
//
// After SetImage() + Recognize(), we extract:
//   1. Full UTF-8 text (GetUTF8Text)
//   2. Per-word results via ResultIterator
//
// ResultIterator:
//   Tesseract's iterator walks the recognized text at different
//   granularities: RIL_BLOCK, RIL_PARA, RIL_TEXTLINE, RIL_WORD, RIL_SYMBOL
//
//   We iterate at RIL_WORD (word level) to get per-word confidence.
//
// Memory management for GetUTF8Text():
//   Returns char* allocated with new[].
//   We wrap it in a unique_ptr<char[]> for automatic deletion.
//   This is the RAII boundary — C API allocation → C++ ownership.
//
// Memory management for ResultIterator:
//   GetIterator() returns a new ResultIterator*.
//   We delete it when done (manual here — we could unique_ptr it too).
// ─────────────────────────────────────────────────────────────────────

OcrResult OcrEngine::extractResults()
{
    OcrResult result;

    // Run recognition (idempotent if called again with same image)
    m_tess->Recognize(nullptr);

    // Get full text — immediately wrap in RAII
    {
        // unique_ptr<char[]> for array-delete
        std::unique_ptr<char[]> rawText(m_tess->GetUTF8Text());
        if (rawText) {
            result.fullText = std::string(rawText.get());
            // rawText deleted here automatically
        }
    }

    // Iterate word by word to collect confidence scores
    tesseract::ResultIterator* iter = m_tess->GetIterator();

    if (iter) {
        float confidenceSum = 0.0f;
        int   wordCount     = 0;
        std::string filteredText;
        filteredText.reserve(result.fullText.size());

        do {
            if (iter->IsAtBeginningOf(tesseract::RIL_WORD)) {
                // Get word text — again, char* that we must free
                std::unique_ptr<char[]> wordText(
                    iter->GetUTF8Text(tesseract::RIL_WORD));

                float conf = iter->Confidence(tesseract::RIL_WORD);

                if (wordText && conf >= 0.0f) {
                    WordResult wr;
                    wr.text       = std::string(wordText.get());
                    wr.confidence = conf;
                    iter->BoundingBox(
                        tesseract::RIL_WORD,
                        &wr.x, &wr.y, &wr.width, &wr.height);
                    // BoundingBox gives (x1,y1,x2,y2) — adjust width/height
                    wr.width  -= wr.x;
                    wr.height -= wr.y;

                    result.words.push_back(wr);

                    confidenceSum += conf;
                    ++wordCount;

                    // Only include high-confidence words in full text
                    if (conf >= m_confidenceThreshold) {
                        filteredText += wr.text;
                        filteredText += ' ';
                    }
                }
            }
        } while (iter->Next(tesseract::RIL_WORD));

        delete iter;

        if (wordCount > 0) {
            result.averageConfidence = confidenceSum / wordCount;
            // Replace full text with confidence-filtered version
            result.fullText = filteredText;
        }
    }

    result.pageCount = 1;
    result.success   = true;
    return result;
}

// ─────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────

void OcrEngine::setConfidenceThreshold(float threshold)
{
    m_confidenceThreshold = threshold;
}

void OcrEngine::setPageSegMode(int mode)
{
    if (m_tess) {
        m_tess->SetPageSegMode(
            static_cast<tesseract::PageSegMode>(mode));
    }
}

bool OcrEngine::isInitialized() const { return m_initialized; }

std::string OcrEngine::getVersion() const
{
    return tesseract::TessBaseAPI::Version();
}

} // namespace ocr
} // namespace sl
