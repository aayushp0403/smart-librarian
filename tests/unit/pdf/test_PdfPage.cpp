#include <gtest/gtest.h>
#include "pdf/PdfPage.h"

using namespace sl::pdf;

TEST(PdfPageTest, EmptyPageHasEmptyContentStream)
{
    PdfPage page;
    EXPECT_TRUE(page.getContentStream().empty());
}

TEST(PdfPageTest, BeginEndTextProducesCorrectOperators)
{
    PdfPage page;
    page.beginText();
    page.endText();

    std::string content = page.getContentStream();
    EXPECT_NE(content.find("BT"), std::string::npos);
    EXPECT_NE(content.find("ET"), std::string::npos);
}

TEST(PdfPageTest, SetFontProducesCorrectOperator)
{
    PdfPage page;
    page.beginText();
    page.setFont("F1", 12.0f);
    page.endText();

    std::string content = page.getContentStream();
    EXPECT_NE(content.find("/F1"), std::string::npos);
    EXPECT_NE(content.find("Tf"),  std::string::npos);
}

TEST(PdfPageTest, ShowTextEscapesParentheses)
{
    PdfPage page;
    page.beginText();
    page.moveTo(100, 700);
    page.showText("Hello (World)");
    page.endText();

    std::string content = page.getContentStream();
    // Parentheses must be escaped with backslash
    EXPECT_NE(content.find("\\("), std::string::npos);
    EXPECT_NE(content.find("\\)"), std::string::npos);
    // Raw unescaped parens around the string literal should not appear
    // as adjacent unescaped pairs (the outer parens are the string delimiters)
    EXPECT_NE(content.find("Tj"), std::string::npos);
}

TEST(PdfPageTest, PageDimensionsAreUSLetter)
{
    PdfPage page;
    EXPECT_FLOAT_EQ(page.width(),  612.0f);
    EXPECT_FLOAT_EQ(page.height(), 792.0f);
}

TEST(PdfPageTest, DrawLineProducesPathOperators)
{
    PdfPage page;
    page.drawLine(0, 0, 100, 100);
    std::string content = page.getContentStream();
    EXPECT_NE(content.find(" m\n"), std::string::npos);  // moveto
    EXPECT_NE(content.find(" l\n"), std::string::npos);  // lineto
    EXPECT_NE(content.find("S\n"),  std::string::npos);  // stroke
}

TEST(PdfPageTest, FillRectProducesFillOperator)
{
    PdfPage page;
    page.setFillColor(1.0f, 0.0f, 0.0f);
    page.fillRect(10, 10, 100, 50);
    std::string content = page.getContentStream();
    EXPECT_NE(content.find("re\n"), std::string::npos);  // rectangle
    EXPECT_NE(content.find("f\n"),  std::string::npos);  // fill
}

TEST(PdfPageTest, MoveToProducesTdOperator)
{
    PdfPage page;
    page.beginText();
    page.moveTo(72.0f, 700.0f);
    page.endText();

    std::string content = page.getContentStream();
    EXPECT_NE(content.find("Td"), std::string::npos);
}
