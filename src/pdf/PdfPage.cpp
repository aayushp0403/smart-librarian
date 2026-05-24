#include "pdf/PdfPage.h"
#include <iomanip>
#include <sstream>

namespace sl {
namespace pdf {

PdfPage::PdfPage()
{
    // Nothing to initialize — m_content (ostringstream) is
    // default-constructed as an empty buffer.
}

// ─────────────────────────────────────────────────────────────
// Text operations
// ─────────────────────────────────────────────────────────────

void PdfPage::beginText()
{
    m_content << "BT\n";
}

void PdfPage::endText()
{
    m_content << "ET\n";
}

void PdfPage::setFont(const std::string& fontAlias, float size)
{
    // PDF operator: /F1 12 Tf
    // '/' prefix is part of the Name object type in PDF
    m_content << "/" << fontAlias << " " << size << " Tf\n";
}

void PdfPage::moveTo(float x, float y)
{
    // Td operator: move to (x, y) relative to current text matrix.
    // We use it as absolute positioning by starting from origin.
    //
    // Precision: 2 decimal places is sufficient for points.
    m_content << std::fixed << std::setprecision(2)
              << x << " " << y << " Td\n";
}

void PdfPage::moveDown(float points)
{
    // Move cursor: x stays same (0), y decreases (negative = down)
    m_content << std::fixed << std::setprecision(2)
              << "0 " << -points << " Td\n";
}

void PdfPage::showText(const std::string& text)
{
    // PDF string literal syntax: (content) Tj
    // Parentheses must be balanced or escaped.
    m_content << "(" << escapePdfString(text) << ") Tj\n";
}

void PdfPage::setLeading(float points)
{
    // TL operator: sets the text leading (line spacing)
    m_content << std::fixed << std::setprecision(2)
              << points << " TL\n";
}

void PdfPage::nextLine()
{
    // T* operator: move to start of next line
    // (uses the current leading value set by TL)
    m_content << "T*\n";
}

void PdfPage::setTextColor(float r, float g, float b)
{
    // rg operator: set fill color (used for text rendering)
    m_content << std::fixed << std::setprecision(3)
              << r << " " << g << " " << b << " rg\n";
}

// ─────────────────────────────────────────────────────────────
// Graphics operations
// ─────────────────────────────────────────────────────────────

void PdfPage::drawLine(float x1, float y1, float x2, float y2, float lineWidth)
{
    // w = line width, m = moveto, l = lineto, S = stroke
    m_content << std::fixed << std::setprecision(2)
              << lineWidth << " w\n"
              << x1 << " " << y1 << " m\n"
              << x2 << " " << y2 << " l\n"
              << "S\n";
}

void PdfPage::drawRect(float x, float y, float width, float height, float lineWidth)
{
    // re = rectangle, S = stroke
    m_content << std::fixed << std::setprecision(2)
              << lineWidth << " w\n"
              << x << " " << y << " "
              << width << " " << height << " re\n"
              << "S\n";
}

void PdfPage::fillRect(float x, float y, float width, float height)
{
    // re = rectangle, f = fill
    m_content << std::fixed << std::setprecision(2)
              << x << " " << y << " "
              << width << " " << height << " re\n"
              << "f\n";
}

void PdfPage::setFillColor(float r, float g, float b)
{
    // rg operator (lowercase) = fill color
    m_content << std::fixed << std::setprecision(3)
              << r << " " << g << " " << b << " rg\n";
}

void PdfPage::setStrokeColor(float r, float g, float b)
{
    // RG operator (uppercase) = stroke color
    m_content << std::fixed << std::setprecision(3)
              << r << " " << g << " " << b << " RG\n";
}

// ─────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────

std::string PdfPage::getContentStream() const
{
    return m_content.str();
}

// ─────────────────────────────────────────────────────────────
// escapePdfString
//
// PDF string literals use ( ) as delimiters.
// Inside a string, '(', ')', and '\' must be escaped with '\'.
// Otherwise the PDF parser gets confused about where the string ends.
//
// Example: "Hello (world)" → "Hello \(world\)"
// ─────────────────────────────────────────────────────────────

std::string PdfPage::escapePdfString(const std::string& input) const
{
    std::string result;
    result.reserve(input.size());  // pre-allocate — avoids repeated realloc

    for (char c : input) {
        switch (c) {
            case '(':  result += "\\("; break;
            case ')':  result += "\\)"; break;
            case '\\': result += "\\\\"; break;
            default:   result += c;    break;
        }
    }

    return result;
}

} // namespace pdf
} // namespace sl
