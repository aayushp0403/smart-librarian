#pragma once

#include "gui/DropZone.h"
#include "gui/SearchWidget.h"
#include "gui/ArchiveWidget.h"
#include "archive/PersistenceManager.h"
#include "archive/AppConfig.h"

#include <QMainWindow>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QThread>
#include <memory>

namespace sl {
namespace ocr    { class OcrEngine;    }
namespace search { class SearchEngine; }
namespace gui    { class OcrWorker;    }
}

namespace sl {
namespace gui {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

signals:
    void startOcrProcessing(const QString& imagePath);

private slots:
    void onImageDropped(const QString& filePath);
    void onOcrProgress(int percent, const QString& stage);
    void onOcrCompleted(const QString& imagePath,
                        const QString& extractedText,
                        float          confidence,
                        int            wordCount);
    void onOcrFailed(const QString& imagePath,
                     const QString& errorMessage);

protected:
    // Save on close
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUi();
    void setupOcrThread();
    void seedDemoDocuments();
    void setProcessingState(bool processing);
    void saveAll();
    void loadAll();

    // ── Backend ───────────────────────────────────────────────────
    std::unique_ptr<sl::ocr::OcrEngine>              m_ocrEngine;
    std::unique_ptr<sl::search::SearchEngine>        m_searchEngine;
    std::unique_ptr<sl::archive::PersistenceManager> m_persistence;
    sl::archive::AppConfig                           m_config;
    QString                                          m_ocrInitError;

    // ── Threading ─────────────────────────────────────────────────
    QThread*   m_ocrThread { nullptr };
    OcrWorker* m_ocrWorker { nullptr };

    // ── Widgets ───────────────────────────────────────────────────
    DropZone*      m_dropZone        { nullptr };
    SearchWidget*  m_searchWidget    { nullptr };
    ArchiveWidget* m_archiveWidget   { nullptr };
    QProgressBar*  m_progressBar     { nullptr };
    QLabel*        m_statusLabel     { nullptr };
    QLabel*        m_ocrStatusLabel  { nullptr };
    QLabel*        m_quickStatsLabel { nullptr };
    QPushButton*   m_mainBrowseBtn   { nullptr };

    int  m_nextDocId      { 1 };
    bool m_demoDataLoaded { false };
};

} // namespace gui
} // namespace sl
