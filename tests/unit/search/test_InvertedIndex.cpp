#include <gtest/gtest.h>
#include "search/InvertedIndex.h"

using namespace sl::search;

class InvertedIndexTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Register two documents
        doc1 = index.registerDocument(
            "path/to/doc1.txt", "Document One", 100);
        doc2 = index.registerDocument(
            "path/to/doc2.txt", "Document Two", 200);

        // Index some words
        index.addPosting("neural", doc1, 10);
        index.addPosting("neural", doc1, 25);   // second occurrence → freq=2
        index.addPosting("neural", doc2, 5);

        index.addPosting("network", doc1, 15);
        index.addPosting("algorithm", doc2, 30);
    }

    InvertedIndex index;
    DocumentID    doc1 { 0 };
    DocumentID    doc2 { 0 };
};

TEST_F(InvertedIndexTest, RegisteredDocumentsAreRetrievable)
{
    const DocumentInfo* info1 = index.getDocumentInfo(doc1);
    ASSERT_NE(info1, nullptr);
    EXPECT_EQ(info1->path,  "path/to/doc1.txt");
    EXPECT_EQ(info1->title, "Document One");
    EXPECT_EQ(info1->totalWords, 100u);
}

TEST_F(InvertedIndexTest, PostingListHasCorrectSize)
{
    // "neural" appears in both documents
    const auto& postings = index.getPostings("neural");
    EXPECT_EQ(postings.size(), 2u);
}

TEST_F(InvertedIndexTest, TermFrequencyAccumulatesCorrectly)
{
    // "neural" was added twice for doc1 — termFreq should be 2
    const auto& postings = index.getPostings("neural");

    const Posting* doc1Posting = nullptr;
    for (const auto& p : postings) {
        if (p.docId == doc1) {
            doc1Posting = &p;
            break;
        }
    }

    ASSERT_NE(doc1Posting, nullptr);
    EXPECT_EQ(doc1Posting->termFreq, 2u);
}

TEST_F(InvertedIndexTest, DocumentFrequencyIsCorrect)
{
    // "neural" appears in 2 documents
    EXPECT_EQ(index.documentFrequency("neural"), 2u);
    // "network" appears in 1 document
    EXPECT_EQ(index.documentFrequency("network"), 1u);
    // "unknown" appears in 0 documents
    EXPECT_EQ(index.documentFrequency("unknown"), 0u);
}

TEST_F(InvertedIndexTest, EmptyPostingsForUnknownWord)
{
    const auto& postings = index.getPostings("nonexistent");
    EXPECT_TRUE(postings.empty());
}

TEST_F(InvertedIndexTest, TotalDocumentsIsCorrect)
{
    EXPECT_EQ(index.totalDocuments(), 2u);
}

TEST_F(InvertedIndexTest, TotalUniqueWordsIsCorrect)
{
    // "neural", "network", "algorithm" = 3 unique words
    EXPECT_EQ(index.totalUniqueWords(), 3u);
}

TEST_F(InvertedIndexTest, ClearResetsEverything)
{
    index.clear();
    EXPECT_EQ(index.totalDocuments(), 0u);
    EXPECT_EQ(index.totalUniqueWords(), 0u);
    EXPECT_TRUE(index.getPostings("neural").empty());
    EXPECT_TRUE(index.empty());
}

// ─────────────────────────────────────────────────────────────────────
// Persistence API tests
// ─────────────────────────────────────────────────────────────────────

TEST(InvertedIndexPersistence, RegisterWithIdPreservesId)
{
    InvertedIndex index;
    index.registerDocumentWithId(42u, "path.txt", "Title", 100u);

    const DocumentInfo* info = index.getDocumentInfo(42u);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->id,    42u);
    EXPECT_EQ(info->path,  "path.txt");
    EXPECT_EQ(info->title, "Title");
}

TEST(InvertedIndexPersistence, AddPostingDirectBypassesDuplicateCheck)
{
    InvertedIndex index;
    index.registerDocumentWithId(1u, "p.txt", "T", 10u);

    // addPostingDirect writes exactly what we give it
    index.addPostingDirect("word", 1u, 5u, 0u);
    index.addPostingDirect("word", 2u, 3u, 10u);

    const auto& postings = index.getPostings("word");
    ASSERT_EQ(postings.size(), 2u);

    EXPECT_EQ(postings[0].termFreq, 5u);
    EXPECT_EQ(postings[1].termFreq, 3u);
}
