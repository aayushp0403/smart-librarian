#pragma once

#include <fstream>
#include <string>
#include <cstdint>
#include <cstddef>
#include <stdexcept>

namespace sl {
namespace archive {

/**
 * BinaryReader
 *
 * Mirror of BinaryWriter — reads the same format back.
 *
 * Every read operation must exactly mirror its corresponding
 * write operation or the data will be misinterpreted.
 * This is why documenting the binary format precisely matters —
 * a one-byte misalignment corrupts every subsequent read.
 *
 * Error handling strategy:
 *   Throws std::runtime_error on:
 *   - EOF reached unexpectedly (file truncated or wrong format)
 *   - File not open
 *   - Read returning fewer bytes than expected
 *
 *   The caller catches these at the PersistenceManager level
 *   and treats them as "cannot load — start fresh".
 */
class BinaryReader
{
public:
    explicit BinaryReader(const std::string& filepath);
    ~BinaryReader();

    BinaryReader(const BinaryReader&)            = delete;
    BinaryReader& operator=(const BinaryReader&) = delete;

    uint32_t    readU32();
    uint64_t    readU64();
    uint8_t     readU8();
    float       readF32();
    std::string readString();

    /**
     * readRawBytes — read exactly 'length' bytes into 'buffer'.
     * buffer must be pre-allocated to hold at least 'length' bytes.
     */
    void readRawBytes(char* buffer, std::size_t length);

    std::size_t position() const;
    bool        isOpen()   const;

    /**
     * isAtEnd — returns true if the read position is at or past EOF.
     */
    bool isAtEnd() const;

private:
    std::ifstream m_file;
    std::size_t   m_position { 0 };
    std::string   m_filepath;

    void checkOpen() const;
    void checkRead(std::streamsize expected, std::streamsize actual) const;
};

} // namespace archive
} // namespace sl
