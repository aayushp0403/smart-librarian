#include "pdf/PdfWriter.h"
#include <stdexcept>
#include <iomanip>
#include <sstream>

namespace sl {
namespace pdf {

// ─────────────────────────────────────────────────────────────────────
// Object number management
//
// Object 0 is the free-list head (PDF spec requires it).
// We pre-fill offset 0 so index alignment stays correct.
// Real objects start at index 1.
// ─────────────────────────────────────────────────────────────────────

int PdfWriter::allocateObjectNumber()
{
    int num = m_nextObjectNumber++;
    // Expand the offsets vector to accommodate this object number.
    // We'll fill in the actual offset when we write the object.
    if (static_cast<std::size_t>(num) >= m_objectOffsets.size()) {
        m_objectOffsets.resize(static_cast<std::size_t>(num) + 1, 0);
    }
    return num;
}

void PdfWriter::recordObjectOffset(int objectNum, std::size_t offset)
{
    if (static_cast<std::size_t>(objectNum) >= m_objectOffsets.size()) {
        m_objectOffsets.resize(static_cast<std::size_t>(objectNum) + 1, 0);
    }
    m_objectOffsets[static_cast<std::size_t>(objectNum)] = offset;
}

// ─────────────────────────────────────────────────────────────────────
// write() — main entry point
//
// The write sequence is dictated by the PDF specification.
// Objects must be written in a consistent order so that
// forward references can be resolved.
//
// We pre-allocate ALL object numbers before writing ANY objects.
// This way we can write object dictionaries that reference objects
// not yet written (e.g., the Pages object lists Page objects by
// number before the page objects are actually written).
// ─────────────────────────────────────────────────────────────────────

void PdfWriter::write(
    const std::string&          filepath,
    std::vector<PdfPage*>&      pages,
    const std::vector<PdfFont>& fonts
)
{
    if (pages.empty()) {
        throw std::runtime_error("PdfWriter: cannot write PDF with no pages");
    }

    // Reset state (allows PdfWriter to be reused)
    m_nextObjectNumber = 1;
    m_objectOffsets.clear();
    // Object 0 is always the free-list head — offset 0
    m_objectOffsets.push_back(0);

    // ─────────────────────────────────────
    // Phase 1: Pre-allocate all object numbers
    //
    // This must be done BEFORE writing so that the Catalog
    // can reference the Pages object, and the Pages object
    // can list all Page object numbers, even though we haven't
    // written any of them yet.
    // ─────────────────────────────────────

    int catalogNum  = allocateObjectNumber(); // 1
    int pagesNum    = allocateObjectNumber(); // 2

    // For each page: allocate page object + content stream object
    struct PageAlloc {
        int pageObjNum;
        int contentObjNum;
    };
    std::vector<PageAlloc> pageAllocs;
    pageAllocs.reserve(pages.size());
    for (std::size_t i = 0; i < pages.size(); ++i) {
        int pageNum    = allocateObjectNumber();
        int contentNum = allocateObjectNumber();
        pageAllocs.push_back({ pageNum, contentNum });
    }

    // For each font: allocate a font object number
    // We need a mutable copy so we can assign object numbers
    std::vector<PdfFont> fontsCopy = fonts;
    for (auto& font : fontsCopy) {
        font.objectNumber = allocateObjectNumber();
    }

    // ─────────────────────────────────────
    // Phase 2: Open the file and write objects
    // ─────────────────────────────────────

    PdfStream stream(filepath);

    // 1. Header — magic bytes that identify this as PDF
    writeHeader(stream);

    // 2. Catalog object
    {
        std::size_t offset = stream.position();
        recordObjectOffset(catalogNum, offset);
        stream.writeF("%d 0 obj\n", catalogNum);
        stream.writeF("<< /Type /Catalog /Pages %d 0 R >>\n", pagesNum);
        stream.writeStr("endobj\n\n");
    }

    // 3. Pages tree object
    //    Lists all page object numbers as Kids
    {
        std::size_t offset = stream.position();
        recordObjectOffset(pagesNum, offset);
        stream.writeF("%d 0 obj\n", pagesNum);
        stream.writeStr("<< /Type /Pages\n");
        stream.writeStr("   /Kids [");
        for (std::size_t i = 0; i < pageAllocs.size(); ++i) {
            stream.writeF("%d 0 R", pageAllocs[i].pageObjNum);
            if (i + 1 < pageAllocs.size()) stream.writeStr(" ");
        }
        stream.writeStr("]\n");
        stream.writeF("   /Count %zu\n", pages.size());
        stream.writeStr(">>\n");
        stream.writeStr("endobj\n\n");
    }

    // 4. For each page: content stream first, then page dictionary
    //    Why content before page?
    //    Because the page dictionary references the content stream
    //    by object number — which we know. The PDF reader uses
    //    the xref table to find objects anyway; file order
    //    doesn't matter for readers, only for us writing.
    for (std::size_t i = 0; i < pages.size(); ++i) {
        const PdfPage& page = *pages[i];
        const PageAlloc& alloc = pageAllocs[i];

        // Write content stream
        {
            std::string content = page.getContentStream();
            std::size_t offset = stream.position();
            recordObjectOffset(alloc.contentObjNum, offset);

            stream.writeF("%d 0 obj\n", alloc.contentObjNum);
            stream.writeF("<< /Length %zu >>\n", content.size());
            stream.writeStr("stream\n");
            stream.writeStr(content);
            stream.writeStr("\nendstream\n");
            stream.writeStr("endobj\n\n");
        }

        // Write page dictionary
        {
            std::size_t offset = stream.position();
            recordObjectOffset(alloc.pageObjNum, offset);

            stream.writeF("%d 0 obj\n", alloc.pageObjNum);
            stream.writeStr("<< /Type /Page\n");
            stream.writeF("   /Parent %d 0 R\n", pagesNum);
            stream.writeF("   /MediaBox [0 0 %.2f %.2f]\n",
static_cast<double>(page.width()), static_cast<double>(page.height()));           
 stream.writeF("   /Contents %d 0 R\n", alloc.contentObjNum);

            // Font resources dictionary
            if (!fontsCopy.empty()) {
                stream.writeStr("   /Resources <<\n");
                stream.writeStr("      /Font <<\n");
                for (const auto& font : fontsCopy) {
                    stream.writeF("         /%s %d 0 R\n",
                                  font.alias.c_str(),
                                  font.objectNumber);
                }
                stream.writeStr("      >>\n");
                stream.writeStr("   >>\n");
            }

            stream.writeStr(">>\n");
            stream.writeStr("endobj\n\n");
        }
    }

    // 5. Font objects
    for (const auto& font : fontsCopy) {
        std::size_t offset = stream.position();
        recordObjectOffset(font.objectNumber, offset);

        stream.writeF("%d 0 obj\n", font.objectNumber);
        stream.writeStr("<< /Type /Font\n");
        stream.writeStr("   /Subtype /Type1\n");
        stream.writeF("   /BaseFont /%s\n", font.baseName.c_str());
        stream.writeStr("   /Encoding /WinAnsiEncoding\n");
        stream.writeStr(">>\n");
        stream.writeStr("endobj\n\n");
    }

    // 6. Cross-reference table — must know position before writing
    std::size_t xrefOffset = stream.position();
    writeXrefTable(stream);

    // 7. Trailer
    writeTrailer(stream, catalogNum, xrefOffset);

    // 8. File is closed by PdfStream destructor (RAII)
}

// ─────────────────────────────────────────────────────────────────────
// writeHeader
//
// The PDF header serves two purposes:
//   1. Identifies the file as PDF and declares the spec version
//   2. The binary comment (4 bytes > 127) tells transfer tools
//      (FTP, email) that this is a binary file, not text.
//      This prevents line-ending translation that would
//      corrupt byte offsets.
// ─────────────────────────────────────────────────────────────────────

void PdfWriter::writeHeader(PdfStream& stream)
{
    stream.writeStr("%PDF-1.7\n");
    // Four bytes with values > 127: binary file marker
    // These are arbitrary high-byte values — just a convention
    const char binaryComment[] = "%\xe2\xe3\xcf\xd3\n";
    stream.writeBytes(binaryComment, sizeof(binaryComment) - 1);
}

// ─────────────────────────────────────────────────────────────────────
// writeXrefTable
//
// The xref table format is FIXED-WIDTH: each entry is EXACTLY 20 bytes.
// Format: "nnnnnnnnnn ggggg n \n" or "nnnnnnnnnn ggggg f \n"
//   n = 10-digit byte offset (zero-padded)
//   g = 5-digit generation number (zero-padded)
//   type = 'n' (in-use) or 'f' (free)
//   space + \r\n or space + space + \n (to make exactly 20 bytes)
//
// We write 2 spaces before \n to ensure exactly 20 bytes:
//   10 (offset) + 1 (space) + 5 (gen) + 1 (space) + 1 (n/f)
//   + 1 (space) + 1 (\n) = 20 bytes
// ─────────────────────────────────────────────────────────────────────

void PdfWriter::writeXrefTable(PdfStream& stream)
{
    int totalObjects = m_nextObjectNumber;  // includes object 0

    stream.writeStr("xref\n");
    stream.writeF("0 %d\n", totalObjects);

    // Object 0: free list head — always this exact entry
    stream.writeStr("0000000000 65535 f \n");

    // Objects 1..N: write their recorded byte offsets
    for (int i = 1; i < totalObjects; ++i) {
        std::size_t offset = m_objectOffsets[static_cast<std::size_t>(i)];
        // Format: 10-digit offset, space, 5-digit gen "00000", space, "n", space, newline
        // That is: OOOOOOOOOO_GGGGG_n_\n  (exactly 20 bytes)
        stream.writeF("%010zu 00000 n \n", offset);
    }
}

// ─────────────────────────────────────────────────────────────────────
// writeTrailer
//
// The trailer tells readers:
//   /Size   — total number of PDF objects
//   /Root   — which object is the Catalog (document entry point)
//
// startxref followed by the byte offset of the xref keyword
// lets readers find the xref table by seeking backward from %%EOF.
//
// PDF readers open the file, seek to the end, find %%EOF,
// read startxref + offset, seek to that offset, read xref.
// This is why the xref table is at the END of the file.
// ─────────────────────────────────────────────────────────────────────

void PdfWriter::writeTrailer(
    PdfStream& stream,
    int catalogObjectNum,
    std::size_t xrefOffset
)
{
    stream.writeStr("trailer\n");
    stream.writeF("<< /Size %d\n", m_nextObjectNumber);
    stream.writeF("   /Root %d 0 R\n", catalogObjectNum);
    stream.writeStr(">>\n");
    stream.writeStr("startxref\n");
    stream.writeF("%zu\n", xrefOffset);
    stream.writeStr("%%EOF\n");
}

} // namespace pdf
} // namespace sl
