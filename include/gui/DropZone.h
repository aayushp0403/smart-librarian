#pragma once

#include <QWidget>
#include <QString>
#include <QPushButton>

namespace sl {
namespace gui {

/**
 * DropZone
 *
 * Accepts image files via two methods:
 *   1. Drag and drop (works when running with a real X11 session
 *      where cross-process DnD is available)
 *   2. Browse button — opens QFileDialog (always works, including WSL2)
 *
 * The browse button is the primary interaction path for WSL2 users
 * because Windows→WSL2 drag-and-drop requires special X11 bridge
 * configuration that most users don't have set up.
 *
 * Both paths emit the same imageDropped(QString) signal so
 * MainWindow doesn't need to know which method was used.
 */
class DropZone : public QWidget
{
    Q_OBJECT

public:
    explicit DropZone(QWidget* parent = nullptr);
    ~DropZone() override = default;

    QSize sizeHint() const override;

signals:
    void imageDropped(const QString& filePath);

public slots:
    /**
     * openFileBrowser — show QFileDialog and emit imageDropped
     * for each selected file.
     * Can be called externally (e.g., from a toolbar button).
     */
    void openFileBrowser();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event)           override;
    void paintEvent(QPaintEvent* event)         override;
    void mousePressEvent(QMouseEvent* event)    override;

private:
    bool         m_dragActive  { false };
    QPushButton* m_browseBtn   { nullptr };
};

} // namespace gui
} // namespace sl
