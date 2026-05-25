#include "gui/MainWindow.h"
#include "gui/OcrWorker.h"
#include "ocr/OcrEngine.h"
#include "search/SearchEngine.h"
#include <QCloseEvent>
#include "utils/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QFileInfo>
#include <QMessageBox>
#include <QDateTime>
#include <QFont>
#include <QApplication>
#include <QFrame>
#include <QFileDialog>


namespace sl {
namespace gui {

// ─────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_searchEngine(std::make_unique<sl::search::SearchEngine>())
    , m_persistence(std::make_unique<sl::archive::PersistenceManager>())
{
    setWindowTitle("SmartLibrarian — Intelligent Document Archive");
    setMinimumSize(920, 660);
    resize(1120, 740);

    try {
        m_ocrEngine = std::make_unique<sl::ocr::OcrEngine>("", "eng");
    }
    catch (const std::exception& ex) {
        m_ocrInitError = QString::fromStdString(ex.what());
    }

    setupUi();
    loadAll();                          // ← restore saved session
    if (!m_demoDataLoaded) {
        seedDemoDocuments();            // ← only seed if nothing was loaded
    }
    setupOcrThread();
}

// ─────────────────────────────────────────────────────────────────────
// Destructor
// ─────────────────────────────────────────────────────────────────────

MainWindow::~MainWindow()
{
    if (m_ocrThread && m_ocrThread->isRunning()) {
        m_ocrThread->quit();
        if (!m_ocrThread->wait(5000)) {
            m_ocrThread->terminate();
            m_ocrThread->wait();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────
// seedDemoDocuments
// ─────────────────────────────────────────────────────────────────────

void MainWindow::seedDemoDocuments()
{
    struct SeedDoc {
        const char* path;
        const char* title;
        const char* content;
    };

    static const SeedDoc seeds[] = {
        {
            "demo/neural_networks.txt",
            "Introduction to Neural Networks",
            "Neural networks are computing systems inspired by biological neural "
            "networks. A neural network consists of interconnected layers of nodes "
            "called neurons. Deep learning uses many-layered neural networks. "
            "Backpropagation trains neural networks by computing gradients. "
            "Convolutional neural networks excel at image recognition tasks."
        },
        {
            "demo/cpp_memory.txt",
            "C++ Memory Management",
            "Memory management in C++ involves stack and heap allocation. "
            "Stack allocation is automatic and very fast. Heap allocation uses "
            "new and delete operators. Smart pointers manage heap memory safely. "
            "unique_ptr provides exclusive ownership semantics. shared_ptr uses "
            "reference counting. RAII ensures resources are released correctly."
        },
        {
            "demo/algorithms.txt",
            "Algorithm Complexity Analysis",
            "An algorithm is a sequence of instructions for solving a problem. "
            "Complexity is measured using Big-O notation. Binary search runs "
            "in O(log n) time on sorted data structures. Quicksort achieves "
            "average O(n log n) complexity. Dynamic programming decomposes "
            "problems into overlapping subproblems for efficiency."
        },
        {
            "demo/data_structures.txt",
            "Fundamental Data Structures",
            "Arrays provide O(1) random access with contiguous memory layout. "
            "Linked lists allow O(1) insertion but O(n) sequential access. "
            "Hash tables achieve O(1) average lookup using hash functions. "
            "Binary search trees maintain sorted order with O(log n) operations. "
            "A trie is a prefix tree optimized for string search operations."
        },
        {
            "demo/pdf_internals.txt",
            "PDF File Format Internals",
            "PDF is a binary file format for portable document exchange. "
            "A PDF contains objects: booleans, integers, strings, arrays, "
            "dictionaries, and streams. The cross-reference table maps object "
            "numbers to exact byte offsets enabling random access. Content "
            "streams use PostScript-like drawing operators for text rendering."
        },
        {
            "demo/ocr_technology.txt",
            "Optical Character Recognition",
            "Optical character recognition extracts text from scanned images. "
            "Tesseract uses a neural network to recognize characters in images. "
            "Image preprocessing dramatically improves OCR accuracy. Binarization "
            "converts grayscale images to black and white. Otsu thresholding "
            "finds the optimal binarization threshold automatically."
        }
    };

    for (const auto& seed : seeds) {
        m_searchEngine->indexDocument(seed.path, seed.title, seed.content);
    }

    if (m_searchWidget) {
        m_searchWidget->refreshResults();
    }
}

// ─────────────────────────────────────────────────────────────────────
// setupUi
// ─────────────────────────────────────────────────────────────────────

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* outerLayout = new QVBoxLayout(central);
    outerLayout->setSpacing(0);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    // ── Header ────────────────────────────────────────────────────
    auto* header = new QFrame(this);
    header->setFixedHeight(52);
    header->setStyleSheet(
        "QFrame {"
        "  background: qlineargradient("
        "    x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #0D2444, stop:1 #1A4A8A);"
        "  border-bottom: 2px solid #2088FF;"
        "}"
    );
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(16, 0, 16, 0);

    auto* appTitle = new QLabel("  SmartLibrarian", header);
    QFont tf = appTitle->font();
    tf.setPointSize(14);
    tf.setBold(true);
    appTitle->setFont(tf);
    appTitle->setStyleSheet("color: white; background: transparent;");
    headerLayout->addWidget(appTitle);
    headerLayout->addStretch();

    m_ocrStatusLabel = new QLabel(header);
    m_ocrStatusLabel->setStyleSheet(
        "background: transparent; font-size: 10px;");
    if (m_ocrEngine && m_ocrEngine->isInitialized()) {
        m_ocrStatusLabel->setText(
            "OCR Ready  |  Tesseract " +
            QString::fromStdString(m_ocrEngine->getVersion()));
        m_ocrStatusLabel->setStyleSheet(
            "color: #66FF88; font-size: 10px; background: transparent;");
    } else {
        m_ocrStatusLabel->setText("OCR Unavailable");
        m_ocrStatusLabel->setStyleSheet(
            "color: #FFAA44; font-size: 10px; background: transparent;");
    }
    headerLayout->addWidget(m_ocrStatusLabel);
    outerLayout->addWidget(header);

    // ── Splitter ──────────────────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal, central);
    splitter->setHandleWidth(3);
    splitter->setStyleSheet(
        "QSplitter::handle { background: #E0E0E0; }"
        "QSplitter::handle:hover { background: #2088FF; }");

    // ── Left pane ─────────────────────────────────────────────────
    auto* leftPane = new QWidget(splitter);
    leftPane->setMinimumWidth(260);
    leftPane->setMaximumWidth(380);

    auto* leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(12, 12, 8, 12);
    leftLayout->setSpacing(8);

    // Section label
    auto* sectionLabel = new QLabel("Process Document", leftPane);
    QFont sf = sectionLabel->font();
    sf.setPointSize(11);
    sf.setBold(true);
    sectionLabel->setFont(sf);
    sectionLabel->setStyleSheet("color: #333;");
    leftLayout->addWidget(sectionLabel);

    // ── PRIMARY: Browse button (large, always visible) ────────────
    m_mainBrowseBtn = new QPushButton(
        "   Select Image File...", leftPane);
    m_mainBrowseBtn->setFixedHeight(44);
    m_mainBrowseBtn->setCursor(Qt::PointingHandCursor);
    m_mainBrowseBtn->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient("
        "    x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #2A9AFF, stop:1 #1070DD);"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  text-align: left;"
        "  padding-left: 14px;"
        "}"
        "QPushButton:hover  {"
        "  background: qlineargradient("
        "    x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #44AAFF, stop:1 #2088EE);"
        "}"
        "QPushButton:pressed {"
        "  background: #0055BB;"
        "}"
        "QPushButton:disabled {"
        "  background: #AAAAAA;"
        "}"
    );
    leftLayout->addWidget(m_mainBrowseBtn);

    // Supported formats hint
    auto* formatHint = new QLabel(
        "Supports: PNG  JPG  TIFF  BMP", leftPane);
    formatHint->setAlignment(Qt::AlignCenter);
    formatHint->setStyleSheet(
        "color: #999; font-size: 9px;");
    leftLayout->addWidget(formatHint);

    // ── SECONDARY: Drop zone (still available) ────────────────────
    m_dropZone = new DropZone(leftPane);
    leftLayout->addWidget(m_dropZone);

    // ── Progress bar ──────────────────────────────────────────────
    m_progressBar = new QProgressBar(leftPane);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    m_progressBar->setFixedHeight(16);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  border: 1px solid #CCC;"
        "  border-radius: 4px;"
        "}"
        "QProgressBar::chunk {"
        "  background: #2088FF;"
        "  border-radius: 4px;"
        "}"
    );
    leftLayout->addWidget(m_progressBar);

    // ── Status label ──────────────────────────────────────────────
    m_statusLabel = new QLabel(
        "Click the button above\nor drop an image here\nto run OCR.",
        leftPane);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setMinimumHeight(52);
    m_statusLabel->setStyleSheet(
        "color: #666;"
        "font-size: 10px;"
        "padding: 6px;"
        "background: #F5F5F5;"
        "border: 1px solid #E8E8E8;"
        "border-radius: 4px;"
    );
    leftLayout->addWidget(m_statusLabel);

    // Separator
    auto* sep = new QFrame(leftPane);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #E8E8E8;");
    leftLayout->addWidget(sep);

    // Stats
    m_quickStatsLabel = new QLabel(
        "Demo data loaded: 6 documents\nTry searching in the Search tab.",
        leftPane);
    m_quickStatsLabel->setWordWrap(true);
    m_quickStatsLabel->setStyleSheet(
        "color: #2088FF; font-size: 10px; padding: 2px;");
    leftLayout->addWidget(m_quickStatsLabel);

    leftLayout->addStretch();
    splitter->addWidget(leftPane);

    // ── Right pane: tabs ──────────────────────────────────────────
    auto* rightPane = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(8, 12, 12, 12);
    rightLayout->setSpacing(0);

    auto* tabs = new QTabWidget(rightPane);
    tabs->setDocumentMode(true);
    tabs->setStyleSheet(
        "QTabWidget::pane {"
        "  border: 1px solid #DDD;"
        "  background: white;"
        "}"
        "QTabBar::tab {"
        "  padding: 8px 24px;"
        "  font-size: 11px;"
        "  border: 1px solid #DDD;"
        "  border-bottom: none;"
        "  border-radius: 4px 4px 0 0;"
        "  margin-right: 2px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: white;"
        "  font-weight: bold;"
        "  color: #2088FF;"
        "}"
        "QTabBar::tab:!selected {"
        "  background: #F0F0F0; color: #666;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  background: #E8F4FF;"
        "}"
    );

    m_searchWidget  = new SearchWidget(m_searchEngine.get(), tabs);
    m_archiveWidget = new ArchiveWidget(m_searchEngine.get(), tabs);
    tabs->addTab(m_searchWidget,  "Search");
    tabs->addTab(m_archiveWidget, "Archive");

    rightLayout->addWidget(tabs);
    splitter->addWidget(rightPane);
    splitter->setSizes({ 300, 800 });
    outerLayout->addWidget(splitter, 1);

    // ── Status bar ────────────────────────────────────────────────
    statusBar()->showMessage(
        "Ready  |  6 demo documents loaded  |  "
        "Click 'Select Image File' or drop an image to run OCR");
    statusBar()->setStyleSheet(
        "QStatusBar { font-size: 10px; color: #555; "
        "border-top: 1px solid #DDD; }");

    // ── Wire signals ──────────────────────────────────────────────

    // Browse button in left pane → open file dialog directly
    connect(m_mainBrowseBtn, &QPushButton::clicked,
            m_dropZone, &DropZone::openFileBrowser);

    // DropZone (both button and drop) → onImageDropped
    connect(m_dropZone, &DropZone::imageDropped,
            this, &MainWindow::onImageDropped);

    // Archive status messages → status bar
    connect(m_archiveWidget, &ArchiveWidget::statusMessage,
            [this](const QString& msg) {
                statusBar()->showMessage(msg);
            });
}

// ─────────────────────────────────────────────────────────────────────
// setupOcrThread
// ─────────────────────────────────────────────────────────────────────

void MainWindow::setupOcrThread()
{
    m_ocrThread = new QThread(this);
    m_ocrWorker = new OcrWorker(m_ocrEngine.get());
    m_ocrWorker->moveToThread(m_ocrThread);

    connect(this,        &MainWindow::startOcrProcessing,
            m_ocrWorker, &OcrWorker::process,
            Qt::QueuedConnection);

    connect(m_ocrWorker, &OcrWorker::progressUpdated,
            this,        &MainWindow::onOcrProgress);
    connect(m_ocrWorker, &OcrWorker::ocrCompleted,
            this,        &MainWindow::onOcrCompleted);
    connect(m_ocrWorker, &OcrWorker::ocrFailed,
            this,        &MainWindow::onOcrFailed);

    connect(m_ocrThread, &QThread::finished,
            m_ocrWorker, &QObject::deleteLater);

    m_ocrThread->start();
}

// ─────────────────────────────────────────────────────────────────────
// onImageDropped
// ─────────────────────────────────────────────────────────────────────

void MainWindow::onImageDropped(const QString& filePath)
{
    if (!m_ocrEngine || !m_ocrEngine->isInitialized()) {
        QMessageBox::warning(this, "OCR Not Available",
            "Tesseract OCR is not initialized.\n\n"
            "Install with:\n"
            "  sudo apt install tesseract-ocr tesseract-ocr-eng\n\n"
            "Then restart SmartLibrarian.");
        return;
    }

    setProcessingState(true);

    QFileInfo fi(filePath);
    m_statusLabel->setText(
        "Processing:\n" + fi.fileName());
    statusBar()->showMessage("OCR running on: " + fi.fileName());

    emit startOcrProcessing(filePath);
}

// ─────────────────────────────────────────────────────────────────────
// onOcrProgress
// ─────────────────────────────────────────────────────────────────────

void MainWindow::onOcrProgress(int percent, const QString& stage)
{
    m_progressBar->setValue(percent);
    m_statusLabel->setText(stage);
    statusBar()->showMessage(stage);
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

// ─────────────────────────────────────────────────────────────────────
// onOcrCompleted
// ─────────────────────────────────────────────────────────────────────

void MainWindow::onOcrCompleted(
    const QString& imagePath,
    const QString& extractedText,
    float          confidence,
    int            wordCount)
{
    setProcessingState(false);

    QFileInfo fi(imagePath);
    const std::string filename = fi.fileName().toStdString();

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

    m_quickStatsLabel->setText(
        QString("Index: %1 docs  |  %2 unique words")
            .arg(m_searchEngine->documentCount())
            .arg(m_searchEngine->uniqueWordCount()));

    QString summary = QString(
        "Done: %1\n%2 words extracted\nConfidence: %3%")
        .arg(fi.fileName())
        .arg(wordCount)
        .arg(static_cast<double>(confidence), 0, 'f', 1);
    m_statusLabel->setText(summary);

    statusBar()->showMessage(
        QString("Indexed: %1  |  %2 words  |  Confidence: %3%")
            .arg(fi.fileName())
            .arg(wordCount)
            .arg(static_cast<double>(confidence), 0, 'f', 1));
}

// ─────────────────────────────────────────────────────────────────────
// onOcrFailed
// ─────────────────────────────────────────────────────────────────────

void MainWindow::onOcrFailed(
    const QString& imagePath,
    const QString& errorMessage)
{
    setProcessingState(false);

    QFileInfo fi(imagePath);
    m_statusLabel->setText("Failed: " + fi.fileName() +
                           "\n\n" + errorMessage);
    m_statusLabel->setStyleSheet(
        "color: #AA0000; font-size: 10px; padding: 6px;"
        "background: #FFF0F0; border-radius: 4px;"
        "border: 1px solid #FFCCCC;");

    statusBar()->showMessage("OCR failed: " + fi.fileName());
    QMessageBox::warning(this, "OCR Failed",
        "Could not process: " + fi.fileName() +
        "\n\n" + errorMessage);
}

// ─────────────────────────────────────────────────────────────────────
// setProcessingState
// ─────────────────────────────────────────────────────────────────────

void MainWindow::setProcessingState(bool processing)
{
    m_dropZone->setEnabled(!processing);
    m_mainBrowseBtn->setEnabled(!processing);
    m_progressBar->setVisible(processing);

    if (processing) {
        m_progressBar->setValue(0);
        m_mainBrowseBtn->setText("   Processing...");
        m_statusLabel->setStyleSheet(
            "color: #0055AA; font-size: 10px; padding: 6px;"
            "background: #EEF6FF; border-radius: 4px;"
            "border: 1px solid #CCE4FF;");
    } else {
        m_mainBrowseBtn->setText("   Select Image File...");
        m_statusLabel->setStyleSheet(
            "color: #555; font-size: 10px; padding: 6px;"
            "background: #F5F5F5; border-radius: 4px;"
            "border: 1px solid #E8E8E8;");
    }
}



// ─────────────────────────────────────────────────────────────────────
// closeEvent — Qt calls this when the user closes the window.
// We save everything before allowing the close to proceed.
// Calling event->accept() allows the close; ignore() would cancel it.
// ─────────────────────────────────────────────────────────────────────

void MainWindow::closeEvent(QCloseEvent* event)
{
    statusBar()->showMessage("Saving data...");
    QApplication::processEvents();

    saveAll();

    event->accept();  // allow the window to close
}

// ─────────────────────────────────────────────────────────────────────
// saveAll
// ─────────────────────────────────────────────────────────────────────

void MainWindow::saveAll()
{
    if (!m_persistence) return;

    // Update config with current state
    m_config.maxSearchResults = 20;

    m_persistence->saveConfig(m_config);
    m_persistence->saveIndex(*m_searchEngine);
    m_persistence->saveArchive(m_archiveWidget->getDocuments());

    sl::utils::Logger::info("All data saved to " +
                            m_persistence->dataDirectory());
}

// ─────────────────────────────────────────────────────────────────────
// loadAll — called during construction, before window is shown
// ─────────────────────────────────────────────────────────────────────

void MainWindow::loadAll()
{
    if (!m_persistence) return;

    // Load config
    m_persistence->loadConfig(m_config);

    // Load search index
    bool indexLoaded = m_persistence->loadIndex(*m_searchEngine);

    // Load archive documents
    std::vector<sl::gui::ArchivedDocument> savedDocs;
    bool archiveLoaded = m_persistence->loadArchive(savedDocs);

    if (archiveLoaded && !savedDocs.empty()) {
        // Restore nextDocId to be beyond all loaded document IDs
        for (const auto& doc : savedDocs) {
            if (doc.id >= m_nextDocId) {
                m_nextDocId = doc.id + 1;
            }
            m_archiveWidget->addDocument(doc);
        }
    }

    if (indexLoaded || archiveLoaded) {
        sl::utils::Logger::info(
            "Session restored: " +
            std::to_string(m_searchEngine->documentCount()) +
            " docs in index, " +
            std::to_string(savedDocs.size()) +
            " docs in archive");
        m_demoDataLoaded = true;  // skip seeding demo data
    }

    if (m_searchWidget) {
        m_searchWidget->refreshResults();
    }

    if (m_quickStatsLabel) {
        if (indexLoaded) {
            m_quickStatsLabel->setText(
                QString("Restored: %1 docs  |  %2 unique words")
                    .arg(m_searchEngine->documentCount())
                    .arg(m_searchEngine->uniqueWordCount()));
        } else {
            m_quickStatsLabel->setText(
                "No saved data found.\nDemo data loaded.");
        }
    }
}
} // namespace gui
} // namespace sl
