#include "pdf/PdfStream.h"

#include <stdexcept>
#include <cstdarg>    // va_list, va_start, va_end
#include <cstdio>     // vsnprintf
#include <vector>

namespace sl {
namespace pdf {

// ─────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────

PdfStream::PdfStream(const std::string& filepath)
    : m_filepath(filepath)
    , m_position(0)
{
    // ios::binary is CRITICAL.
    //
    // On Windows, text mode translates '\n' to '\r\n'.
    // PDF byte offsets in the xref table must be exact.
    // A single extra '\r' byte would corrupt every offset.
    // Binary mode writes bytes exactly as we specify them.
    m_file.open(filepath, std::ios::binary | std::ios::out | std::ios::trunc);

    if (!m_file.is_open()) {
        throw std::runtime_error(
            "PdfStream: failed to open file: " + filepath
        );
    }
}

// ─────────────────────────────────────────────────────────
// Destructor — RAII: file closed automatically
// ─────────────────────────────────────────────────────────

PdfStream::~PdfStream()
{
    if (m_file.is_open()) {
        m_file.flush();
        m_file.close();
    }
}

// ─────────────────────────────────────────────────────────
// writeBytes — core primitive
// ─────────────────────────────────────────────────────────

void PdfStream::writeBytes(const char* data, std::size_t length)
{
    if (!data || length == 0) return;

    m_file.write(data, static_cast<std::streamsize>(length));

    if (!m_file.good()) {
        throw std::runtime_error(
            "PdfStream: write failed on file: " + m_filepath
        );
    }

    // Track position manually.
    // Every byte written advances our offset counter.
    // This counter becomes the xref table entry for each object.
    m_position += length;
}

// ─────────────────────────────────────────────────────────
// writeStr
// ─────────────────────────────────────────────────────────

void PdfStream::writeStr(std::string_view str)
{
    writeBytes(str.data(), str.size());
}

// ─────────────────────────────────────────────────────────
// writeF — formatted write
//
// We use a stack buffer first (512 bytes covers most PDF
// syntax). If the format expands beyond that, we allocate
// a heap vector sized to fit.
//
// This is a common C++ pattern: stack-first, heap fallback.
// It avoids heap allocation for the common case.
// ─────────────────────────────────────────────────────────

PdfStream& PdfStream::writeF(const char* format, ...)
{
    // First attempt: stack buffer
    char stackBuf[512];

    va_list args;
    va_start(args, format);
    int needed = vsnprintf(stackBuf, sizeof(stackBuf), format, args);
    va_end(args);

    if (needed < 0) {
        throw std::runtime_error("PdfStream::writeF: formatting error");
    }

    if (static_cast<std::size_t>(needed) < sizeof(stackBuf)) {
        // Fits in stack buffer — zero heap allocation
        writeBytes(stackBuf, static_cast<std::size_t>(needed));
    } else {
        // Doesn't fit — allocate exact size on heap
        std::vector<char> heapBuf(static_cast<std::size_t>(needed) + 1);

        va_list args2;
        va_start(args2, format);
        vsnprintf(heapBuf.data(), heapBuf.size(), format, args2);
        va_end(args2);

        writeBytes(heapBuf.data(), static_cast<std::size_t>(needed));
    }

    return *this;
}

// ─────────────────────────────────────────────────────────
// newline
// ─────────────────────────────────────────────────────────

void PdfStream::newline()
{
    // PDF spec recommends '\n' as line separator.
    // We are in binary mode so '\n' writes as 0x0A, exactly one byte.
    writeBytes("\n", 1);
}

// ─────────────────────────────────────────────────────────
// position
// ─────────────────────────────────────────────────────────

std::size_t PdfStream::position() const
{
    return m_position;
}

// ─────────────────────────────────────────────────────────
// flush
// ─────────────────────────────────────────────────────────

void PdfStream::flush()
{
    m_file.flush();
}

// ─────────────────────────────────────────────────────────
// close
// ─────────────────────────────────────────────────────────

void PdfStream::close()
{
    if (m_file.is_open()) {
        m_file.flush();
        m_file.close();
    }
}

// ─────────────────────────────────────────────────────────
// isOpen
// ─────────────────────────────────────────────────────────

bool PdfStream::isOpen() const
{
    return m_file.is_open();
}

} // namespace pdf
} // namespace sl