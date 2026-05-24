#include "gui/ArchiveWidget.h"
#include "search/SearchEngine.h"
#include "pdf/PdfDocument.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QTableWidgetItem>
#include <QFont>

namespace sl {
namespace gui {

ArchiveWidget::ArchiveWidget(sl::search::SearchEngine* engine,
                             QWidget* parent)
    : QWidget(parent)
    , m_engine(engine)
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 12, 12, 12);

    // Title
    auto* title = new QLabel("📁  Document Archive", this);
    QFont f = title->font();
    f.setPointSize(13);
    f.setBold(true);
    title->setFont(f);
    layout->addWidget(title);

    // Stats
    m_statsLabel = new QLabel("No documents archived.", this);
    m_statsLabel->setStyleSheet("color: #666; font-size: 10px;");
    layout->addWidget(m_statsLabel);

    // Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        "ID", "Filename", "Words", "OCR Confidence", "Processed"
    });

    // Column sizing
    m_table->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);  // ID
    m_table->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);           // Filename — takes remaining space
    m_table->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);  // Words
    m_table->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::ResizeToContents);  // Confidence
    m_table->horizontalHeader()->setSectionResizeMode(
        4, QHeaderView::ResizeToContents);  // Timestamp

    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setStyleSheet(
        "QTableWidget { border: 1px solid #DDD; border-radius: 6px; }"
        "QHeaderView::section {"
        "  background-color: #F0F0F0;"
        "  border: none;"
        "  padding: 6px;"
        "  font-weight: bold;"
        "}"
    );
    layout->addWidget(m_table, 1);

    // Buttons
    auto* btnRow = new QHBoxLayout();

    m_exportBtn = new QPushButton("Export Selected to PDF", this);
    m_exportBtn->setEnabled(false);
    m_exportBtn->setStyleSheet(
        "QPushButton {"
        "  background: #2088FF; color: white;"
        "  border-radius: 6px; padding: 6px 16px; font-weight: bold;"
        "}"
        "QPushButton:disabled { background: #AAA; }"
        "QPushButton:hover:!disabled { background: #1070EE; }"
    );
    btnRow->addWidget(m_exportBtn);

    auto* clearBtn = new QPushButton("Clear Archive", this);
    clearBtn->setStyleSheet(
        "QPushButton {"
        "  background: #FF4444; color: white;"
        "  border-radius: 6px; padding: 6px 16px; font-weight: bold;"
        "}"
        "QPushButton:hover { background: #DD2222; }"
    );
    btnRow->addWidget(clearBtn);
    btnRow->addStretch();

    layout->addLayout(btnRow);

    // Connections
    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &ArchiveWidget::onSelectionChanged);
    connect(m_exportBtn, &QPushButton::clicked,
            this, &ArchiveWidget::onExportPdfClicked);
    connect(clearBtn, &QPushButton::clicked,
            this, &ArchiveWidget::onClearArchiveClicked);
}

// ─────────────────────────────────────────────────────────────────────
// addDocument
// ─────────────────────────────────────────────────────────────────────

void ArchiveWidget::addDocument(const ArchivedDocument& doc)
{
    m_documents.push_back(doc);
    rebuildTable();
}

int ArchiveWidget::documentCount() const
{
    return static_cast<int>(m_documents.size());
}

// ─────────────────────────────────────────────────────────────────────
// rebuildTable
//
// We rebuild the entire table on any change rather than
// doing incremental updates. For archive sizes we expect
// (tens to hundreds of documents), a full rebuild is fast
// and simpler than managing row insertion/deletion.
// ─────────────────────────────────────────────────────────────────────

void ArchiveWidget::rebuildTable()
{
    m_table->setRowCount(static_cast<int>(m_documents.size()));

    for (int row = 0; row < static_cast<int>(m_documents.size()); ++row)
    {
        const ArchivedDocument& doc = m_documents[
            static_cast<std::size_t>(row)];

        auto makeItem = [](const QString& text) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            return item;
        };

        m_table->setItem(row, 0, makeItem(QString::number(doc.id)));
        m_table->setItem(row, 1,
            new QTableWidgetItem(
                QString::fromStdString(doc.filename)));
        m_table->setItem(row, 2,
            makeItem(QString::number(doc.wordCount)));

        // Color-code confidence: green=good, orange=medium, red=poor
        auto* confItem = makeItem(
            QString::number(static_cast<double>(doc.ocrConfidence),
                           'f', 1) + "%");
        if (doc.ocrConfidence >= 80.0f)
            confItem->setForeground(QColor(0x22, 0xAA, 0x22));
        else if (doc.ocrConfidence >= 60.0f)
            confItem->setForeground(QColor(0xFF, 0x88, 0x00));
        else
            confItem->setForeground(QColor(0xDD, 0x22, 0x22));
        m_table->setItem(row, 3, confItem);

        m_table->setItem(row, 4,
            makeItem(QString::fromStdString(doc.timestamp)));
    }

    m_statsLabel->setText(
        QString("Archive: %1 document(s)  |  "
                "Search index: %2 docs  |  %3 unique words")
        .arg(m_documents.size())
        .arg(m_engine ? m_engine->documentCount() : 0)
        .arg(m_engine ? m_engine->uniqueWordCount() : 0)
    );
}

// ─────────────────────────────────────────────────────────────────────
// onExportPdfClicked — export selected documents to PDF
// ─────────────────────────────────────────────────────────────────────

void ArchiveWidget::onExportPdfClicked()
{
    QList<QTableWidgetItem*> selected = m_table->selectedItems();
    if (selected.isEmpty()) return;

    // Collect unique selected rows
    QSet<int> selectedRows;
    for (auto* item : selected) {
        selectedRows.insert(item->row());
    }

    // Ask user for save path
    QString savePath = QFileDialog::getSaveFileName(
        this,
        "Export Archive to PDF",
        "SmartLibrarian_Archive.pdf",
        "PDF Files (*.pdf)"
    );

    if (savePath.isEmpty()) return;

    try {
        pdf::PdfDocument doc;
        doc.addFont("F1", "Helvetica");
        doc.addFont("F2", "Helvetica-Bold");
        doc.addFont("F3", "Courier");

        // Cover page
        {
            pdf::PdfPage& page = doc.addPage();

            page.setFillColor(0.1f, 0.3f, 0.6f);
            page.fillRect(0, 750, 612, 42);

            page.beginText();
            page.setTextColor(1.0f, 1.0f, 1.0f);
            page.setFont("F2", 20.0f);
            page.moveTo(72.0f, 762.0f);
            page.showText("SmartLibrarian — Archive Export");
            page.endText();

            page.setStrokeColor(0.1f, 0.3f, 0.6f);
            page.drawLine(72, 730, 540, 730, 1.0f);

            page.beginText();
            page.setTextColor(0.1f, 0.1f, 0.1f);
            page.setFont("F2", 12.0f);
            page.moveTo(72.0f, 700.0f);
            page.showText("Exported Documents: " +
                          std::to_string(selectedRows.size()));

            page.setFont("F1", 11.0f);
            page.setLeading(18.0f);
            page.moveTo(72.0f, 675.0f);

            for (int row : selectedRows) {
                const auto& adoc = m_documents[
                    static_cast<std::size_t>(row)];
                page.showText("• " + adoc.filename +
                              "  [" + std::to_string(adoc.wordCount) +
                              " words, confidence: " +
                              std::to_string(
                                  static_cast<int>(adoc.ocrConfidence)) +
                              "%]");
                page.nextLine();
            }
            page.endText();
        }

        // One page per selected document showing extracted text
        for (int row : selectedRows) {
            const auto& adoc = m_documents[
                static_cast<std::size_t>(row)];

            pdf::PdfPage& page = doc.addPage();

            page.setFillColor(0.2f, 0.5f, 0.2f);
            page.fillRect(0, 750, 612, 42);

            page.beginText();
            page.setTextColor(1.0f, 1.0f, 1.0f);
            page.setFont("F2", 14.0f);
            page.moveTo(72.0f, 762.0f);
            page.showText(adoc.filename);
            page.endText();

            page.setStrokeColor(0.2f, 0.5f, 0.2f);
            page.drawLine(72, 730, 540, 730, 1.0f);

            // Extracted text — word-wrap at ~80 chars
            page.beginText();
            page.setTextColor(0.1f, 0.1f, 0.1f);
            page.setFont("F3", 9.0f);
            page.setLeading(13.0f);
            page.moveTo(72.0f, 710.0f);

            std::string text = adoc.extractedText;
            std::size_t lineStart = 0;
            int linesOnPage = 0;
            const int maxLines = 55;
            const std::size_t lineWidth = 90;

            while (lineStart < text.size() && linesOnPage < maxLines) {
                std::size_t end = std::min(lineStart + lineWidth,
                                            text.size());
                // Find last space for word-wrap
                if (end < text.size()) {
                    std::size_t lastSpace = text.rfind(' ', end);
                    if (lastSpace > lineStart) end = lastSpace;
                }
                page.showText(text.substr(lineStart, end - lineStart));
                page.nextLine();
                lineStart = (end < text.size() && text[end] == ' ')
                    ? end + 1 : end;
                ++linesOnPage;
            }
            page.endText();
        }

        doc.save(savePath.toStdString());

        QMessageBox::information(this, "Export Complete",
            "PDF exported successfully:\n" + savePath);

        emit statusMessage("PDF exported: " + savePath);
    }
    catch (const std::exception& ex) {
        QMessageBox::critical(this, "Export Failed",
            QString("Failed to export PDF:\n") + ex.what());
    }
}

void ArchiveWidget::onClearArchiveClicked()
{
    if (m_documents.empty()) return;

    auto reply = QMessageBox::question(
        this, "Clear Archive",
        "Clear all archived documents?\n"
        "This does not delete the original image files.",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_documents.clear();
        m_table->setRowCount(0);
        if (m_engine) m_engine->clear();
        m_statsLabel->setText("Archive cleared.");
        emit statusMessage("Archive cleared.");
    }
}

void ArchiveWidget::onSelectionChanged()
{
    m_exportBtn->setEnabled(
        !m_table->selectedItems().isEmpty());
}

} // namespace gui
} // namespace sl
