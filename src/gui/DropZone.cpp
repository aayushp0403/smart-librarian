#include "gui/DropZone.h"
#include "ocr/ImageLoader.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QUrl>
#include <QFileInfo>

namespace sl {
namespace gui {

DropZone::DropZone(QWidget* parent)
    : QWidget(parent)
{
    // Tell Qt this widget accepts drop events.
    // Without this, dragEnterEvent is never called.
    setAcceptDrops(true);

    // Minimum size so the zone is always visible
    setMinimumHeight(120);

    // Style: slightly rounded, visible background
    setStyleSheet(
        "DropZone {"
        "  background-color: #F8F9FA;"
        "  border-radius: 8px;"
        "}"
    );
}

QSize DropZone::sizeHint() const
{
    return QSize(500, 140);
}

// ─────────────────────────────────────────────────────────────────────
// dragEnterEvent
//
// The user dragged something over us. We inspect the MIME data.
// MIME (Multipurpose Internet Mail Extensions) is a standard
// for describing data types. File drag-and-drop uses:
//   "text/uri-list" — a list of file:// URLs
//
// If the drag contains file URLs, we accept it and change our
// visual state. Otherwise we ignore it (the event propagates
// to parent widgets and eventually the OS shows rejection cursor).
// ─────────────────────────────────────────────────────────────────────

void DropZone::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        // Check if any URL is a supported image file
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl& url : urls) {
            if (url.isLocalFile()) {
                QString path = url.toLocalFile();
                if (sl::ocr::ImageLoader::isSupported(path.toStdString())) {
                    event->acceptProposedAction();
                    m_dragActive = true;
                    update(); // trigger repaint for visual feedback
                    return;
                }
            }
        }
    }
    event->ignore();
}

// ─────────────────────────────────────────────────────────────────────
// dragLeaveEvent — user dragged away without dropping
// ─────────────────────────────────────────────────────────────────────

void DropZone::dragLeaveEvent(QDragLeaveEvent* event)
{
    m_dragActive = false;
    update();
    event->accept();
}

// ─────────────────────────────────────────────────────────────────────
// dropEvent — user released the drag
//
// Extract file URLs from the MIME data.
// For each valid image file, emit imageDropped().
// One drop can contain multiple files.
// ─────────────────────────────────────────────────────────────────────

void DropZone::dropEvent(QDropEvent* event)
{
    m_dragActive = false;
    update();

    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }

    const QList<QUrl> urls = event->mimeData()->urls();
    bool accepted = false;

    for (const QUrl& url : urls) {
        if (url.isLocalFile()) {
            QString path = url.toLocalFile();
            if (sl::ocr::ImageLoader::isSupported(path.toStdString())) {
                emit imageDropped(path);
                accepted = true;
            }
        }
    }

    if (accepted) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

// ─────────────────────────────────────────────────────────────────────
// paintEvent — custom rendering
//
// QPainter is Qt's 2D drawing API.
// We draw:
//   - A dashed border rectangle (changes color when drag is active)
//   - A center-aligned instruction text
//   - An icon hint (↓ unicode arrow)
//
// QPainter must be constructed with 'this' as the device,
// and is only valid within paintEvent. It is stack-allocated
// and its destructor flushes the drawing commands to the screen.
// ─────────────────────────────────────────────────────────────────────

void DropZone::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Colors: change when drag is hovering
    QColor borderColor = m_dragActive
        ? QColor(0x20, 0x88, 0xFF)   // bright blue when active
        : QColor(0xCC, 0xCC, 0xCC);  // gray when idle

    QColor bgColor = m_dragActive
        ? QColor(0xE8, 0xF4, 0xFF)   // light blue tint when active
        : QColor(0xF8, 0xF9, 0xFA);  // near-white when idle

    // Background
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(rect().adjusted(1,1,-1,-1), 8, 8);

    // Dashed border
    QPen borderPen(borderColor, 2, Qt::DashLine);
    painter.setPen(borderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect().adjusted(1,1,-1,-1), 8, 8);

    // Icon and text
    painter.setPen(m_dragActive
        ? QColor(0x20, 0x88, 0xFF)
        : QColor(0x99, 0x99, 0x99));

    QFont iconFont = painter.font();
    iconFont.setPointSize(28);
    painter.setFont(iconFont);
    painter.drawText(
        QRect(rect().x(), rect().y(), rect().width(), rect().height() / 2),
        Qt::AlignHCenter | Qt::AlignBottom,
        m_dragActive ? "⬇" : "📄"
    );

    QFont textFont = painter.font();
    textFont.setPointSize(11);
    painter.setFont(textFont);
    painter.drawText(
        QRect(rect().x(), rect().height()/2,
              rect().width(), rect().height()/2),
        Qt::AlignHCenter | Qt::AlignTop,
        m_dragActive
            ? "Release to process image"
            : "Drop image here  (PNG, JPG, TIFF)"
    );
}

} // namespace gui
} // namespace sl
