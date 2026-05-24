
#include "gui/OcrWorker.h"
#include "ocr/OcrEngine.h"
#include "ocr/ImageLoader.h"

#include <QFileInfo>

namespace sl {
namespace gui {

OcrWorker::OcrWorker(sl::ocr::OcrEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{}

void OcrWorker::process(const QString& imagePath)
{
    // This method runs on the worker thread.
    // We must never touch any QWidget from here.
    // We communicate back to the main thread via signals only.

    const std::string path = imagePath.toStdString();

    try {
        // Stage 1: load
        emit progressUpdated(10, "Loading image...");

        if (!sl::ocr::ImageLoader::isSupported(path)) {
            emit ocrFailed(imagePath,
                "Unsupported image format. "
                "Supported: PNG, JPG, BMP, TIFF");
            return;
        }

        sl::ocr::ImageBuffer raw = sl::ocr::ImageLoader::load(path);

        // Stage 2: preprocess
        emit progressUpdated(30, "Preprocessing image...");

        // Stage 3: OCR
        emit progressUpdated(50, "Running OCR engine...");
        sl::ocr::OcrResult result =
            m_engine->processImageWithPreprocessing(raw);

        // Stage 4: done
        emit progressUpdated(100, "Complete");

        if (!result.success) {
            emit ocrFailed(imagePath,
                QString::fromStdString(result.errorMessage));
            return;
        }

        emit ocrCompleted(
            imagePath,
            QString::fromStdString(result.fullText),
            result.averageConfidence,
            static_cast<int>(result.words.size())
        );
    }
    catch (const std::exception& ex) {
        emit ocrFailed(imagePath, QString::fromStdString(ex.what()));
    }
}

} // namespace gui
} // namespace sl
