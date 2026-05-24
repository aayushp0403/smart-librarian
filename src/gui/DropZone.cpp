#include "gui/DropZone.h"
#include "ocr/ImageLoader.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QUrl>
#include <QFileInfo>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QCursor>

namespace sl {
namespace gui {

DropZone::DropZone(QWidget* parent)
    : QWidget(parent)
{
    setAcceptDrops(true);
    setMinimumHeight(160);
    setCursor(Qt::PointingHandCursor);

    // ── Browse button ─────────────────────────────────────────────
    // Positioned at the bottom of the drop zone.
    // We use a real QPushButton so it gets keyboard focus,
    // hover states, and accessibility for free.
    m_browseBtn = new QPushButton("Browse for Image...", this);
    m_browseBtn->setFixedHeight(32);
    m_browseBtn->setCursor(Qt::PointingHandCursor);
    m_browseBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2088FF;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 5px;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "  padding: 0 16px;"
        "}"
        "QPushButton:hover  { background-color: #1070EE; }"
        "QPushButton:pressed{ background-color: #0055CC; }"
    );

    // Layout: push button to bottom, leave top area for drop target
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 10);
    layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(m_browseBtn);

    // Connect button click → openFileBrowser slot
    connect(m_browseBtn, &QPushButton::clicked,
            this, &DropZone::openFileBrowser);
}

QSize DropZone::sizeHint() const
{
    return QSize(300, 170);
}

// ─────────────────────────────────────────────────────────────────────
// openFileBrowser
//
// QFileDialog::getOpenFileNames — lets user select one or more files.
//
// Filter string format: "Display Name (*.ext1 *.ext2);;Other (*.ext3)"
// The ;; separator creates separate filter categories in the dialog.
//
// Why getOpenFileNames (plural) instead of getOpenFileName?
//   Allows processing multiple images in one operation.
//   Each selected file emits a separate imageDropped signal.
//   MainWindow queues them — OCR processes one at a time.
// ─────────────────────────────────────────────────────────────────────

void DropZone::openFileBrowser()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,                          // parent widget (centers dialog)
        "Select Image File(s)",        // dialog title
        QString(),                     // starting directory (empty = last used)
        "Image Files (*.png *.jpg *.jpeg *.bmp *.tiff *.tif *.pnm);;"
        "PNG Images (*.png);;"
        "JPEG Images (*.jpg *.jpeg);;"
        "TIFF Images (*.tif *.tiff);;"
        "All Files (*)"
    );

    // Emit imageDropped for each selected file
    for (const QString& path : files) {
        if (!path.isEmpty()) {
            emit imageDropped(path);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────
// mousePressEvent
//
// Clicking anywhere on the drop zone (not just the button) also
// opens the file browser. This makes the whole widget feel clickable,
// which is a common UX pattern for drop zones.
//
// We check that the click isn't on the button itself (the button
// handles its own clicks) to avoid triggering the dialog twice.
// ─────────────────────────────────────────────────────────────────────

void DropZone::mousePressEvent(QMouseEvent* event)
{
    // Only respond to left-click outside the button
    if (event->button() == Qt::LeftButton &&
        !m_browseBtn->geometry().contains(event->pos()))
    {
        openFileBrowser();
    }
    QWidget::mousePressEvent(event);
}

// ─────────────────────────────────────────────────────────────────────
// Drag and drop event handlers — unchanged from before
// ─────────────────────────────────────────────────────────────────────

void DropZone::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl& url : urls) {
            if (url.isLocalFile()) {
                QString path = url.toLocalFile();
                if (sl::ocr::ImageLoader::isSupported(
                        path.toStdString())) {
                    event->acceptProposedAction();
                    m_dragActive = true;
                    update();
                    return;
                }
            }
        }
    }
    event->ignore();
}

void DropZone::dragLeaveEvent(QDragLeaveEvent* event)
{
    m_dragActive = false;
    update();
    event->accept();
}

void DropZone::dropEvent(QDropEvent* event)
{
    m_dragActive = false;
    update();

    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }

    bool accepted = false;
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            QString path = url.toLocalFile();
            if (sl::ocr::ImageLoader::isSupported(
                    path.toStdString())) {
                emit imageDropped(path);
                accepted = true;
            }
        }
    }

    accepted ? event->acceptProposedAction() : event->ignore();
}

// ─────────────────────────────────────────────────────────────────────
// paintEvent — custom drawing for the drop zone area
//
// We draw the dashed border and instructional text in the upper
// portion of the widget. The browse button occupies the bottom.
//
// buttonArea: reserve space at bottom for the button so the
// text doesn't render on top of it.
// ─────────────────────────────────────────────────────────────────────

void DropZone::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // The "drop target" area is above the button
    int btnAreaHeight = m_browseBtn->height() + 18;
    QRect dropArea(0, 0, width(), height() - btnAreaHeight);

    // Background
    QColor bg = m_dragActive
        ? QColor(0xE3, 0xF2, 0xFF)
        : QColor(0xF8, 0xF9, 0xFA);
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1), 8, 8);

    // Dashed border
    QColor border = m_dragActive
        ? QColor(0x20, 0x88, 0xFF)
        : QColor(0xBB, 0xBB, 0xBB);
    p.setPen(QPen(border, 2, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1), 8, 8);

    // Icon — large, centered in the drop area
    p.setPen(m_dragActive
        ? QColor(0x20, 0x88, 0xFF)
        : QColor(0xAA, 0xAA, 0xAA));

    QFont iconFont = p.font();
    iconFont.setPointSize(26);
    p.setFont(iconFont);
    p.drawText(
        QRect(dropArea.x(), dropArea.y(),
              dropArea.width(), dropArea.height() * 6 / 10),
        Qt::AlignCenter,
        m_dragActive ? "⬇" : "🖼"
    );

    // Instruction text
    QFont textFont = p.font();
    textFont.setPointSize(10);
    p.setFont(textFont);
    p.setPen(m_dragActive
        ? QColor(0x20, 0x88, 0xFF)
        : QColor(0x88, 0x88, 0x88));

    p.drawText(
        QRect(dropArea.x(), dropArea.height() * 6 / 10,
              dropArea.width(), dropArea.height() * 4 / 10),
        Qt::AlignHCenter | Qt::AlignTop,
        m_dragActive
            ? "Release to process"
            : "Drop image here\nor click to browse"
    );
}

} // namespace gui
} // namespace sl
