#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>

namespace sl { namespace search { class SearchEngine; } }

namespace sl {
namespace gui {

class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SearchWidget(sl::search::SearchEngine* engine,
                          QWidget* parent = nullptr);
    ~SearchWidget() override = default;

    void refreshResults();

private slots:
    void onQueryChanged(const QString& text);
    void onSearchTriggered();
    void onResultDoubleClicked(QListWidgetItem* item);

private:
    void performSearch(const QString& query);
    void showSuggestions(const QString& prefix);

    // Single declaration with default parameters — avoids overload
    // ambiguity that causes vtable/staticMetaObject linker errors
    void updateStats(const QString& query = QString(),
                     int resultCount = 0);

    sl::search::SearchEngine* m_engine { nullptr };

    QLineEdit*   m_queryInput  { nullptr };
    QListWidget* m_resultsList { nullptr };
    QLabel*      m_statsLabel  { nullptr };
    QListWidget* m_suggestions { nullptr };
};

} // namespace gui
} // namespace sl
