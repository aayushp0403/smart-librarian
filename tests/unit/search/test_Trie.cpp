#include <gtest/gtest.h>
#include "search/Trie.h"

using namespace sl::search;

// ─────────────────────────────────────────────────────────────────────
// Fixture: TrieTest
//
// A test fixture is a class that provides shared setup for multiple
// tests. Each TEST_F creates a fresh instance of the fixture —
// tests cannot affect each other through shared state.
//
// Why use a fixture here?
//   Multiple tests need a pre-populated Trie.
//   The fixture's SetUp() runs before EACH test.
//   No test depends on the state left by another test.
//   This is the "test isolation" principle.
// ─────────────────────────────────────────────────────────────────────

class TrieTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Words inserted before every test
        trie.insert("car");
        trie.insert("card");
        trie.insert("care");
        trie.insert("careful");
        trie.insert("cat");
        trie.insert("dog");
        trie.insert("dodge");
    }

    Trie trie;
};

TEST_F(TrieTest, ContainsInsertedWords)
{
    EXPECT_TRUE(trie.contains("car"));
    EXPECT_TRUE(trie.contains("card"));
    EXPECT_TRUE(trie.contains("care"));
    EXPECT_TRUE(trie.contains("cat"));
    EXPECT_TRUE(trie.contains("dog"));
}

TEST_F(TrieTest, DoesNotContainPrefixesUnlessInserted)
{
    // "ca" is a prefix of "car" and "cat" but was never inserted
    EXPECT_FALSE(trie.contains("ca"));
    EXPECT_FALSE(trie.contains("do"));
    EXPECT_FALSE(trie.contains("c"));
}

TEST_F(TrieTest, DoesNotContainUninsertedWords)
{
    EXPECT_FALSE(trie.contains("cart"));
    EXPECT_FALSE(trie.contains("dogs"));
    EXPECT_FALSE(trie.contains("zebra"));
    EXPECT_FALSE(trie.contains(""));
}

TEST_F(TrieTest, StartsWithReturnsTrueForValidPrefixes)
{
    EXPECT_TRUE(trie.startsWith("ca"));
    EXPECT_TRUE(trie.startsWith("car"));
    EXPECT_TRUE(trie.startsWith("do"));
    EXPECT_TRUE(trie.startsWith("c"));
    EXPECT_TRUE(trie.startsWith("d"));
}

TEST_F(TrieTest, StartsWithReturnsFalseForInvalidPrefixes)
{
    EXPECT_FALSE(trie.startsWith("ze"));
    EXPECT_FALSE(trie.startsWith("xyz"));
    EXPECT_FALSE(trie.startsWith("dogs")); // no word starts with "dogs"
}

TEST_F(TrieTest, AllWithPrefixReturnsCorrectWords)
{
    auto results = trie.allWithPrefix("car");

    // Should find: car, card, care, careful
    ASSERT_EQ(results.size(), 4u);

    // Check all expected words are present (order not guaranteed)
    auto contains = [&](const std::string& w) {
        return std::find(results.begin(), results.end(), w)
               != results.end();
    };

    EXPECT_TRUE(contains("car"));
    EXPECT_TRUE(contains("card"));
    EXPECT_TRUE(contains("care"));
    EXPECT_TRUE(contains("careful"));
}

TEST_F(TrieTest, AllWithPrefixRespectsMaxResults)
{
    // "car" prefix has 4 matches, request max 2
    auto results = trie.allWithPrefix("car", 2);
    EXPECT_LE(results.size(), 2u);
}

TEST_F(TrieTest, AllWithPrefixReturnsEmptyForUnknownPrefix)
{
    auto results = trie.allWithPrefix("xyz");
    EXPECT_TRUE(results.empty());
}

TEST_F(TrieTest, WordCountTracksUniqueInsertions)
{
    // 7 unique words were inserted in SetUp()
    EXPECT_EQ(trie.wordCount(), 7u);
}

TEST_F(TrieTest, DuplicateInsertionDoesNotIncreaseWordCount)
{
    std::size_t before = trie.wordCount();
    trie.insert("car");   // already inserted
    trie.insert("dog");   // already inserted
    EXPECT_EQ(trie.wordCount(), before);
}

TEST_F(TrieTest, ClearResetsAllState)
{
    trie.clear();
    EXPECT_EQ(trie.wordCount(), 0u);
    EXPECT_EQ(trie.nodeCount(), 1u);  // only root node remains
    EXPECT_FALSE(trie.contains("car"));
    EXPECT_FALSE(trie.startsWith("c"));
}

// ─────────────────────────────────────────────────────────────────────
// Edge case tests — these are the tests that find real bugs
// ─────────────────────────────────────────────────────────────────────

TEST(TrieEdgeCases, EmptyTrieContainsNothing)
{
    Trie t;
    EXPECT_FALSE(t.contains("anything"));
    EXPECT_FALSE(t.startsWith("a"));
    EXPECT_EQ(t.wordCount(), 0u);
}

TEST(TrieEdgeCases, SingleCharacterWords)
{
    Trie t;
    // Single char words are unusual but valid
    t.insert("a");
    t.insert("b");
    EXPECT_TRUE(t.contains("a"));
    EXPECT_TRUE(t.contains("b"));
    EXPECT_FALSE(t.contains("c"));
}

TEST(TrieEdgeCases, LongWordHandled)
{
    Trie t;
    std::string longWord(100, 'a');  // 100 'a' characters
    t.insert(longWord);
    EXPECT_TRUE(t.contains(longWord));
    EXPECT_FALSE(t.contains(std::string(99, 'a')));
    EXPECT_TRUE(t.startsWith(std::string(50, 'a')));
}

TEST(TrieEdgeCases, NodeCountGrowsWithInsertions)
{
    Trie t;
    std::size_t initial = t.nodeCount();  // just root = 1
    EXPECT_EQ(initial, 1u);

    t.insert("abc");   // adds 3 nodes
    EXPECT_GT(t.nodeCount(), initial);

    t.insert("abd");   // shares "ab", adds 1 node
    std::size_t afterTwo = t.nodeCount();

    t.insert("abc");   // duplicate — no new nodes
    EXPECT_EQ(t.nodeCount(), afterTwo);
}
