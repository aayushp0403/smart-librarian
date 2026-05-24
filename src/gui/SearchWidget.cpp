#include "gui/SearchWidget.h"
#include "search/SearchEngine.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidgetItem>
#include <QFont>
#include <QTimer>
#include <sstream>
#include <iomanip>

namespace sl {
namespace gui {

SearchWidget::SearchWidget(sl::search::SearchEngine* engine, QWidget* parent)
    : QWidget(parent)
    , m_engine(engine)
{
    // ── Build the layout ─────────────────────────────────────────
    // Qt layout system:
    //   QVBoxLayout stacks children vertically (top to bottom)
    //   QHBoxLayout places children horizontally (left to right)
    //   Layouts automatically handle resize and spacing

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // Title
    auto* titleLabel = new QLabel("🔍  Document Search", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // Query input row
    auto* inputRow = new QHBoxLayout();

    m_queryInput = new QLineEdit(this);
    m_queryInput->setPlaceholderText(
        "Type to search indexed documents...");
    m_queryInput->setMinimumHeight(36);
    m_queryInput->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #CCC;"
        "  border-radius: 6px;"
        "  padding: 4px 10px;"
        "  font-size: 12px;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #2088FF;"
        "}"
    );
    inputRow->addWidget(m_queryInput);

    auto* searchBtn = new QPushButton("Search", this);
    searchBtn->setMinimumHeight(36);
    searchBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2088FF;"
        "  color: white;"
        "  border-radius: 6px;"
        "  padding: 4px 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1070EE;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #0060DD;"
        "}"
    );
    inputRow->addWidget(searchBtn);
    mainLayout->addLayout(inputRow);

    // Autocomplete suggestions dropdown (hidden by default)
    m_suggestions = new QListWidget(this);
    m_suggestions->setMaximumHeight(120);
    m_suggestions->hide();
    m_suggestions->setStyleSheet(
        "QListWidget {"
        "  border: 1px solid #2088FF;"
        "  border-radius: 4px;"
        "  font-size: 11px;"
        "}"
        "QListWidget::item:hover { background: #E8F4FF; }"
        "QListWidget::item:selected { background: #2088FF; color: white; }"
    );
    mainLayout->addWidget(m_suggestions);

    // Stats label
    m_statsLabel = new QLabel("No documents indexed yet.", this);
    m_statsLabel->setStyleSheet("color: #888; font-size: 10px;");
    mainLayout->addWidget(m_statsLabel);

    // Results list
    m_resultsList = new QListWidget(this);
    m_resultsList->setAlternatingRowColors(true);
    m_resultsList->setStyleSheet(
        "QListWidget {"
        "  border: 1px solid #DDD;"
        "  border-radius: 6px;"
        "  font-size: 11px;"
        "}"
        "QListWidget::item { padding: 8px; }"
        "QListWidget::item:hover { background: #F0F7FF; }"
        "QListWidget::item:selected { background: #2088FF; color: white; }"
    );
    mainLayout->addWidget(m_resultsList, 1); // stretch factor 1 = expand

    // ── Connections ──────────────────────────────────────────────
    // connect(sender, signal, receiver, slot)
    //
    // &QLineEdit::textChanged is the signal — emitted every keystroke
    // &SearchWidget::onQueryChanged is our slot — runs the search
    //
    // Lambda connection for the button (simpler for single-use):
    connect(m_queryInput, &QLineEdit::textChanged,
            this, &SearchWidget::onQueryChanged);

    connect(m_queryInput, &QLineEdit::returnPressed,
            this, &SearchWidget::onSearchTriggered);

    connect(searchBtn, &QPushButton::clicked,
            this, &SearchWidget::onSearchTriggered);

    connect(m_resultsList, &QListWidget::itemDoubleClicked,
            this, &SearchWidget::onResultDoubleClicked);

    // Clicking a suggestion fills the query input
    connect(m_suggestions, &QListWidget::itemClicked,
            [this](QListWidgetItem* item) {
                m_queryInput->setText(item->text());
                m_suggestions->hide();
                onSearchTriggered();
            });
}

// ─────────────────────────────────────────────────────────────────────
// onQueryChanged — called on every keystroke
//
// We don't search on every keystroke — that's wasteful for slow
// queries. Instead we show autocomplete suggestions (cheap: Trie lookup)
// and only run the full search on Enter or button click.
// ─────────────────────────────────────────────────────────────────────

void SearchWidget::onQueryChanged(const QString& text)
{
    if (text.isEmpty()) {
        m_suggestions->hide();
        m_resultsList->clear();
        updateStats();
        return;
    }

    // Show prefix suggestions for the last typed word
    // Extract last word: "neural net" → prefix = "net"
    QString prefix = text.split(' ', Qt::SkipEmptyParts).last();
    showSuggestions(prefix);
}

void SearchWidget::onSearchTriggered()
{
    m_suggestions->hide();
    performSearch(m_queryInput->text());
}

// ─────────────────────────────────────────────────────────────────────
// showSuggestions — prefix autocomplete via Trie
// ─────────────────────────────────────────────────────────────────────

void SearchWidget::showSuggestions(const QString& prefix)
{
    if (!m_engine || prefix.length() < 2) {
        m_suggestions->hide();
        return;
    }

    auto words = m_engine->prefixSearch(prefix.toStdString(), 6);

    if (words.empty()) {
        m_suggestions->hide();
        return;
    }

    m_suggestions->clear();
    for (const auto& word : words) {
        m_suggestions->addItem(QString::fromStdString(word));
    }
    m_suggestions->show();
}

// ─────────────────────────────────────────────────────────────────────
// performSearch — full TF-IDF ranked search
// ─────────────────────────────────────────────────────────────────────

void SearchWidget::performSearch(const QString& query)
{
    if (!m_engine || query.trimmed().isEmpty()) return;

    m_resultsList->clear();

    auto results = m_engine->search(query.toStdString(), 20);

    if (results.empty()) {
        auto* item = new QListWidgetItem(
            "No results found for: \"" + query + "\"");
        item->setForeground(QColor(0x99, 0x99, 0x99));
        m_resultsList->addItem(item);
        return;
    }

    for (const auto& result : results) {
        // Format each result as a rich two-line item
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << result.score;

        QString display =
            QString("📄  %1\n"
                    "    Score: %2  |  Matched terms: %3  |  %4")
            .arg(QString::fromStdString(result.title))
            .arg(QString::fromStdString(oss.str()))
            .arg(result.matchCount)
            .arg(QString::fromStdString(result.path));

        auto* item = new QListWidgetItem(display);
        // Store doc info as item data for double-click handler
        item->setData(Qt::UserRole,
                      QString::fromStdString(result.path));
        m_resultsList->addItem(item);
    }

    updateStats(query, static_cast<int>(results.size()));
}

void SearchWidget::onResultDoubleClicked(QListWidgetItem* item)
{
    // Future: open document in viewer
    QString path = item->data(Qt::UserRole).toString();
    if (!path.isEmpty()) {
        // For now: show path in status
        m_statsLabel->setText("Selected: " + path);
    }
}

void SearchWidget::refreshResults()
{
    QString current = m_queryInput->text();
    if (!current.isEmpty()) {
        performSearch(current);
    }
    updateStats();
}

void SearchWidget::updateStats(const QString& query, int resultCount)
{
    if (!m_engine) return;

    QString stats;
    if (query.isEmpty()) {
        stats = QString("Index: %1 documents  |  %2 unique words  |  "
                        "%3 trie nodes")
            .arg(m_engine->documentCount())
            .arg(m_engine->uniqueWordCount())
            .arg(m_engine->trieNodeCount());
    } else {
        stats = QString("Found %1 results  |  Index: %2 docs  |  "
                        "%3 unique words")
            .arg(resultCount)
            .arg(m_engine->documentCount())
            .arg(m_engine->uniqueWordCount());
    }
    m_statsLabel->setText(stats);
}

} // namespace gui
} // namespace sl
