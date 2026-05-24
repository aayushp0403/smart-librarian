#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace sl {
namespace pdf {

/**
 * PdfPage
 *
 * Represents a single page in the PDF document.
 *
 * Accumulates drawing commands into a content stream buffer.
 * The content stream is serialized when the page is written
 * to disk by PdfWriter.
 *
 * Coordinate system reminder:
 *   (0, 0) = bottom-left of page
 *   Y increases upward
 *   US Letter = 612 x 792 points
 *
 * The content stream is built in m_contentStream using
 * std::ostringstream — an in-memory string buffer.
 * When we're done adding content, we get the string,
 * measure its length, and write it as a PDF stream object.
 *
 * Why build in memory first?
 *   The PDF stream object requires /Length in its dictionary,
 *   but we only know the length after all content is added.
 *   Building in memory lets us measure before writing.
 */
class PdfPage
{
public:
    // Standard US Letter: 612 x 792 points
    static constexpr float PAGE_WIDTH  = 612.0f;
    static constexpr float PAGE_HEIGHT = 792.0f;

    // Default margins in points
    static constexpr float MARGIN_LEFT  = 72.0f;   // 1 inch
    static constexpr float MARGIN_RIGHT = 72.0f;
    static constexpr float MARGIN_TOP   = 72.0f;
    static constexpr float MARGIN_BOTTOM= 72.0f;

    PdfPage();
    ~PdfPage() = default;

    // Non-copyable (content stream is unique per page)
    PdfPage(const PdfPage&)            = delete;
    PdfPage& operator=(const PdfPage&) = delete;

    // Movable — pages can be moved into the document's vector
    PdfPage(PdfPage&&)            = default;
    PdfPage& operator=(PdfPage&&) = default;

    // ─────────────────────────────────────
    // Text Operations
    // ─────────────────────────────────────

    /**
     * beginText / endText
     * Wrap all text operations in BT...ET block.
     * PDF requires this.
     */
    void beginText();
    void endText();

    /**
     * setFont — select a named font at given point size.
     * @param fontAlias  the /F1-style alias
     * @param size       point size (e.g., 12.0f)
     */
    void setFont(const std::string& fontAlias, float size);

    /**
     * moveTo — move text cursor to absolute page coordinates.
     * @param x  points from left
     * @param y  points from bottom
     */
    void moveTo(float x, float y);

    /**
     * moveDown — move text cursor down by 'points' from current position.
     * Negative Td Y value in PDF coordinates.
     */
    void moveDown(float points);

    /**
     * showText — render a string at the current cursor position.
     * Handles parenthesis escaping required by PDF string syntax.
     */
    void showText(const std::string& text);

    /**
     * setLeading — set line spacing in points.
     * Called with TL operator. Used with T* (next line).
     */
    void setLeading(float points);

    /**
     * nextLine — advance to next line using current leading.
     * Uses T* operator.
     */
    void nextLine();

    /**
     * setTextColor — set fill color for text (RGB, 0.0–1.0 range).
     */
    void setTextColor(float r, float g, float b);

    // ─────────────────────────────────────
    // Graphics State Operations
    // ─────────────────────────────────────

    /**
     * drawLine — stroke a line from (x1,y1) to (x2,y2).
     */
    void drawLine(float x1, float y1, float x2, float y2, float lineWidth = 1.0f);

    /**
     * drawRect — stroke a rectangle.
     */
    void drawRect(float x, float y, float width, float height, float lineWidth = 1.0f);

    /**
     * fillRect — fill a rectangle with current fill color.
     */
    void fillRect(float x, float y, float width, float height);

    /**
     * setFillColor — RGB fill color.
     */
    void setFillColor(float r, float g, float b);

    /**
     * setStrokeColor — RGB stroke color.
     */
    void setStrokeColor(float r, float g, float b);

    // ─────────────────────────────────────
    // Accessors
    // ─────────────────────────────────────

    /**
     * getContentStream — returns the accumulated content stream
     * as a string. Called by PdfWriter when serializing.
     */
    std::string getContentStream() const;

    float width()  const { return PAGE_WIDTH; }
    float height() const { return PAGE_HEIGHT; }

private:
    std::ostringstream m_content;

    /**
     * escapePdfString — PDF string syntax requires escaping
     * parentheses and backslashes inside (string) literals.
     */
    std::string escapePdfString(const std::string& input) const;
};

} // namespace pdf
} // namespace sl
