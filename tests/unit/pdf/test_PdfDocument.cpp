#include <gtest/gtest.h>
#include "pdf/PdfDocument.h"

#include <fstream>
#include <filesystem>
#include <string>

using namespace sl::pdf;

class PdfDocumentTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_outputPath = "/tmp/sl_test_doc.pdf";
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove(m_outputPath, ec);
    }

    // Read first N bytes of output file
    std::string readFirstBytes(std::size_t n) const
    {
        std::ifstream f(m_outputPath, std::ios::binary);
        std::string result(n, '\0');
        f.read(result.data(), static_cast<std::streamsize>(n));
        return result;
    }

    // Check if a string exists anywhere in the file
    bool fileContains(const std::string& needle) const
    {
        std::ifstream f(m_outputPath, std::ios::binary);
        std::string content{
            std::istreambuf_iterator<char>(f),
            std::istreambuf_iterator<char>()};
        return content.find(needle) != std::string::npos;
    }

    std::string m_outputPath;
};

TEST_F(PdfDocumentTest, NewDocumentHasZeroPages)
{
    PdfDocument doc;
    EXPECT_EQ(doc.pageCount(), 0u);
}

TEST_F(PdfDocumentTest, AddPageIncrementsPageCount)
{
    PdfDocument doc;
    doc.addPage();
    EXPECT_EQ(doc.pageCount(), 1u);

    doc.addPage();
    EXPECT_EQ(doc.pageCount(), 2u);
}

TEST_F(PdfDocumentTest, DuplicateFontAliasThrows)
{
    PdfDocument doc;
    doc.addFont("F1", "Helvetica");
    EXPECT_THROW(doc.addFont("F1", "Times-Roman"),
                 std::invalid_argument);
}

TEST_F(PdfDocumentTest, SaveWithNoFontsThrows)
{
    PdfDocument doc;
    doc.addPage();
    // No fonts registered
    EXPECT_THROW(doc.save(m_outputPath), std::runtime_error);
}

TEST_F(PdfDocumentTest, SaveWithNoPagesThrows)
{
    PdfDocument doc;
    doc.addFont("F1", "Helvetica");
    // No pages added
    EXPECT_THROW(doc.save(m_outputPath), std::runtime_error);
}

TEST_F(PdfDocumentTest, GeneratedPdfStartsWithMagicBytes)
{
    PdfDocument doc;
    doc.addFont("F1", "Helvetica");

    PdfPage& page = doc.addPage();
    page.beginText();
    page.setFont("F1", 12.0f);
    page.moveTo(100, 700);
    page.showText("Test");
    page.endText();

    doc.save(m_outputPath);

    // A valid PDF must start with "%PDF-"
    std::string header = readFirstBytes(5);
    EXPECT_EQ(header, "%PDF-")
        << "Generated file does not start with '%PDF-' magic bytes";
}

TEST_F(PdfDocumentTest, GeneratedPdfContainsEof)
{
    PdfDocument doc;
    doc.addFont("F1", "Helvetica");
    PdfPage& page = doc.addPage();
    page.beginText();
    page.setFont("F1", 12.0f);
    page.moveTo(100, 700);
    page.showText("EOF test");
    page.endText();
    doc.save(m_outputPath);

    EXPECT_TRUE(fileContains("%%EOF"))
        << "Generated PDF does not contain %%EOF marker";
}

TEST_F(PdfDocumentTest, GeneratedPdfContainsXrefTable)
{
    PdfDocument doc;
    doc.addFont("F1", "Helvetica");
    PdfPage& page = doc.addPage();
    page.beginText();
    page.setFont("F1", 12.0f);
    page.moveTo(100, 700);
    page.showText("xref test");
    page.endText();
    doc.save(m_outputPath);

    EXPECT_TRUE(fileContains("xref"))
        << "Generated PDF does not contain xref table";
    EXPECT_TRUE(fileContains("startxref"))
        << "Generated PDF does not contain startxref";
    EXPECT_TRUE(fileContains("trailer"))
        << "Generated PDF does not contain trailer dictionary";
}

TEST_F(PdfDocumentTest, MultiPageDocumentContainsCatalog)
{
    PdfDocument doc;
    doc.addFont("F1", "Helvetica");

    for (int i = 0; i < 3; ++i) {
        PdfPage& page = doc.addPage();
        page.beginText();
        page.setFont("F1", 11.0f);
        page.moveTo(72, 700);
        page.showText("Page " + std::to_string(i + 1));
        page.endText();
    }

    doc.save(m_outputPath);

    EXPECT_EQ(doc.pageCount(), 3u);
    EXPECT_TRUE(fileContains("/Catalog"));
    EXPECT_TRUE(fileContains("/Pages"));
}
