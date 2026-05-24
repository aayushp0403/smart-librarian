#include "gui/MainWindow.h"
#include "gui/OcrWorker.h"
#include "ocr/OcrEngine.h"
#include "search/SearchEngine.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QFileInfo>
#include <QMessageBox>
#include <QDateTime>
#include <QFont>
#include <QApplication>

namespace sl {
namespace gui {

// ─────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_searchEngine(std::make_unique<sl::search::SearchEngine>())
{
    setWindowTitle("SmartLibrarian — Intelligent Document Archive");
    setMinimumSize(900, 650);
    resize(1100, 720);

    // Initialize OCR engine (expensive — do in constructor, not on demand)
    try {
        m_ocrEngine = std::make_unique<sl::ocr::OcrEngine>("", "eng");
    }
    catch (const std::exception& ex) {
        // OCR unavailable — app still works for search/PDF
        // We'll show a notice in the UI
        Q_UNUSED(ex);
    }

    setupUi();
    setupOcrThread();
}

// ─────────────────────────────────────────────────────────────────────
// Destructor
//
// Must stop the OCR thread cleanly before destroying the worker.
//
// Qt thread shutdown sequence:
//   1. thread.quit()  — sends a quit event to the thread's event loop
//   2. thread.wait()  — blocks until the thread has finished
//
// If we destroy OcrWorker while the thread is still running,
// we have a data race — the worker might be mid-OCR when we delete it.
// Quit + wait ensures the thread is fully stopped before any objects
// are destroyed. This is the correct Qt thread cleanup pattern.
// ─────────────────────────────────────────────────────────────────────

MainWindow::~MainWindow()
{
    if (m_ocrThread) {
        m_ocrThread->quit();
        m_ocrThread->wait(3000);  // 3 second timeout
    }
    // m_ocrWorker is owned by the thread via Qt's object tree
    // (parent = nullptr but deleteLater is called by thread)
    // m_ocrEngine and m_searchEngine are unique_ptrs — auto-deleted
}

// ─────────────────────────────────────────────────────────────────────
// setupUi
// ─────────────────────────────────────────────────────────────────────

void MainWindow::setupUi()
{
    // ── Central widget and outer layout ──────────────────────────
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* outerLayout = new QVBoxLayout(central);
    outerLayout->setSpacing(0);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    // ── Toolbar / Header ─────────────────────────────────────────
    auto* header = new QWidget(this);
    header->setFixedHeight(54);
    header->setStyleSheet(
        "background: qlineargradient("
        "  x1:0, y1:0, x2:1, y2:0,"
        "  stop:0 #0F2E5A, stop:1 #1A4A8A);"
    );
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(16, 0, 16, 0);

    auto* appTitle = new QLabel("📚  SmartLibrarian", header);
    QFont titleFont = appTitle->font();
    titleFont.setPointSize(15);
    titleFont.setBold(true);
    appTitle->setFont(titleFont);
    appTitle->setStyleSheet("color: white;");
    headerLayout->addWidget(appTitle);

    headerLayout->addStretch();

    m_ocrStatusLabel = new QLabel("OCR: Initializing...", header);
    m_ocrStatusLabel->setStyleSheet("color: #AAD4FF; font-size: 11px;");
    headerLayout->addWidget(m_ocrStatusLabel);

    if (m_ocrEngine && m_ocrEngine->isInitialized()) {
        m_ocrStatusLabel->setText(
            "OCR: Ready  (Tesseract " +
            QString::fromStdString(m_ocrEngine->getVersion()) + ")");
        m_ocrStatusLabel->setStyleSheet("color: #88FF88; font-size: 11px;");
    } else {
        m_ocrStatusLabel->setText("OCR: Unavailable");
        m_ocrStatusLabel->setStyleSheet("color: #FFAA44; font-size: 11px;");
    }

    outerLayout->addWidget(header);

    // ── Content area: splitter ────────────────────────────────────
    // QSplitter lets the user drag to resize the two panes.
    auto* splitter = new QSplitter(Qt::Horizontal, central);
    splitter->setHandleWidth(2);
    splitter->setStyleSheet("QSplitter::handle { background: #DDD; }");

    // ── Left pane: DropZone ───────────────────────────────────────
    auto* leftPane = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(12, 12, 6, 12);
    leftLayout->setSpacing(10);

    auto* dropTitle = new QLabel("Process Image", leftPane);
    QFont dropTitleFont = dropTitle->font();
    dropTitleFont.setPointSize(12);
    dropTitleFont.setBold(true);
    dropTitle->setFont(dropTitleFont);
    leftLayout->addWidget(dropTitle);

    m_dropZone = new DropZone(leftPane);
    leftLayout->addWidget(m_dropZone);

    // Processing indicator
    m_progressBar = new QProgressBar(leftPane);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->hide();
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  border: 1px solid #DDD;"
        "  border-radius: 4px;"
        "  text-align: center;"
        "  height: 20px;"
        "}"
        "QProgressBar::chunk {"
        "  background: #2088FF;"
        "  border-radius: 4px;"
        "}"
    );
    leftLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel("Drop an image to begin.", leftPane);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(
        "color: #555; font-size: 10px; padding: 4px;");
    leftLayout->addWidget(m_statusLabel);

    leftLayout->addStretch();
    splitter->addWidget(leftPane);

    // ── Right pane: tabs ──────────────────────────────────────────
    auto* tabs = new QTabWidget(splitter);
    tabs->setStyleSheet(
        "QTabWidget::pane {"
        "  border: 1px solid #DDD;"
        "  border-radius: 4px;"
        "}"
        "QTabBar::tab {"
        "  padding: 8px 20px;"
        "  border: 1px solid #DDD;"
        "  border-bottom: none;"
        "  border-top-left-radius: 4px;"
        "  border-top-right-radius: 4px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: white;"
        "  font-weight: bold;"
        "}"
        "QTabBar::tab:!selected {"
        "  background: #F5F5F5;"
        "}"
    );

    m_searchWidget = new SearchWidget(m_searchEngine.get(), tabs);
    tabs->addTab(m_searchWidget, "🔍  Search");

    m_archiveWidget = new ArchiveWidget(m_searchEngine.get(), tabs);
    tabs->addTab(m_archiveWidget, "📁  Archive");

    splitter->addWidget(tabs);

    // Initial splitter ratio: 35% left, 65% right
    splitter->setSizes({350, 650});
    outerLayout->addWidget(splitter, 1);

    // ── Status bar ────────────────────────────────────────────────
    statusBar()->showMessage("Ready — drop an image to begin OCR processing");
    statusBar()->setStyleSheet("font-size: 10px; color: #555;");

    // ── Connections ───────────────────────────────────────────────
    connect(m_dropZone, &DropZone::imageDropped,
            this, &MainWindow::onImageDropped);

    connect(m_archiveWidget, &ArchiveWidget::statusMessage,
            [this](const QString& msg) { statusBar()->showMessage(msg); });
}

// ─────────────────────────────────────────────────────────────────────
// setupOcrThread
//
// Worker-object threading pattern:
//
//   1. Create QThread
//   2. Create OcrWorker (no parent — we'll move it to the thread)
//   3. moveToThread() — changes worker's thread affinity
//   4. Connect signals/slots across thread boundary
//   5. thread.start() — starts the thread's event loop
//
// Important: OcrWorker is NOT moved to the thread in its constructor.
// It's moved AFTER construction. This is the correct pattern because
// constructors run on the creating thread.
//
// Cross-thread signals are automatically queued by Qt:
//   - main thread emits → worker thread receives (queued)
//   - worker thread emits → main thread receives (queued, safe for widgets)
// ─────────────────────────────────────────────────────────────────────

void MainWindow::setupOcrThread()
{
    m_ocrThread = new QThread(this);

    // OcrWorker takes the engine as a borrowed pointer.
    // The engine is owned by MainWindow (m_ocrEngine unique_ptr).
    // OcrWorker's parent is nullptr — we give thread ownership
    // via moveToThread, not Qt's object parent system.
    m_ocrWorker = new OcrWorker(m_ocrEngine.get());
    m_ocrWorker->moveToThread(m_ocrThread);

    // When the worker's process() slot is triggered by a signal
    // from the main thread, Qt queues the call to run on m_ocrThread.
    connect(this, &MainWindow::startOcrProcessing,
            m_ocrWorker, &OcrWorker::process,
            Qt::QueuedConnection);

    // Results come back to main thread automatically (Qt queues them)
    connect(m_ocrWorker, &OcrWorker::progressUpdated,
            this, &MainWindow::onOcrProgress);
    connect(m_ocrWorker, &OcrWorker::ocrCompleted,
            this, &MainWindow::onOcrCompleted);
    connect(m_ocrWorker, &OcrWorker::ocrFailed,
            this, &MainWindow::onOcrFailed);

    // Clean up worker when thread finishes
    connect(m_ocrThread, &QThread::finished,
            m_ocrWorker, &QObject::deleteLater);

    m_ocrThread->start();
}

// ─────────────────────────────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────────────────────────────

void MainWindow::onImageDropped(const QString& filePath)
{
    if (!m_ocrEngine || !m_ocrEngine->isInitialized()) {
        QMessageBox::warning(this, "OCR Unavailable",
            "Tesseract OCR is not initialized.\n"
            "Install: sudo apt install tesseract-ocr tesseract-ocr-eng\n"
            "Then restart the application.");
        return;
    }

    setProcessingState(true);
    updateStatusBar("Processing: " + filePath);

    // Emit signal to trigger OcrWorker::process() on the OCR thread
    // This is a cross-thread signal — Qt automatically queues it
    emit startOcrProcessing(filePath);
}

void MainWindow::onOcrProgress(int percent, const QString& stage)
{
    // This runs on the main thread (Qt queued the signal)
    // Safe to update widgets here
    m_progressBar->setValue(percent);
    m_statusLabel->setText(stage);
    statusBar()->showMessage(stage);
}

void MainWindow::onOcrCompleted(
    const QString& imagePath,
    const QString& extractedText,
    float          confidence,
    int            wordCount)
{
    setProcessingState(false);

    QFileInfo fi(imagePath);
    std::string filename = fi.fileName().toStdString();

    // Index in search engine
    m_searchEngine->indexDocument(
        imagePath.toStdString(),
        filename,
        extractedText.toStdString()
    );

    // Add to archive
    ArchivedDocument adoc;
    adoc.id            = m_nextDocId++;
    adoc.filename      = filename;
    adoc.fullPath      = imagePath.toStdString();
    adoc.extractedText = extractedText.toStdString();
    adoc.ocrConfidence = confidence;
    adoc.wordCount     = wordCount;
    adoc.timestamp     = QDateTime::currentDateTime()
                             .toString("yyyy-MM-dd hh:mm")
                             .toStdString();

    m_archiveWidget->addDocument(adoc);
    m_searchWidget->refreshResults();

    QString msg = QString("✓ Indexed: %1  |  %2 words  |  "
                          "Confidence: %3%")
        .arg(fi.fileName())
        .arg(wordCount)
        .arg(static_cast<double>(confidence), 0, 'f', 1);

    updateStatusBar(msg);
    m_statusLabel->setText(msg);
    statusBar()->showMessage(msg);
}

void MainWindow::onOcrFailed(
    const QString& imagePath,
    const QString& errorMessage)
{
    setProcessingState(false);

    QString msg = "✗ OCR failed: " + errorMessage;
    updateStatusBar(msg);
    m_statusLabel->setText(msg);
    m_statusLabel->setStyleSheet(
        "color: #DD2222; font-size: 10px; padding: 4px;");

    QMessageBox::warning(this, "OCR Failed",
        "Failed to process: " + imagePath + "\n\n" + errorMessage);
}

// ─────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────

void MainWindow::setProcessingState(bool processing)
{
    m_dropZone->setEnabled(!processing);
    m_progressBar->setVisible(processing);
    if (!processing) {
        m_progressBar->setValue(0);
    }
}

void MainWindow::updateStatusBar(const QString& message)
{
    statusBar()->showMessage(message);
}

} // namespace gui
} // namespace sl
