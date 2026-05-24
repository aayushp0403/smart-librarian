
#pragma once

#include "ocr/OcrResult.h"

#include <QObject>
#include <QString>
#include <memory>

// Forward declarations — minimize header coupling
namespace sl { namespace ocr { class OcrEngine; } }

namespace sl {
namespace gui {

/**
 * OcrWorker
 *
 * Runs OCR processing on a QThread — never on the main/UI thread.
 *
 * Threading model in Qt:
 *   - The main thread runs the event loop and owns all widgets.
 *   - Widgets MUST only be touched from the main thread.
 *   - Long operations MUST run on worker threads.
 *
 * Pattern used here: Worker Object + moveToThread()
 *
 *   QThread thread;
 *   OcrWorker* worker = new OcrWorker(engine);
 *   worker->moveToThread(&thread);
 *   connect(&thread, &QThread::started, worker, &OcrWorker::process);
 *   thread.start();
 *
 * moveToThread() changes the object's "thread affinity" — Qt now
 * delivers queued signals to this object through the worker thread's
 * event loop instead of the main thread's. This is how cross-thread
 * signal/slot calls become safe: Qt serializes them through the
 * receiving thread's event queue.
 *
 * Q_OBJECT macro:
 *   Required for any class using signals/slots.
 *   Tells MOC to generate the meta-object code for this class.
 *   Must appear in the private section of the class definition.
 *   The class must be in a header file (MOC reads headers, not .cpp).
 */
class OcrWorker : public QObject
{
    Q_OBJECT

public:
    explicit OcrWorker(sl::ocr::OcrEngine* engine, QObject* parent = nullptr);
    ~OcrWorker() override = default;

public slots:
    /**
     * process — called when the worker thread starts.
     * Runs OCR on m_imagePath, emits signals when done.
     *
     * 'slots' is a Qt keyword (macro) that declares these
     * methods as callable via the signal/slot system.
     */
    void process(const QString& imagePath);

signals:
    /**
     * Signals — emitted by this object to notify others.
     *
     * 'signals' is a Qt keyword. Signal bodies are generated
     * by MOC — you declare them but never implement them.
     * Emitting a signal calls all connected slots.
     */
    void progressUpdated(int percent, const QString& stage);
    void ocrCompleted(const QString& imagePath,
                      const QString& extractedText,
                      float          averageConfidence,
                      int            wordCount);
    void ocrFailed(const QString& imagePath, const QString& errorMessage);

private:
    sl::ocr::OcrEngine* m_engine; // borrowed pointer — OcrEngine owned by App
};

} // namespace gui
} // namespace sl
