
#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <vector>
#include <string>

namespace sl { namespace search { class SearchEngine; } }

namespace sl {
namespace gui {

struct ArchivedDocument
{
    int         id            { 0 };
    std::string filename;
    std::string fullPath;
    std::string extractedText;
    float       ocrConfidence { 0.0f };
    int         wordCount     { 0 };
    std::string timestamp;
};

class ArchiveWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ArchiveWidget(sl::search::SearchEngine* engine,
                           QWidget* parent = nullptr);
    ~ArchiveWidget() override = default;

    void addDocument(const ArchivedDocument& doc);
    int  documentCount() const;

    /**
     * getDocuments — read-only access for PersistenceManager to save.
     */
    const std::vector<ArchivedDocument>& getDocuments() const {
        return m_documents;
    }

signals:
    void exportPdfRequested(const std::vector<int>& selectedIds);
    void statusMessage(const QString& message);

private slots:
    void onExportPdfClicked();
    void onClearArchiveClicked();
    void onSelectionChanged();

private:
    void rebuildTable();

    sl::search::SearchEngine*     m_engine    { nullptr };
    std::vector<ArchivedDocument> m_documents;

    QTableWidget* m_table      { nullptr };
    QLabel*       m_statsLabel { nullptr };
    QPushButton*  m_exportBtn  { nullptr };
};

} // namespace gui
} // namespace sl
