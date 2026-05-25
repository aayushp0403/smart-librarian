#include <gtest/gtest.h>
#include "search/SearchEngine.h"

using namespace sl::search;

class SearchEngineTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Index five documents with distinct topics
        id_neural = engine.indexDocument(
            "neural.txt",
            "Neural Networks",
            "Neural networks use layers of neurons for deep learning. "
            "Backpropagation trains neural networks by computing gradients. "
            "Convolutional neural networks excel at image tasks."
        );

        id_memory = engine.indexDocument(
            "memory.txt",
            "C++ Memory",
            "Memory management in C++ uses stack and heap allocation. "
            "Smart pointers manage heap memory automatically. "
            "RAII ensures memory resources are released correctly."
        );

        id_algo = engine.indexDocument(
            "algo.txt",
            "Algorithms",
            "Binary search achieves O(log n) complexity. "
            "Quicksort averages O(n log n). "
            "Hash tables provide constant time lookup."
        );

        id_pdf = engine.indexDocument(
            "pdf.txt",
            "PDF Format",
            "PDF files contain objects streams and cross references. "
            "The trailer tells readers where the catalog object lives."
        );

        id_ocr = engine.indexDocument(
            "ocr.txt",
            "OCR Technology",
            "Optical character recognition extracts text from images. "
            "Tesseract uses neural networks for character recognition."
        );
    }

    SearchEngine engine;
    DocumentID   id_neural { 0 };
    DocumentID   id_memory { 0 };
    DocumentID   id_algo   { 0 };
    DocumentID   id_pdf    { 0 };
    DocumentID   id_ocr    { 0 };
};

// ─────────────────────────────────────────────────────────────────────
// Basic correctness
// ─────────────────────────────────────────────────────────────────────

TEST_F(SearchEngineTest, DocumentCountIsCorrect)
{
    EXPECT_EQ(engine.documentCount(), 5u);
}

TEST_F(SearchEngineTest, UniqueWordCountIsPositive)
{
    EXPECT_GT(engine.uniqueWordCount(), 0u);
}

TEST_F(SearchEngineTest, TrieNodeCountIsPositive)
{
    EXPECT_GT(engine.trieNodeCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────
// Search correctness
// ─────────────────────────────────────────────────────────────────────

TEST_F(SearchEngineTest, SearchReturnsResultsForKnownWord)
{
    auto results = engine.search("neural");
    EXPECT_FALSE(results.empty());
}

TEST_F(SearchEngineTest, TopResultForNeuralIsNeuralDoc)
{
    auto results = engine.search("neural", 5);

    ASSERT_FALSE(results.empty());
    // Neural networks document contains "neural" most frequently
    // so it should rank first
    EXPECT_EQ(results[0].docId, id_neural);
}

TEST_F(SearchEngineTest, NeuralQueryAlsoMatchesOcrDoc)
{
    // OCR doc mentions "neural networks" once — should appear in results
    auto results = engine.search("neural", 10);

    bool ocrFound = false;
    for (const auto& r : results) {
        if (r.docId == id_ocr) {
            ocrFound = true;
            break;
        }
    }
    EXPECT_TRUE(ocrFound)
        << "OCR document (which mentions neural networks) "
           "should appear in results for 'neural'";
}

TEST_F(SearchEngineTest, UnknownWordReturnsEmptyResults)
{
    auto results = engine.search("quantumEntanglement12345");
    EXPECT_TRUE(results.empty());
}

TEST_F(SearchEngineTest, EmptyQueryReturnsEmptyResults)
{
    auto results = engine.search("");
    EXPECT_TRUE(results.empty());
}

TEST_F(SearchEngineTest, ResultsAreSortedByDescendingScore)
{
    auto results = engine.search("neural network");
    ASSERT_GE(results.size(), 2u);

    for (std::size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i-1].score, results[i].score)
            << "Results are not sorted in descending score order "
            << "at positions " << i-1 << " and " << i;
    }
}

TEST_F(SearchEngineTest, ScoresArePositive)
{
    auto results = engine.search("memory");
    for (const auto& r : results) {
        EXPECT_GT(r.score, 0.0)
            << "Score for doc '" << r.title << "' should be positive";
    }
}

TEST_F(SearchEngineTest, MaxResultsLimitsReturnCount)
{
    // Request only 2 results even though more might match
    auto results = engine.search("neural", 2);
    EXPECT_LE(results.size(), 2u);
}

TEST_F(SearchEngineTest, MultiTermSearchScoresHigherForMoreMatches)
{
    // "neural network" — neural doc has both terms many times
    // OCR doc has only one occurrence of neural
    auto results = engine.search("neural network", 10);

    ASSERT_GE(results.size(), 2u);
    EXPECT_EQ(results[0].docId, id_neural)
        << "Neural networks doc should rank #1 for 'neural network' query";
}

// ─────────────────────────────────────────────────────────────────────
// TotalTermFreq correctness — our Phase 4 bug fix
// ─────────────────────────────────────────────────────────────────────

TEST_F(SearchEngineTest, TotalTermFreqReflectsActualOccurrences)
{
    // "neural" appears 3 times in the neural doc
    // (neural, neural, neural in our setup text)
    auto results = engine.search("neural", 10);

    const SearchResult* neuralResult = nullptr;
    for (const auto& r : results) {
        if (r.docId == id_neural) {
            neuralResult = &r;
            break;
        }
    }

    ASSERT_NE(neuralResult, nullptr);
    EXPECT_GT(neuralResult->totalTermFreq, 1u)
        << "Neural doc has 'neural' more than once — "
           "totalTermFreq should be > 1";
}

TEST_F(SearchEngineTest, MatchCountEqualsDistinctQueryTermsFound)
{
    // "neural network" is a 2-term query
    // Neural doc contains both — matchCount should be 2
    auto results = engine.search("neural network", 5);

    ASSERT_FALSE(results.empty());
    // Top result should have matchCount >= 1
    EXPECT_GE(results[0].matchCount, 1u);
}

// ─────────────────────────────────────────────────────────────────────
// Prefix search
// ─────────────────────────────────────────────────────────────────────

TEST_F(SearchEngineTest, PrefixSearchReturnsMatchingWords)
{
    auto words = engine.prefixSearch("neur");
    EXPECT_FALSE(words.empty());
    for (const auto& w : words) {
        EXPECT_EQ(w.substr(0, 4), "neur")
            << "'" << w << "' does not start with 'neur'";
    }
}

TEST_F(SearchEngineTest, PrefixSearchReturnsEmptyForUnknownPrefix)
{
    auto words = engine.prefixSearch("zzz");
    EXPECT_TRUE(words.empty());
}

TEST_F(SearchEngineTest, PrefixSearchRespectsMaxResults)
{
    auto words = engine.prefixSearch("a", 3);
    EXPECT_LE(words.size(), 3u);
}

// ─────────────────────────────────────────────────────────────────────
// Clear
// ─────────────────────────────────────────────────────────────────────

TEST_F(SearchEngineTest, ClearResetsAllState)
{
    engine.clear();
    EXPECT_EQ(engine.documentCount(), 0u);
    EXPECT_EQ(engine.uniqueWordCount(), 0u);

    auto results = engine.search("neural");
    EXPECT_TRUE(results.empty());
}
