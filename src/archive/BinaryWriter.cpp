#include "archive/BinaryWriter.h"

#include <cstring>   // memcpy

namespace sl {
namespace archive {

BinaryWriter::BinaryWriter(const std::string& filepath)
    : m_filepath(filepath)
{
    // ios::binary — critical: prevents CR/LF translation on Windows.
    // ios::trunc  — overwrite any existing file.
    m_file.open(filepath,
        std::ios::binary | std::ios::out | std::ios::trunc);

    if (!m_file.is_open()) {
        throw std::runtime_error(
            "BinaryWriter: cannot open '" + filepath + "' for writing");
    }
}

BinaryWriter::~BinaryWriter()
{
    if (m_file.is_open()) {
        m_file.flush();
        m_file.close();
    }
}

void BinaryWriter::checkOpen() const
{
    if (!m_file.is_open() || !m_file.good()) {
        throw std::runtime_error(
            "BinaryWriter: file '" + m_filepath + "' is not open or in error state");
    }
}

// ─────────────────────────────────────────────────────────────────────
// writeU32
//
// We write the integer byte-by-byte in explicit little-endian order.
// This is portable: it produces the same byte sequence regardless of
// the host machine's native endianness.
//
// Alternative: just write the raw bytes with m_file.write(&value, 4).
// That would be native-endian — fine on x64 but not portable.
// For a portfolio project on x64 Linux, either works.
// We choose explicit little-endian to demonstrate the concept.
// ─────────────────────────────────────────────────────────────────────

void BinaryWriter::writeU32(uint32_t value)
{
    checkOpen();

    uint8_t bytes[4] = {
        static_cast<uint8_t>( value        & 0xFF),  // least significant
        static_cast<uint8_t>((value >>  8) & 0xFF),
        static_cast<uint8_t>((value >> 16) & 0xFF),
        static_cast<uint8_t>((value >> 24) & 0xFF)   // most significant
    };

    m_file.write(reinterpret_cast<const char*>(bytes), 4);

    if (!m_file.good()) {
        throw std::runtime_error(
            "BinaryWriter: write failed at offset " +
            std::to_string(m_position));
    }

    m_position += 4;
}

void BinaryWriter::writeU64(uint64_t value)
{
    checkOpen();

    uint8_t bytes[8];
    for (int i = 0; i < 8; ++i) {
        bytes[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
    }

    m_file.write(reinterpret_cast<const char*>(bytes), 8);

    if (!m_file.good()) {
        throw std::runtime_error("BinaryWriter: writeU64 failed");
    }

    m_position += 8;
}

void BinaryWriter::writeU8(uint8_t value)
{
    checkOpen();
    m_file.write(reinterpret_cast<const char*>(&value), 1);
    if (!m_file.good()) {
        throw std::runtime_error("BinaryWriter: writeU8 failed");
    }
    ++m_position;
}

// ─────────────────────────────────────────────────────────────────────
// writeF32
//
// IEEE 754 single-precision float is exactly 4 bytes.
// We use memcpy to reinterpret the float's bit pattern as a uint32_t,
// then write it with writeU32.
//
// Why memcpy and not reinterpret_cast<uint32_t*>(&value)?
//   Accessing a float through a uint32_t pointer violates strict
//   aliasing rules — the compiler may assume two pointers of
//   different types don't alias, allowing it to reorder or
//   optimize away reads/writes. memcpy is the standard-compliant
//   way to do type-punning in C++.
// ─────────────────────────────────────────────────────────────────────

void BinaryWriter::writeF32(float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    writeU32(bits);
}

// ─────────────────────────────────────────────────────────────────────
// writeString — length-prefixed UTF-8 string
// ─────────────────────────────────────────────────────────────────────

void BinaryWriter::writeString(std::string_view str)
{
    checkOpen();

    // Guard against extremely long strings
    if (str.size() > 0xFFFF'FFFF) {
        throw std::runtime_error("BinaryWriter: string too long to serialize");
    }

    // Write length prefix
    writeU32(static_cast<uint32_t>(str.size()));

    // Write string bytes (no null terminator)
    if (!str.empty()) {
        m_file.write(str.data(), static_cast<std::streamsize>(str.size()));
        if (!m_file.good()) {
            throw std::runtime_error("BinaryWriter: string data write failed");
        }
        m_position += str.size();
    }
}

void BinaryWriter::writeRawBytes(const char* data, std::size_t length)
{
    checkOpen();
    m_file.write(data, static_cast<std::streamsize>(length));
    if (!m_file.good()) {
        throw std::runtime_error("BinaryWriter: raw byte write failed");
    }
    m_position += length;
}

std::size_t BinaryWriter::position() const { return m_position; }

void BinaryWriter::flush()
{
    if (m_file.is_open()) m_file.flush();
}

void BinaryWriter::close()
{
    if (m_file.is_open()) {
        m_file.flush();
        m_file.close();
    }
}

bool BinaryWriter::isOpen() const { return m_file.is_open(); }

} // namespace archive
} // namespace sl
