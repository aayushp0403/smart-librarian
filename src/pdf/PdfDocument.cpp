#include "pdf/PdfDocument.h"
#include <stdexcept>

namespace sl {
namespace pdf {

PdfDocument::PdfDocument()
{
    // Reserve space for typical document sizes.
    // This avoids vector reallocation for documents
    // with up to 16 pages or 8 fonts.
    m_pages.reserve(16);
    m_fonts.reserve(8);
}

void PdfDocument::addFont(
    const std::string& alias,
    const std::string& baseName
)
{
    // Check for duplicate alias
    for (const auto& font : m_fonts) {
        if (font.alias == alias) {
            throw std::invalid_argument(
                "PdfDocument: font alias '" + alias + "' already registered"
            );
        }
    }
    m_fonts.emplace_back(alias, baseName);
}

PdfPage& PdfDocument::addPage()
{
    // Construct a new PdfPage on the heap, wrap in unique_ptr.
    // Push to vector. Return reference to the managed page.
    //
    // Why return a reference and not a pointer?
    //   Reference makes caller code cleaner and communicates
    //   that the page is always valid (never null).
    //   The caller must not outlive the document.
    m_pages.push_back(std::make_unique<PdfPage>());
    return *m_pages.back();
}

void PdfDocument::save(const std::string& filepath)
{
    if (m_pages.empty()) {
        throw std::runtime_error("PdfDocument::save: document has no pages");
    }

    if (m_fonts.empty()) {
        throw std::runtime_error(
            "PdfDocument::save: no fonts registered. "
            "Call addFont() before adding text."
        );
    }

    // Build a vector of raw page pointers for PdfWriter.
    // PdfWriter does not own these pointers.
    // PdfDocument continues to own them via unique_ptr.
    std::vector<PdfPage*> rawPages;
    rawPages.reserve(m_pages.size());
    for (const auto& page : m_pages) {
        rawPages.push_back(page.get());
    }

    m_writer.write(filepath, rawPages, m_fonts);
}

std::size_t PdfDocument::pageCount() const
{
    return m_pages.size();
}

} // namespace pdf
} // namespace sl
