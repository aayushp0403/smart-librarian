#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <memory>

namespace sl { namespace search { class SearchEngine; } }

namespace sl {
namespace gui {

/**
 * SearchWidget
 *
 * A self-contained search interface:
 *   - QLineEdit for query input
 *   - QListWidget for displaying results
 *   - Live prefix autocomplete as the user types
 *   - Full search on Enter
 *
 * Architecture:
 *   SearchWidget holds a non-owning pointer to the SearchEngine.
 *   The SearchEngine is owned by MainWindow.
 *   This is a dependency injection pattern: SearchWidget
 *   doesn't construct or destroy the engine, it just uses it.
 *
 *   Why not pass by reference?
 *     Because SearchWidget is a QObject — its constructor takes
 *     QObject* parent. We can't mix reference initialization with
 *     Qt's object parenting system cleanly. A pointer makes
 *     the "borrowed" ownership semantics explicit.
 */
class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SearchWidget(sl::search::SearchEngine* engine,
                          QWidget* parent = nullptr);
    ~SearchWidget() override = default;

    /**
     * refreshResults — re-run the current query.
     * Called after new documents are indexed.
     */
    void refreshResults();

private slots:
    void onQueryChanged(const QString& text);
    void onSearchTriggered();
    void onResultDoubleClicked(QListWidgetItem* item);

private:
    void performSearch(const QString& query);
    void showSuggestions(const QString& prefix);
void updateStats(const QString& query = "", int resultCount = 0);
    sl::search::SearchEngine* m_engine;   // borrowed, not owned

    QLineEdit*   m_queryInput   { nullptr };
    QListWidget* m_resultsList  { nullptr };
    QLabel*      m_statsLabel   { nullptr };
    QListWidget* m_suggestions  { nullptr };
};

} // namespace gui
} // namespace sl
