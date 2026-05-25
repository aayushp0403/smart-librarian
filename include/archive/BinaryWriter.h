#pragma once

#include <fstream>
#include <string>
#include <string_view>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <stdexcept>

namespace sl {
namespace archive {

/**
 * BinaryWriter
 *
 * Writes primitive types to a binary file in a consistent,
 * platform-aware format.
 *
 * Endianness:
 *   Different CPU architectures store multi-byte integers
 *   differently. x86/x64 (your laptop) is little-endian:
 *   the least significant byte comes first in memory.
 *   ARM (phones, M-series Macs) is also little-endian in
 *   practice. Big-endian (network byte order) is used by
 *   some older architectures and network protocols.
 *
 *   We write in little-endian format and document this in
 *   the file header. A reader on a big-endian machine would
 *   need to byte-swap. For our use case (WSL2/Linux x64),
 *   this is not an issue — we note it in the format spec.
 *
 * Why not use std::ofstream directly?
 *   ofstream::write() takes char* and a count. Every call site
 *   would need to cast, check errors, and track position.
 *   BinaryWriter centralizes all of that behind a clean API.
 *
 * Error handling:
 *   All write failures throw std::runtime_error immediately.
 *   We use exceptions rather than return codes because a write
 *   failure in the middle of a complex struct would leave the
 *   file in a partial state — it's better to surface the error
 *   loudly and let the caller decide how to recover.
 */
class BinaryWriter
{
public:
    explicit BinaryWriter(const std::string& filepath);
    ~BinaryWriter();

    BinaryWriter(const BinaryWriter&)            = delete;
    BinaryWriter& operator=(const BinaryWriter&) = delete;

    // Write a fixed-size unsigned integer (4 bytes, little-endian)
    void writeU32(uint32_t value);

    // Write a fixed-size unsigned integer (8 bytes, little-endian)
    void writeU64(uint64_t value);

    // Write a single byte
    void writeU8(uint8_t value);

    // Write a 32-bit float (IEEE 754, 4 bytes)
    void writeF32(float value);

    /**
     * writeString — write a length-prefixed string.
     *
     * Format: [uint32_t length][char* data]
     *
     * Why length-prefix instead of null-termination?
     *   Null-terminated strings require scanning to find the end
     *   (O(n)). Length-prefixed strings tell you the size upfront
     *   so you can allocate exactly the right buffer and read
     *   exactly the right number of bytes (O(1) setup, O(n) copy).
     *   This is how most binary protocols work (e.g., protobuf,
     *   MessagePack, our PDF engine's stream objects).
     */
    void writeString(std::string_view str);

    /**
     * writeRawBytes — write arbitrary bytes.
     * Used for magic numbers and padding.
     */
    void writeRawBytes(const char* data, std::size_t length);

    /**
     * position — current byte offset from start of file.
     * Used for format verification and debugging.
     */
    std::size_t position() const;

    void flush();
    void close();
    bool isOpen() const;

private:
    std::ofstream m_file;
    std::size_t   m_position { 0 };
    std::string   m_filepath;

    void checkOpen() const;
};

} // namespace archive
} // namespace sl
