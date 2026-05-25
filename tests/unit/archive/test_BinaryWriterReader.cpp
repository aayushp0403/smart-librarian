#include <gtest/gtest.h>
#include "archive/BinaryWriter.h"
#include "archive/BinaryReader.h"
#include <cstring>
#include <filesystem>
#include <cmath>

using namespace sl::archive;

class BinaryRoundTripTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_path = "/tmp/sl_test_binary_" +
            std::to_string(reinterpret_cast<std::size_t>(this)) +
            ".bin";
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove(m_path, ec);
    }

    std::string m_path;
};

// ─────────────────────────────────────────────────────────────────────
// Round-trip tests: write then read, verify identical values
// These are the most important tests for a serializer.
// ─────────────────────────────────────────────────────────────────────

TEST_F(BinaryRoundTripTest, U32RoundTrip)
{
    const uint32_t values[] = {
        0u, 1u, 127u, 128u, 255u,
        65535u, 65536u,
        0xDEADBEEFu,
        std::numeric_limits<uint32_t>::max()
    };

    for (uint32_t v : values) {
        { BinaryWriter w(m_path); w.writeU32(v); }

        BinaryReader r(m_path);
        EXPECT_EQ(r.readU32(), v)
            << "Round-trip failed for value " << v;
    }
}

TEST_F(BinaryRoundTripTest, U64RoundTrip)
{
    const uint64_t values[] = {
        0ull, 1ull,
        static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) + 1,
        0xDEADBEEFCAFEBABEull,
        std::numeric_limits<uint64_t>::max()
    };

    for (uint64_t v : values) {
        { BinaryWriter w(m_path); w.writeU64(v); }

        BinaryReader r(m_path);
        EXPECT_EQ(r.readU64(), v);
    }
}

TEST_F(BinaryRoundTripTest, F32RoundTrip)
{
    const float values[] = {
        0.0f, 1.0f, -1.0f, 3.14159f,
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::min(),
        99.5f, -0.001f
    };

    for (float v : values) {
        { BinaryWriter w(m_path); w.writeF32(v); }

        BinaryReader r(m_path);
        float loaded = r.readF32();
        // Use bit-exact comparison for round-trip
        // (memcpy-based serialization should be perfectly exact)
        EXPECT_EQ(loaded, v)
            << "Float round-trip failed for " << v;
    }
}

TEST_F(BinaryRoundTripTest, StringRoundTrip)
{
    const std::string values[] = {
        "",
        "a",
        "Hello, World!",
        "Smart Librarian — OCR Test",
        std::string(1000, 'x'),  // 1000-char string
        "path/to/some/document.png"
    };

    for (const auto& s : values) {
        { BinaryWriter w(m_path); w.writeString(s); }

        BinaryReader r(m_path);
        EXPECT_EQ(r.readString(), s)
            << "String round-trip failed for string of length "
            << s.size();
    }
}

TEST_F(BinaryRoundTripTest, MultipleValuesRoundTrip)
{
    // Write several values in sequence — tests that position tracking
    // and sequential read/write alignment work correctly
    {
        BinaryWriter w(m_path);
        w.writeU32(42u);
        w.writeString("neural");
        w.writeF32(0.95f);
        w.writeU32(100u);
        w.writeString("networks");
    }

    BinaryReader r(m_path);
    EXPECT_EQ(r.readU32(),    42u);
    EXPECT_EQ(r.readString(), "neural");
    EXPECT_EQ(r.readF32(),    0.95f);
    EXPECT_EQ(r.readU32(),    100u);
    EXPECT_EQ(r.readString(), "networks");
}

TEST_F(BinaryRoundTripTest, WriterTracksBytesWritten)
{
    BinaryWriter w(m_path);
    EXPECT_EQ(w.position(), 0u);

    w.writeU32(1u);
    EXPECT_EQ(w.position(), 4u);

    w.writeU32(2u);
    EXPECT_EQ(w.position(), 8u);

    w.writeString("hi");
    // string: 4 bytes length + 2 bytes data = 6 bytes
    EXPECT_EQ(w.position(), 14u);
}

TEST_F(BinaryRoundTripTest, ReaderThrowsOnTruncatedFile)
{
    // Write only 2 bytes but try to read a uint32 (needs 4)
    {
        BinaryWriter w(m_path);
        w.writeU8(0xAB);
        w.writeU8(0xCD);
    }

    BinaryReader r(m_path);
    EXPECT_THROW(r.readU32(), std::runtime_error);
}

TEST_F(BinaryRoundTripTest, MagicBytesRoundTrip)
{
    const char magic[] = "SLIDX001";  // 8 bytes, no null
    {
        BinaryWriter w(m_path);
        w.writeRawBytes(magic, 8);
    }

    BinaryReader r(m_path);
    char loaded[8];
    r.readRawBytes(loaded, 8);
    EXPECT_EQ(std::memcmp(magic, loaded, 8), 0)
        << "Magic bytes did not round-trip correctly";
}
