#pragma once

#include <string>
#include <cstdint>

namespace sl {
namespace pdf {

/**
 * PdfFont
 *
 * Represents a font resource in the PDF.
 *
 * We use PDF Type1 built-in fonts for Phase 1.
 * These 14 fonts are guaranteed to be available
 * in every PDF reader without embedding:
 *
 *   Helvetica, Helvetica-Bold, Helvetica-Oblique,
 *   Helvetica-BoldOblique, Times-Roman, Times-Bold,
 *   Times-Italic, Times-BoldItalic, Courier, Courier-Bold,
 *   Courier-Oblique, Courier-BoldOblique, Symbol, ZapfDingbats
 *
 * m_alias is the name used in content streams: "/F1", "/F2", etc.
 * m_baseName is the PDF font name: "/Helvetica", etc.
 * m_objectNumber is assigned when the font is added to a document.
 */
struct PdfFont
{
    std::string  alias;         // e.g., "F1"
    std::string  baseName;      // e.g., "Helvetica"
    int          objectNumber;  // assigned during PDF construction

    PdfFont(std::string alias, std::string baseName)
        : alias(std::move(alias))
        , baseName(std::move(baseName))
        , objectNumber(-1)
    {}
};

} // namespace pdf
} // namespace sl