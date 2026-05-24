/**
 * SmartLibrarian — Phase 3 Entry Point
 *
 * Demonstrates:
 *   Phase 1: Custom PDF engine
 *   Phase 2: Search engine
 *   Phase 3: OCR pipeline (Tesseract + preprocessing)
 *
 * Full pipeline:
 *   Image file → OcrEngine → text → SearchEngine → indexed
 *   Query → SearchEngine → ranked results → PDF report
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include "core/Application.h"
#include "pdf/PdfDocument.h"
#include "search/SearchEngine.h"
#include "ocr/OcrEngine.h"
#include "ocr/ImageLoader.h"
#include "utils/Logger.h"

// ─────────────────────────────────────────────────────────────────────
// Helper: print OCR result summary
// ─────────────────────────────────────────────────────────────────────

static void printOcrResult(const sl::ocr::OcrResult& result,
                            const std::string& source)
{
    std::cout << "\n── OCR Result: " << source << " ──────────────────\n";
    if (!result.success) {
        std::cout << "  FAILED: " << result.errorMessage << "\n";
        return;
    }

    std::cout << "  Words recognized : " << result.words.size()     << "\n";
    std::cout << "  Avg confidence   : " << std::fixed
              << std::setprecision(1) << result.averageConfidence    << "%\n";
    std::cout << "  Text preview     : "
              << result.fullText.substr(0,
                     std::min(result.fullText.size(), std::size_t(120)))
              << "...\n";
}

// ─────────────────────────────────────────────────────────────────────
// Helper: print search results
// ─────────────────────────────────────────────────────────────────────

static void printResults(
    const std::vector<sl::search::SearchResult>& results,
    const std::string& query)
{
    std::cout << "\n  Query: \"" << query << "\"\n";
    if (results.empty()) {
        std::cout << "  No results.\n";
        return;
    }
    for (std::size_t i = 0; i < results.size(); ++i) {
        std::cout << "  " << (i+1) << ". [" << std::fixed
                  << std::setprecision(3) << results[i].score << "] "
                  << results[i].title << "\n";
    }
}

// ─────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    using namespace sl;

    utils::Logger::info("SmartLibrarian — Phase 3: OCR Pipeline");

    // ── Initialize OCR Engine ────────────────────────────────────
    // Expensive — do this ONCE. ~100-300ms on first call.
    utils::Logger::info("Initializing Tesseract OCR engine...");

    std::unique_ptr<ocr::OcrEngine> ocrEngine;
    bool ocrAvailable = false;

    try {
        ocrEngine = std::make_unique<ocr::OcrEngine>("", "eng");
        ocrAvailable = true;
        utils::Logger::info(
            "Tesseract initialized. Version: " + ocrEngine->getVersion());
    }
    catch (const std::exception& ex) {
        utils::Logger::warning(
            std::string("OCR init failed (demo will use synthetic text): ")
            + ex.what());
    }

    // ── Initialize Search Engine ─────────────────────────────────
    search::SearchEngine searchEngine;

    // ── Process Images via OCR ───────────────────────────────────
    const std::string testImagePath =
        "../../assets/test_images/test_document.png";

    if (ocrAvailable) {
        utils::Logger::info("Processing test image: " + testImagePath);

        // Check if the test image exists
        if (ocr::ImageLoader::isSupported(testImagePath)) {
            try {
                ocr::OcrResult result = ocrEngine->processFile(testImagePath);
                printOcrResult(result, "test_document.png");

                if (result.success && !result.fullText.empty()) {
                    // Feed OCR output directly into the search engine
                    search::DocumentID id = searchEngine.indexDocument(
                        testImagePath,
                        "OCR: Test Document",
                        result.fullText
                    );
                    utils::Logger::info(
                        "Indexed OCR document [ID=" +
                        std::to_string(id) + "]"
                    );
                }
            }
            catch (const std::exception& ex) {
                utils::Logger::error(
                    std::string("OCR processing error: ") + ex.what());
            }
        } else {
            utils::Logger::warning(
                "Test image not found at: " + testImagePath +
                "\nRun: convert -size 800x600 -background white -fill black "
                "-font DejaVu-Sans -pointsize 24 -gravity NorthWest "
                "-annotate +40+40 'OCR test text' "
                "assets/test_images/test_document.png"
            );
        }
    }

    // ── Also index synthetic documents for richer demo ───────────
    utils::Logger::info("Indexing synthetic document corpus...");

    struct Doc { const char* path; const char* title; const char* content; };
    static const Doc docs[] = {
        {
            "docs/neural.txt",
            "Neural Networks",
            "Neural networks use interconnected layers of neurons for deep learning. "
            "Backpropagation computes gradients to train neural network weights. "
            "Convolutional neural networks process image data efficiently."
        },
        {
            "docs/cpp_memory.txt",
            "C++ Memory Management",
            "Stack allocation is automatic and fast in C++. Heap allocation uses "
            "new and delete. Smart pointers provide automatic memory management. "
            "RAII ensures resources release when objects go out of scope."
        },
        {
            "docs/algorithms.txt",
            "Algorithm Complexity",
            "Binary search achieves O(log n) complexity on sorted arrays. "
            "Quicksort averages O(n log n). Hash tables provide O(1) lookup. "
            "Dynamic programming breaks problems into overlapping subproblems."
        },
        {
            "docs/ocr_tech.txt",
            "OCR Technology",
            "Optical character recognition extracts text from scanned images. "
            "Tesseract uses neural networks to recognize characters. "
            "Image preprocessing improves OCR accuracy significantly. "
            "Binarization and noise removal are critical preprocessing steps."
        }
    };

    for (const auto& doc : docs) {
        search::DocumentID id = searchEngine.indexDocument(
            doc.path, doc.title, doc.content);
        std::cout << "  Indexed [ID=" << id << "] " << doc.title << "\n";
    }

    std::cout << "\n  Index: "
              << searchEngine.documentCount()  << " docs, "
              << searchEngine.uniqueWordCount() << " unique words\n";

    // ── Run Searches ─────────────────────────────────────────────
    std::cout << "\n── Search Results ─────────────────────────────────────\n";
    printResults(searchEngine.search("neural network"),    "neural network");
    printResults(searchEngine.search("memory management"), "memory management");
    printResults(searchEngine.search("ocr image text"),    "ocr image text");
    printResults(searchEngine.search("algorithm complexity"), "algorithm complexity");

    // ── Autocomplete ─────────────────────────────────────────────
    std::cout << "\n── Autocomplete ───────────────────────────────────────\n";
    for (const char* prefix : {"neur", "mem", "alg", "ocr", "pre"}) {
        auto words = searchEngine.prefixSearch(prefix, 5);
        std::cout << "  \"" << prefix << "\" → ";
        for (std::size_t i = 0; i < words.size(); ++i) {
            if (i) std::cout << ", ";
            std::cout << words[i];
        }
        std::cout << "\n";
    }

    // ── Generate PDF Report ──────────────────────────────────────
    utils::Logger::info("\nGenerating Phase 3 PDF report...");

    pdf::PdfDocument doc;
    doc.addFont("F1", "Helvetica");
    doc.addFont("F2", "Helvetica-Bold");
    doc.addFont("F3", "Courier");

    // Page 1: Pipeline overview
    {
        pdf::PdfPage& page = doc.addPage();

        page.setFillColor(0.1f, 0.3f, 0.6f);
        page.fillRect(0, 750, 612, 42);

        page.beginText();
        page.setTextColor(1.0f, 1.0f, 1.0f);
        page.setFont("F2", 20.0f);
        page.moveTo(72.0f, 762.0f);
        page.showText("SmartLibrarian — OCR Pipeline Report");
        page.endText();

        page.setStrokeColor(0.1f, 0.3f, 0.6f);
        page.drawLine(72, 730, 540, 730, 1.0f);

        page.beginText();
        page.setTextColor(0.1f, 0.1f, 0.1f);
        page.setFont("F2", 13.0f);
        page.moveTo(72.0f, 705.0f);
        page.showText("OCR Pipeline Stages:");

        page.setFont("F3", 10.0f);
        page.setLeading(17.0f);
        page.moveTo(72.0f, 685.0f);

        const char* stages[] = {
            "  1. ImageLoader       Load PNG/JPG/TIFF into ImageBuffer (RAII)",
            "  2. toGrayscale       ITU-R BT.601: Y = 0.299R + 0.587G + 0.114B",
            "  3. normalizeContrast Histogram stretch: min->0, max->255",
            "  4. scale             2x nearest-neighbor upscale for small text",
            "  5. binarize          Otsu's adaptive thresholding algorithm",
            "  6. removeNoise       3x3 median filter (preserves text edges)",
            "  7. Tesseract::Init   Load eng.traineddata (LSTM neural network)",
            "  8. SetImage          Pass pixel buffer to Tesseract (no copy)",
            "  9. Recognize         Character segmentation + classification",
            " 10. ResultIterator    Extract words + per-word confidence scores",
            " 11. Filter            Discard words below confidence threshold",
            " 12. SearchEngine      Index filtered text for full-text search",
            nullptr
        };

        for (int i = 0; stages[i]; ++i) {
            page.showText(stages[i]);
            page.nextLine();
        }

        // Search stats box
        page.setFont("F2", 12.0f);
        page.moveTo(72.0f, 440.0f);
        page.showText("Search Index Statistics:");

        page.setFont("F1", 11.0f);
        page.setLeading(18.0f);
        page.moveTo(72.0f, 420.0f);
        page.showText("Documents: " +
                      std::to_string(searchEngine.documentCount()));
        page.nextLine();
        page.showText("Unique words: " +
                      std::to_string(searchEngine.uniqueWordCount()));
        page.nextLine();
        page.showText("Trie nodes: " +
                      std::to_string(searchEngine.trieNodeCount()));
        page.endText();

        page.setStrokeColor(0.7f, 0.7f, 0.7f);
        page.drawLine(72, 60, 540, 60, 0.5f);
        page.beginText();
        page.setTextColor(0.5f, 0.5f, 0.5f);
        page.setFont("F1", 9.0f);
        page.moveTo(72.0f, 45.0f);
        page.showText("SmartLibrarian v0.1.0 — Phase 3: OCR Pipeline");
        page.endText();
    }

    doc.save("SmartLibrarian_Phase3.pdf");
    utils::Logger::info("PDF saved: SmartLibrarian_Phase3.pdf");

    sl::core::Application app(argc, argv);
    return app.run();
}
