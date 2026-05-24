#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <vector>
#include <string>

namespace sl { namespace search { class SearchEngine; } }
namespace sl { namespace pdf    { class PdfDocument;  } }

namespace sl {
namespace gui {

/**
 * ArchivedDocument
 *
 * Lightweight record of a document that has been processed.
 * Separate from SearchEngine's internal DocumentInfo to keep
 * the UI layer decoupled from the search layer.
 */
struct ArchivedDocument
{
    int         id;
    std::string filename;
    std::string fullPath;
    std::string extractedText;
    float       ocrConfidence;
    int         wordCount;
    std::string timestamp;
};

/**
 * ArchiveWidget
 *
 * Shows all processed documents in a table.
 * Provides PDF export for selected documents.
 */
class ArchiveWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ArchiveWidget(sl::search::SearchEngine* engine,
                           QWidget* parent = nullptr);
    ~ArchiveWidget() override = default;

    /**
     * addDocument — add a newly processed document to the archive.
     */
    void addDocument(const ArchivedDocument& doc);

    int documentCount() const;

signals:
    void exportPdfRequested(const std::vector<int>& selectedIds);
    void statusMessage(const QString& message);

private slots:
    void onExportPdfClicked();
    void onClearArchiveClicked();
    void onSelectionChanged();

private:
    void rebuildTable();

    sl::search::SearchEngine*       m_engine;
    std::vector<ArchivedDocument>   m_documents;

    QTableWidget* m_table       { nullptr };
    QLabel*       m_statsLabel  { nullptr };
    QPushButton*  m_exportBtn   { nullptr };
};

} // namespace gui
} // namespace sl
