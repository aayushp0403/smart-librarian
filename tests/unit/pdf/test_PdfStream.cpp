#include <gtest/gtest.h>
#include "pdf/PdfStream.h"

#include <fstream>
#include <filesystem>
#include <string>
#include <cstdio>

using namespace sl::pdf;

// ─────────────────────────────────────────────────────────────────────
// Fixture: creates and cleans up a temp file for each test
// ─────────────────────────────────────────────────────────────────────

class PdfStreamTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Use /tmp for test files — no permissions issues
        m_path = "/tmp/sl_test_pdfstream_" +
                 std::to_string(
                     std::hash<std::string>{}(
                         ::testing::UnitTest::GetInstance()
                             ->current_test_info()->name())) +
                 ".bin";
    }

    void TearDown() override
    {
        // Clean up temp file after each test
        std::error_code ec;
        std::filesystem::remove(m_path, ec);
    }

    // Helper: read the entire file as a string
    std::string readFile() const
    {
        std::ifstream f(m_path, std::ios::binary);
        return std::string(
            std::istreambuf_iterator<char>(f),
            std::istreambuf_iterator<char>());
    }

    std::string m_path;
};

TEST_F(PdfStreamTest, WritesStringCorrectly)
{
    {
        PdfStream stream(m_path);
        stream.writeStr("Hello");
    }  // destructor flushes and closes

    std::string content = readFile();
    EXPECT_EQ(content, "Hello");
}

TEST_F(PdfStreamTest, TracksPositionAfterWrite)
{
    PdfStream stream(m_path);
    EXPECT_EQ(stream.position(), 0u);

    stream.writeStr("ABC");
    EXPECT_EQ(stream.position(), 3u);

    stream.writeStr("DE");
    EXPECT_EQ(stream.position(), 5u);
}

TEST_F(PdfStreamTest, WriteFormattedProducesCorrectOutput)
{
    {
        PdfStream stream(m_path);
        stream.writeF("%d 0 obj\n", 1);
    }

    std::string content = readFile();
    EXPECT_EQ(content, "1 0 obj\n");
}

TEST_F(PdfStreamTest, WriteBytesMatchesExpectedContent)
{
    const char data[] = { 0x25, 0x50, 0x44, 0x46 };  // %PDF
    {
        PdfStream stream(m_path);
        stream.writeBytes(data, 4);
    }

    std::string content = readFile();
    ASSERT_EQ(content.size(), 4u);
    EXPECT_EQ(static_cast<unsigned char>(content[0]), 0x25u);
    EXPECT_EQ(static_cast<unsigned char>(content[1]), 0x50u);
    EXPECT_EQ(static_cast<unsigned char>(content[2]), 0x44u);
    EXPECT_EQ(static_cast<unsigned char>(content[3]), 0x46u);
}

TEST_F(PdfStreamTest, NonExistentDirectoryThrows)
{
    EXPECT_THROW(
        PdfStream s("/nonexistent/path/file.pdf"),
        std::runtime_error
    );
}
