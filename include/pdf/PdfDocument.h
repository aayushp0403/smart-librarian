#pragma once

#include "pdf/PdfPage.h"
#include "pdf/PdfFont.h"
#include "pdf/PdfWriter.h"

#include <string>
#include <vector>
#include <memory>

namespace sl {
namespace pdf {

/**
 * PdfDocument
 *
 * Top-level document API.
 *
 * Usage:
 *
 *   PdfDocument doc;
 *   doc.addFont("F1", "Helvetica");
 *
 *   PdfPage& page = doc.addPage();
 *   page.beginText();
 *   page.setFont("F1", 14.0f);
 *   page.moveTo(72, 700);
 *   page.showText("Hello, World!");
 *   page.endText();
 *
 *   doc.save("output.pdf");
 *
 * Ownership:
 *   PdfDocument owns all PdfPage objects via unique_ptr.
 *   When the document is destroyed, all pages are destroyed.
 *   RAII — no manual cleanup needed.
 *
 * Why unique_ptr<PdfPage>?
 *   Pages are heap-allocated because:
 *   1. Their size isn't known at compile time (content varies)
 *   2. They need to survive function scope (addPage returns a ref)
 *   3. We need stable addresses — a vector<PdfPage> would
 *      invalidate references on reallocation
 *
 *   unique_ptr gives us:
 *   - Automatic deletion when the document is destroyed
 *   - Clear ownership semantics (document = sole owner)
 *   - Raw pointer access via .get() for the writer interface
 */
class PdfDocument
{
public:
    PdfDocument();
    ~PdfDocument() = default;

    // Non-copyable — document has unique ownership of pages
    PdfDocument(const PdfDocument&)            = delete;
    PdfDocument& operator=(const PdfDocument&) = delete;

    // Movable
    PdfDocument(PdfDocument&&)            = default;
    PdfDocument& operator=(PdfDocument&&) = default;

    /**
     * addFont — register a font for use in this document.
     *
     * @param alias     short name used in content streams: "F1", "F2"
     * @param baseName  PDF built-in font name: "Helvetica", "Times-Roman"
     */
    void addFont(const std::string& alias, const std::string& baseName);

    /**
     * addPage — create a new page and return a reference to it.
     *
     * The document owns the page. The reference is valid for
     * the lifetime of the document.
     *
     * @return reference to the newly created PdfPage
     */
    PdfPage& addPage();

    /**
     * save — serialize the document to a PDF file.
     *
     * @param filepath  output path (e.g., "output.pdf")
     */
    void save(const std::string& filepath);

    /**
     * pageCount — number of pages currently in the document.
     */
    std::size_t pageCount() const;

private:
    std::vector<std::unique_ptr<PdfPage>> m_pages;
    std::vector<PdfFont>                  m_fonts;
    PdfWriter                             m_writer;
};

} // namespace pdf
} // namespace sl
