#pragma once

#include <QWidget>
#include <QString>
#include <QStringList>

namespace sl {
namespace gui {

/**
 * DropZone
 *
 * A widget that accepts dragged image files and emits a signal
 * when files are dropped onto it.
 *
 * Qt drag-and-drop requires overriding two event handlers:
 *
 *   dragEnterEvent(QDragEnterEvent*)
 *     Called when the user drags something OVER the widget.
 *     We inspect the MIME data to check if it contains file URLs.
 *     If yes, call event->acceptProposedAction() to signal
 *     "I can accept this drop."
 *     If no, ignore the event — the OS will show a "not allowed" cursor.
 *
 *   dropEvent(QDropEvent*)
 *     Called when the user RELEASES the drag over the widget.
 *     We extract the file URLs, filter for supported image types,
 *     and emit the imageDropped signal.
 *
 * Visual feedback:
 *   We override paintEvent to draw a dashed border and instruction
 *   text. We track m_dragActive to change appearance when something
 *   is being dragged over the widget — live visual feedback.
 *
 * Why override QWidget instead of using a QLabel?
 *   QLabel doesn't accept drops by default and doesn't give us
 *   paintEvent control for the custom dashed border styling.
 *   A clean QWidget override gives us full control.
 */
class DropZone : public QWidget
{
    Q_OBJECT

public:
    explicit DropZone(QWidget* parent = nullptr);
    ~DropZone() override = default;

    QSize sizeHint() const override;

signals:
    /**
     * imageDropped — emitted once per dropped image file.
     * @param filePath  absolute path to the dropped image
     */
    void imageDropped(const QString& filePath);

protected:
    // Qt event overrides
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event)           override;
    void paintEvent(QPaintEvent* event)         override;

private:
    bool m_dragActive { false }; // true when drag is hovering over us
};

} // namespace gui
} // namespace sl
