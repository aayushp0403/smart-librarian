#include <gtest/gtest.h>
#include "search/TextProcessor.h"

using namespace sl::search;

// ─────────────────────────────────────────────────────────────────────
// Test suite: TextProcessorNormalize
// Each TEST(SuiteName, TestName) is independent — order doesn't matter.
// ─────────────────────────────────────────────────────────────────────

TEST(TextProcessorNormalize, LowercasesAllCharacters)
{
    EXPECT_EQ(TextProcessor::normalize("HELLO"), "hello");
    EXPECT_EQ(TextProcessor::normalize("Hello"), "hello");
    EXPECT_EQ(TextProcessor::normalize("hElLo"), "hello");
}

TEST(TextProcessorNormalize, RemovesNonAlphaCharacters)
{
    EXPECT_EQ(TextProcessor::normalize("hello!"),    "hello");
    EXPECT_EQ(TextProcessor::normalize("c++"),       "c");
    EXPECT_EQ(TextProcessor::normalize("123abc"),    "abc");
    EXPECT_EQ(TextProcessor::normalize("don't"),     "dont");
}

TEST(TextProcessorNormalize, EmptyStringReturnsEmpty)
{
    EXPECT_EQ(TextProcessor::normalize(""), "");
    EXPECT_EQ(TextProcessor::normalize("123"), "");
    EXPECT_EQ(TextProcessor::normalize("!!!"), "");
}

TEST(TextProcessorNormalize, HandlesAllAlpha)
{
    EXPECT_EQ(TextProcessor::normalize("algorithm"), "algorithm");
}

// ─────────────────────────────────────────────────────────────────────
// Test suite: TextProcessorStopWords
// ─────────────────────────────────────────────────────────────────────

TEST(TextProcessorStopWords, CommonWordsAreStopWords)
{
    EXPECT_TRUE(TextProcessor::isStopWord("the"));
    EXPECT_TRUE(TextProcessor::isStopWord("and"));
    EXPECT_TRUE(TextProcessor::isStopWord("is"));
    EXPECT_TRUE(TextProcessor::isStopWord("a"));
    EXPECT_TRUE(TextProcessor::isStopWord("in"));
}

TEST(TextProcessorStopWords, TechnicalWordsAreNotStopWords)
{
    EXPECT_FALSE(TextProcessor::isStopWord("algorithm"));
    EXPECT_FALSE(TextProcessor::isStopWord("neural"));
    EXPECT_FALSE(TextProcessor::isStopWord("pointer"));
    EXPECT_FALSE(TextProcessor::isStopWord("trie"));
}

// ─────────────────────────────────────────────────────────────────────
// Test suite: TextProcessorTokenize
// ─────────────────────────────────────────────────────────────────────

TEST(TextProcessorTokenize, SplitsOnNonAlpha)
{
    auto tokens = TextProcessor::tokenize("hello world");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
}

TEST(TextProcessorTokenize, RemovesStopWords)
{
    // "the" and "is" are stop words — should not appear
    auto tokens = TextProcessor::tokenize("the algorithm is fast");
    for (const auto& t : tokens) {
        EXPECT_NE(t, "the");
        EXPECT_NE(t, "is");
    }
    // "algorithm" and "fast" should remain
    bool hasAlgorithm = false, hasFast = false;
    for (const auto& t : tokens) {
        if (t == "algorithm") hasAlgorithm = true;
        if (t == "fast")      hasFast      = true;
    }
    EXPECT_TRUE(hasAlgorithm);
    EXPECT_TRUE(hasFast);
}

TEST(TextProcessorTokenize, RemovesShortTokens)
{
    // Default minLength = 2, so single chars are removed
    auto tokens = TextProcessor::tokenize("a b hello c");
    for (const auto& t : tokens) {
        EXPECT_GE(t.size(), 2u);
    }
}

TEST(TextProcessorTokenize, EmptyInputReturnsEmptyVector)
{
    auto tokens = TextProcessor::tokenize("");
    EXPECT_TRUE(tokens.empty());
}

TEST(TextProcessorTokenize, HandlesRepeatedDelimiters)
{
    auto tokens = TextProcessor::tokenize("hello,,,world...test");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
    EXPECT_EQ(tokens[2], "test");
}

TEST(TextProcessorTokenizeQuery, PreservesStopWords)
{
    // tokenizeQuery should NOT remove stop words
    auto tokens = TextProcessor::tokenizeQuery("to be or not to be");
    bool hasTo = false, hasBe = false;
    for (const auto& t : tokens) {
        if (t == "to") hasTo = true;
        if (t == "be") hasBe = true;
    }
    EXPECT_TRUE(hasTo);
    EXPECT_TRUE(hasBe);
}
