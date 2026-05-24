#pragma once

#include <string>
#include <string_view>
#include <fstream>
#include <cstddef>   // std::size_t
#include <cstdint>   // uint8_t

namespace sl {
namespace pdf {

/**
 * PdfStream
 *
 * Wraps a file output stream and provides:
 *   1. Byte-level writing primitives
 *   2. Current byte position tracking (crucial for xref table)
 *   3. Formatted write support (printf-style for PDF syntax)
 *
 * Why track position manually?
 *   std::ofstream::tellp() gives the current write position,
 *   but it has platform-specific behavior on text-mode streams.
 *   We open in binary mode (ios::binary) and track manually
 *   for exactness. Every byte we write increments m_position.
 *
 * Memory model:
 *   This class wraps a file descriptor (via ofstream).
 *   The OS maintains a kernel-level buffer.
 *   ofstream has a userspace buffer too (streambuf).
 *   Data is: our code → streambuf → kernel buffer → disk.
 *   flush() / close() forces kernel buffer → disk.
 *
 * Owns the file handle via RAII.
 */
class PdfStream
{
public:
    explicit PdfStream(const std::string& filepath);
    ~PdfStream();

    // Non-copyable (owns a file handle)
    PdfStream(const PdfStream&)            = delete;
    PdfStream& operator=(const PdfStream&) = delete;

    // Movable
    PdfStream(PdfStream&&)            = default;
    PdfStream& operator=(PdfStream&&) = default;

    /**
     * Write raw bytes.
     * @param data pointer to byte buffer
     * @param length number of bytes to write
     */
    void writeBytes(const char* data, std::size_t length);

    /**
     * Write a string (no null terminator written).
     */
    void writeStr(std::string_view str);

    /**
     * Write a formatted string — printf-style.
     * Used for writing PDF syntax like object headers.
     *
     * Returns *this for chaining.
     */
    PdfStream& writeF(const char* format, ...);

    /**
     * Write a single newline character.
     */
    void newline();

    /**
     * Returns current byte offset from the start of the file.
     * This IS the xref entry for the next object we write.
     */
    std::size_t position() const;

    /**
     * Flush userspace buffer to kernel.
     * Call before recording xref offsets if needed.
     */
    void flush();

    /**
     * Close the file. Called automatically by destructor.
     */
    void close();

    /**
     * Returns true if the stream is open and healthy.
     */
    bool isOpen() const;

private:
    std::ofstream  m_file;
    std::size_t    m_position { 0 };
    std::string    m_filepath;
};

} // namespace pdf
} // namespace sl