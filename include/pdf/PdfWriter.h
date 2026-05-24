#pragma once

#include "pdf/PdfStream.h"
#include "pdf/PdfPage.h"
#include "pdf/PdfFont.h"

#include <vector>
#include <string>
#include <cstddef>

namespace sl {
namespace pdf {

/**
 * PdfWriter
 *
 * Serializes a complete PDF document to disk.
 *
 * Internal state:
 *   m_objectOffsets — records the byte offset of each PDF object.
 *     Index = object number. Value = byte offset from file start.
 *     This IS the data for the xref table.
 *
 *   m_nextObjectNumber — auto-incremented as we allocate objects.
 *     Object 0 is always the free-list head (PDF spec).
 *     We start real objects at 1.
 *
 * Write sequence (this is fixed by the PDF spec):
 *   1. Header (%PDF-1.7)
 *   2. Catalog object
 *   3. Pages tree object
 *   4. For each page: page dictionary + content stream + font objects
 *   5. Cross-reference table (xref)
 *   6. Trailer dictionary
 *   7. startxref + byte offset of xref
 *   8. %%EOF
 */
class PdfWriter
{
public:
    /**
     * write() — main entry point.
     *
     * @param filepath  output .pdf path
     * @param pages     page content objects (moved in — writer owns them)
     * @param fonts     font descriptors to embed as resources
     */
    void write(
        const std::string&          filepath,
        std::vector<PdfPage*>&      pages,
        const std::vector<PdfFont>& fonts
    );

private:
    // ─────────────────────────────────────
    // Object number management
    // ─────────────────────────────────────

    int  allocateObjectNumber();
    void recordObjectOffset(int objectNum, std::size_t offset);

    // ─────────────────────────────────────
    // Write stages
    // ─────────────────────────────────────

    void writeHeader(PdfStream& stream);

    int writeCatalog(PdfStream& stream, int pagesObjectNum);

    int writePageTree(PdfStream& stream,
                      const std::vector<int>& pageObjectNums);

    int writePageObject(PdfStream& stream,
                        const PdfPage& page,
                        int pagesObjectNum,
                        int contentObjectNum,
                        const std::vector<PdfFont>& fonts);

    int writeContentStream(PdfStream& stream,
                           const PdfPage& page);

    int writeFontObject(PdfStream& stream,
                        const PdfFont& font);

    void writeXrefTable(PdfStream& stream);

    void writeTrailer(PdfStream& stream,
                      int catalogObjectNum,
                      std::size_t xrefOffset);

    // ─────────────────────────────────────
    // State
    // ─────────────────────────────────────

    int                      m_nextObjectNumber { 1 };
    std::vector<std::size_t> m_objectOffsets;   // index = object number
};

} // namespace pdf
} // namespace sl
