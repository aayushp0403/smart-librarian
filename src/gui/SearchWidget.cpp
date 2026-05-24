#include "gui/SearchWidget.h"
#include "search/SearchEngine.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidgetItem>
#include <QFont>
#include <sstream>
#include <iomanip>

namespace sl {
namespace gui {

SearchWidget::SearchWidget(sl::search::SearchEngine* engine, QWidget* parent)
    : QWidget(parent)
    , m_engine(engine)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // Title
    auto* titleLabel = new QLabel("  Document Search", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // Query input row
    auto* inputRow = new QHBoxLayout();

    m_queryInput = new QLineEdit(this);
    m_queryInput->setPlaceholderText("Type to search indexed documents...");
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
        "QPushButton:hover  { background-color: #1070EE; }"
        "QPushButton:pressed{ background-color: #0060DD; }"
    );
    inputRow->addWidget(searchBtn);
    mainLayout->addLayout(inputRow);

    // Autocomplete suggestions (hidden until needed)
    m_suggestions = new QListWidget(this);
    m_suggestions->setMaximumHeight(120);
    m_suggestions->hide();
    m_suggestions->setStyleSheet(
        "QListWidget {"
        "  border: 1px solid #2088FF;"
        "  border-radius: 4px;"
        "  font-size: 11px;"
        "}"
        "QListWidget::item:hover    { background: #E8F4FF; }"
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
        "QListWidget::item           { padding: 8px; }"
        "QListWidget::item:hover     { background: #F0F7FF; }"
        "QListWidget::item:selected  { background: #2088FF; color: white; }"
    );
    mainLayout->addWidget(m_resultsList, 1);

    // ── Connections ───────────────────────────────────────────────
    connect(m_queryInput, &QLineEdit::textChanged,
            this, &SearchWidget::onQueryChanged);

    connect(m_queryInput, &QLineEdit::returnPressed,
            this, &SearchWidget::onSearchTriggered);

    connect(searchBtn, &QPushButton::clicked,
            this, &SearchWidget::onSearchTriggered);

    connect(m_resultsList, &QListWidget::itemDoubleClicked,
            this, &SearchWidget::onResultDoubleClicked);

    connect(m_suggestions, &QListWidget::itemClicked,
            [this](QListWidgetItem* item) {
                m_queryInput->setText(item->text());
                m_suggestions->hide();
                onSearchTriggered();
            });
}

// ─────────────────────────────────────────────────────────────────────
// Slot: called on every keystroke
// ─────────────────────────────────────────────────────────────────────

void SearchWidget::onQueryChanged(const QString& text)
{
    if (text.trimmed().isEmpty()) {
        m_suggestions->hide();
        m_resultsList->clear();
        updateStats();
        return;
    }

    // Show suggestions for the last typed word
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
// showSuggestions — Trie prefix lookup
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
// performSearch — full TF-IDF search
// ─────────────────────────────────────────────────────────────────────

void SearchWidget::performSearch(const QString& query)
{
    if (!m_engine || query.isEmpty()) return;

    m_resultsList->clear();

    auto results = m_engine->search(query.toStdString(), 20);

    if (results.empty()) {
        auto* item = new QListWidgetItem(
            "  No results found for: \"" + query + "\"");
        item->setForeground(QColor(0x99, 0x99, 0x99));
        m_resultsList->addItem(item);
        updateStats(query, 0);
        return;
    }

    for (const auto& result : results) {
        std::ostringstream scoreStr;
        scoreStr << std::fixed << std::setprecision(4) << result.score;

        QString line = QString("[%1]  %2\n"
                               "      %3 matched terms | %4")
            .arg(QString::fromStdString(scoreStr.str()))
            .arg(QString::fromStdString(result.title))
            .arg(result.matchCount)
            .arg(QString::fromStdString(result.path));

        auto* item = new QListWidgetItem(line);
        item->setData(Qt::UserRole,
                      QString::fromStdString(result.path));
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
// Single declaration with default params — fixes the vtable link error
// ─────────────────────────────────────────────────────────────────────

void SearchWidget::refreshResults()
{
    QString current = m_queryInput->text().trimmed();
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
        stats = QString("Index: %1 docs  |  %2 unique words  |  %3 trie nodes")
            .arg(m_engine->documentCount())
            .arg(m_engine->uniqueWordCount())
            .arg(m_engine->trieNodeCount());
    } else {
        stats = QString("Found %1 result(s) for \"%2\"  |  "
                        "Index: %3 docs  |  %4 words")
            .arg(resultCount)
            .arg(query)
            .arg(m_engine->documentCount())
            .arg(m_engine->uniqueWordCount());
    }
    m_statsLabel->setText(stats);
}

} // namespace gui
} // namespace sl
