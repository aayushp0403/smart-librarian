#include "archive/BinaryReader.h"
#include <cstring>

namespace sl {
namespace archive {

BinaryReader::BinaryReader(const std::string& filepath)
    : m_filepath(filepath)
{
    m_file.open(filepath, std::ios::binary | std::ios::in);

    if (!m_file.is_open()) {
        throw std::runtime_error(
            "BinaryReader: cannot open '" + filepath + "'");
    }
}

BinaryReader::~BinaryReader()
{
    if (m_file.is_open()) {
        m_file.close();
    }
}

void BinaryReader::checkOpen() const
{
    if (!m_file.is_open()) {
        throw std::runtime_error(
            "BinaryReader: file '" + m_filepath + "' is not open");
    }
}

void BinaryReader::checkRead(
    std::streamsize expected,
    std::streamsize actual) const
{
    if (actual != expected) {
        throw std::runtime_error(
            "BinaryReader: expected " + std::to_string(expected) +
            " bytes but read " + std::to_string(actual) +
            " at offset " + std::to_string(m_position) +
            " in '" + m_filepath + "' (file truncated or wrong format?)");
    }
}

uint32_t BinaryReader::readU32()
{
    checkOpen();

    uint8_t bytes[4];
    m_file.read(reinterpret_cast<char*>(bytes), 4);
    checkRead(4, m_file.gcount());
    m_position += 4;

    // Reconstruct from little-endian bytes
    return static_cast<uint32_t>(bytes[0])
         | static_cast<uint32_t>(bytes[1]) <<  8
         | static_cast<uint32_t>(bytes[2]) << 16
         | static_cast<uint32_t>(bytes[3]) << 24;
}

uint64_t BinaryReader::readU64()
{
    checkOpen();

    uint8_t bytes[8];
    m_file.read(reinterpret_cast<char*>(bytes), 8);
    checkRead(8, m_file.gcount());
    m_position += 8;

    uint64_t result = 0;
    for (int i = 0; i < 8; ++i) {
        result |= static_cast<uint64_t>(bytes[i]) << (i * 8);
    }
    return result;
}

uint8_t BinaryReader::readU8()
{
    checkOpen();
    uint8_t value = 0;
    m_file.read(reinterpret_cast<char*>(&value), 1);
    checkRead(1, m_file.gcount());
    ++m_position;
    return value;
}

float BinaryReader::readF32()
{
    uint32_t bits = readU32();
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

std::string BinaryReader::readString()
{
    checkOpen();

    uint32_t length = readU32();

    // Sanity check: reject absurdly long strings
    // (protects against corrupt files)
    if (length > 10 * 1024 * 1024) {  // 10 MB max string
        throw std::runtime_error(
            "BinaryReader: string length " + std::to_string(length) +
            " is suspiciously large — file may be corrupt");
    }

    if (length == 0) return {};

    std::string result(length, '\0');
    m_file.read(result.data(), static_cast<std::streamsize>(length));
    checkRead(static_cast<std::streamsize>(length), m_file.gcount());
    m_position += length;

    return result;
}

void BinaryReader::readRawBytes(char* buffer, std::size_t length)
{
    checkOpen();
    m_file.read(buffer, static_cast<std::streamsize>(length));
    checkRead(static_cast<std::streamsize>(length), m_file.gcount());
    m_position += length;
}

std::size_t BinaryReader::position() const { return m_position; }
bool        BinaryReader::isOpen()   const { return m_file.is_open(); }
bool        BinaryReader::isAtEnd()  const
{
    return m_file.eof() || !m_file.good();
}

} // namespace archive
} // namespace sl
