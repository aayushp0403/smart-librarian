#include "gui/SearchWidget.h"
#include "search/SearchEngine.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidgetItem>
#include <QFont>
#include <QFileInfo>
#include <sstream>
#include <iomanip>

namespace sl {
namespace gui {

SearchWidget::SearchWidget(sl::search::SearchEngine* engine,
                           QWidget* parent)
    : QWidget(parent)
    , m_engine(engine)
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(14, 14, 14, 14);

    // ── Title ─────────────────────────────────────────────────────
    auto* title = new QLabel("Document Search", this);
    QFont tf = title->font();
    tf.setPointSize(13);
    tf.setBold(true);
    title->setFont(tf);
    title->setStyleSheet("color: #222;");
    layout->addWidget(title);

    // ── Query row ─────────────────────────────────────────────────
    auto* row = new QHBoxLayout();

    m_queryInput = new QLineEdit(this);
    m_queryInput->setPlaceholderText(
        "Search indexed documents...");
    m_queryInput->setMinimumHeight(38);
    m_queryInput->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #CCC;"
        "  border-radius: 6px;"
        "  padding: 4px 12px;"
        "  font-size: 12px;"
        "}"
        "QLineEdit:focus { border: 2px solid #2088FF; }"
    );
    row->addWidget(m_queryInput, 1);

    auto* btn = new QPushButton("Search", this);
    btn->setMinimumHeight(38);
    btn->setStyleSheet(
        "QPushButton {"
        "  background: #2088FF; color: white;"
        "  border-radius: 6px; padding: 0 18px;"
        "  font-weight: bold; font-size: 12px;"
        "}"
        "QPushButton:hover  { background: #1070EE; }"
        "QPushButton:pressed{ background: #0055CC; }"
    );
    row->addWidget(btn);
    layout->addLayout(row);

    // ── Autocomplete suggestions ──────────────────────────────────
    m_suggestions = new QListWidget(this);
    m_suggestions->setMaximumHeight(110);
    m_suggestions->hide();
    m_suggestions->setStyleSheet(
        "QListWidget {"
        "  border: 1px solid #2088FF;"
        "  border-radius: 4px; font-size: 11px;"
        "}"
        "QListWidget::item { padding: 4px 10px; }"
        "QListWidget::item:hover    { background: #E8F4FF; }"
        "QListWidget::item:selected { background: #2088FF; color: white; }"
    );
    layout->addWidget(m_suggestions);

    // ── Stats bar ─────────────────────────────────────────────────
    m_statsLabel = new QLabel("No documents indexed yet.", this);
    m_statsLabel->setStyleSheet(
        "color: #888; font-size: 10px; padding: 2px 0;");
    layout->addWidget(m_statsLabel);

    // ── Results list ──────────────────────────────────────────────
    m_resultsList = new QListWidget(this);
    m_resultsList->setSpacing(2);
    m_resultsList->setAlternatingRowColors(false);
    m_resultsList->setStyleSheet(
        "QListWidget {"
        "  border: 1px solid #DDD;"
        "  border-radius: 6px;"
        "  background: white;"
        "  outline: none;"
        "}"
        "QListWidget::item {"
        "  border-bottom: 1px solid #F0F0F0;"
        "  padding: 6px 8px;"
        "}"
        "QListWidget::item:hover {"
        "  background: #F5F9FF;"
        "}"
        "QListWidget::item:selected {"
        "  background: #E8F2FF;"
        "  color: #000;"
        "  border-left: 3px solid #2088FF;"
        "}"
    );
    layout->addWidget(m_resultsList, 1);

    // ── Connections ───────────────────────────────────────────────
    connect(m_queryInput, &QLineEdit::textChanged,
            this, &SearchWidget::onQueryChanged);
    connect(m_queryInput, &QLineEdit::returnPressed,
            this, &SearchWidget::onSearchTriggered);
    connect(btn, &QPushButton::clicked,
            this, &SearchWidget::onSearchTriggered);
    connect(m_resultsList, &QListWidget::itemDoubleClicked,
            this, &SearchWidget::onResultDoubleClicked);
    connect(m_suggestions, &QListWidget::itemClicked,
            [this](QListWidgetItem* item) {
                m_queryInput->setText(item->text());
                m_suggestions->hide();
                onSearchTriggered();
            });

    // Show initial stats
    updateStats();
}

// ─────────────────────────────────────────────────────────────────────
// onQueryChanged — live autocomplete on every keystroke
// ─────────────────────────────────────────────────────────────────────

void SearchWidget::onQueryChanged(const QString& text)
{
    if (text.trimmed().isEmpty()) {
        m_suggestions->hide();
        m_resultsList->clear();
        updateStats();
        return;
    }

    // Autocomplete for the last word being typed
    QStringList parts = text.split(' ', Qt::SkipEmptyParts);
    if (!parts.isEmpty()) {
        showSuggestions(parts.last());
    }
}

void SearchWidget::onSearchTriggered()
{
    m_suggestions->hide();
    performSearch(m_queryInput->text().trimmed());
}

// ─────────────────────────────────────────────────────────────────────
// showSuggestions
// ─────────────────────────────────────────────────────────────────────

void SearchWidget::showSuggestions(const QString& prefix)
{
    if (!m_engine || prefix.length() < 2) {
        m_suggestions->hide();
        return;
    }

    auto words = m_engine->prefixSearch(prefix.toStdString(), 7);
    if (words.empty()) {
        m_suggestions->hide();
        return;
    }

    m_suggestions->clear();
    for (const auto& w : words) {
        m_suggestions->addItem(QString::fromStdString(w));
    }
    m_suggestions->show();
}

// ─────────────────────────────────────────────────────────────────────
// performSearch — the main display logic
//
// Each result item shows four pieces of information:
//   Line 1: rank number + document title
//   Line 2: relevance score bar + occurrence count + filename
//
// The "relevance bar" is built from Unicode block characters
// (█) scaled to the score. This gives instant visual ranking
// without needing a chart widget.
//
// We normalize scores so the top result always shows a full bar.
// ─────────────────────────────────────────────────────────────────────

void SearchWidget::performSearch(const QString& query)
{
    if (!m_engine || query.isEmpty()) return;

    m_resultsList->clear();
    m_lastQuery = query;

    auto results = m_engine->search(query.toStdString(), 20);

    if (results.empty()) {
        auto* item = new QListWidgetItem();
        item->setText(
            "  No results found for: \"" + query + "\"\n"
            "  Try a different word or check the Search tab after "
            "indexing an image.");
        item->setForeground(QColor(0x99, 0x99, 0x99));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_resultsList->addItem(item);
        updateStats(query, 0);
        return;
    }

    // Find max score for normalization (top result = full bar)
    double maxScore = results.front().score;

    for (std::size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];

        // ── Relevance bar ─────────────────────────────────────────
        // Normalize to 0-10 blocks
        int barLen = (maxScore > 0.0)
            ? static_cast<int>((r.score / maxScore) * 10.0)
            : 0;
        barLen = std::max(1, std::min(barLen, 10));

        QString bar;
        for (int b = 0; b < barLen; ++b)      bar += "█";
        for (int b = barLen; b < 10; ++b)     bar += "░";

        // ── Occurrence description ────────────────────────────────
        // totalTermFreq: total times matched terms appear in this doc
        // matchCount:    how many distinct query terms matched
        QString occurrenceStr;
        if (r.totalTermFreq == 1) {
            occurrenceStr = "appears 1 time";
        } else {
            occurrenceStr = QString("appears %1 times")
                .arg(r.totalTermFreq);
        }

        if (r.matchCount > 1) {
            occurrenceStr += QString("  (%1 query terms matched)")
                .arg(r.matchCount);
        }

        // ── Filename (shorter than full path) ─────────────────────
        QString displayPath = QString::fromStdString(r.path);
        QFileInfo fi(displayPath);
        QString shortName = fi.fileName().isEmpty()
            ? displayPath
            : fi.fileName();

        // ── Score formatted ───────────────────────────────────────
        std::ostringstream scoreStr;
        scoreStr << std::fixed << std::setprecision(4) << r.score;

        // ── Assemble two-line item text ───────────────────────────
        QString line = QString(
            "%1. %2\n"
            "     %3  |  %4  |  score: %5")
            .arg(i + 1)
            .arg(QString::fromStdString(r.title))
            .arg(bar)
            .arg(occurrenceStr)
            .arg(QString::fromStdString(scoreStr.str()));

        auto* item = new QListWidgetItem(line);

        // Store path for double-click
        item->setData(Qt::UserRole,
                      QString::fromStdString(r.path));

        // Color the rank number slightly
        if (i == 0) {
            // Top result: slightly highlighted
            item->setBackground(QColor(0xF0, 0xF7, 0xFF));
        }

        // Font: slightly larger for the title line
        QFont itemFont = item->font();
        itemFont.setPointSize(10);
        item->setFont(itemFont);

        m_resultsList->addItem(item);
    }

    updateStats(query, static_cast<int>(results.size()));
}

void SearchWidget::onResultDoubleClicked(QListWidgetItem* item)
{
    QString path = item->data(Qt::UserRole).toString();
    if (!path.isEmpty()) {
        m_statsLabel->setText("Selected: " + path);
    }
}

// ─────────────────────────────────────────────────────────────────────
// refreshResults / updateStats
// ─────────────────────────────────────────────────────────────────────

void SearchWidget::refreshResults()
{
    // Re-run the last query so new documents show up immediately
    if (!m_lastQuery.isEmpty()) {
        performSearch(m_lastQuery);
    }
    updateStats(m_lastQuery,
                m_resultsList->count());
}

void SearchWidget::updateStats(const QString& query, int resultCount)
{
    if (!m_engine) return;

    QString stats;
    if (query.isEmpty()) {
        stats = QString(
            "Index: %1 documents  |  %2 unique words  |  %3 trie nodes")
            .arg(m_engine->documentCount())
            .arg(m_engine->uniqueWordCount())
            .arg(m_engine->trieNodeCount());
    } else {
        stats = QString(
            "%1 result(s) for \"%2\"  |  "
            "Index: %3 docs  |  %4 unique words")
            .arg(resultCount)
            .arg(query)
            .arg(m_engine->documentCount())
            .arg(m_engine->uniqueWordCount());
    }
    m_statsLabel->setText(stats);
}

} // namespace gui
} // namespace sl
