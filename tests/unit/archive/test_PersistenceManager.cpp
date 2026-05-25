#include <gtest/gtest.h>
#include "archive/PersistenceManager.h"
#include "search/SearchEngine.h"
#include "gui/ArchiveWidget.h"

#include <filesystem>

using namespace sl::archive;
using namespace sl::search;
using namespace sl::gui;

class PersistenceManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Use a temp directory unique to this test run
        m_testDir = "/tmp/sl_test_persistence_" +
            std::to_string(
                reinterpret_cast<std::size_t>(this));

        m_persistence =
            std::make_unique<PersistenceManager>(m_testDir);
    }

    void TearDown() override
    {
        m_persistence.reset();

        // Clean up temp directory
        std::error_code ec;
        std::filesystem::remove_all(m_testDir, ec);
    }

    std::string                        m_testDir;
    std::unique_ptr<PersistenceManager> m_persistence;
};

// ─────────────────────────────────────────────────────────────────────
// Config round-trip
// ─────────────────────────────────────────────────────────────────────

TEST_F(PersistenceManagerTest, ConfigRoundTrip)
{
    AppConfig cfg;
    cfg.confidenceThreshold = 75;
    cfg.autoIndexOnDrop     = false;
    cfg.maxSearchResults    = 15;
    cfg.addRecentFile("/home/user/doc1.png");
    cfg.addRecentFile("/home/user/doc2.png");

    ASSERT_TRUE(m_persistence->saveConfig(cfg));

    AppConfig loaded;
    ASSERT_TRUE(m_persistence->loadConfig(loaded));

    EXPECT_EQ(loaded.confidenceThreshold, 75);
    EXPECT_EQ(loaded.autoIndexOnDrop,     false);
    EXPECT_EQ(loaded.maxSearchResults,    15);
    ASSERT_EQ(loaded.recentFiles.size(),  2u);
    EXPECT_EQ(loaded.recentFiles[0], "/home/user/doc2.png");
    EXPECT_EQ(loaded.recentFiles[1], "/home/user/doc1.png");
}

TEST_F(PersistenceManagerTest, LoadConfigReturnsFalseIfFileAbsent)
{
    AppConfig cfg;
    // No save — file doesn't exist
    EXPECT_FALSE(m_persistence->loadConfig(cfg));
}

// ─────────────────────────────────────────────────────────────────────
// Archive round-trip
// ─────────────────────────────────────────────────────────────────────

TEST_F(PersistenceManagerTest, ArchiveRoundTrip)
{
    std::vector<ArchivedDocument> docs;

    ArchivedDocument d1;
    d1.id             = 1;
    d1.filename       = "scan001.png";
    d1.fullPath       = "/home/user/scan001.png";
    d1.extractedText  = "Hello world this is OCR output";
    d1.ocrConfidence  = 87.5f;
    d1.wordCount      = 6;
    d1.timestamp      = "2024-01-15 10:30";
    docs.push_back(d1);

    ArchivedDocument d2;
    d2.id             = 2;
    d2.filename       = "report.png";
    d2.fullPath       = "/home/user/report.png";
    d2.extractedText  = "Quarterly financial report summary";
    d2.ocrConfidence  = 92.1f;
    d2.wordCount      = 5;
    d2.timestamp      = "2024-01-15 11:00";
    docs.push_back(d2);

    ASSERT_TRUE(m_persistence->saveArchive(docs));

    std::vector<ArchivedDocument> loaded;
    ASSERT_TRUE(m_persistence->loadArchive(loaded));

    ASSERT_EQ(loaded.size(), 2u);

    EXPECT_EQ(loaded[0].id,            1);
    EXPECT_EQ(loaded[0].filename,      "scan001.png");
    EXPECT_EQ(loaded[0].extractedText, "Hello world this is OCR output");
    EXPECT_FLOAT_EQ(loaded[0].ocrConfidence, 87.5f);
    EXPECT_EQ(loaded[0].wordCount,     6);

    EXPECT_EQ(loaded[1].id,            2);
    EXPECT_EQ(loaded[1].filename,      "report.png");
    EXPECT_FLOAT_EQ(loaded[1].ocrConfidence, 92.1f);
}

TEST_F(PersistenceManagerTest, LoadArchiveReturnsFalseIfFileAbsent)
{
    std::vector<ArchivedDocument> docs;
    EXPECT_FALSE(m_persistence->loadArchive(docs));
    EXPECT_TRUE(docs.empty());
}

// ─────────────────────────────────────────────────────────────────────
// Search index round-trip
// ─────────────────────────────────────────────────────────────────────

TEST_F(PersistenceManagerTest, IndexRoundTrip)
{
    // Build and save an index
    SearchEngine engine;
    engine.indexDocument(
        "doc1.txt", "Test Doc 1",
        "neural networks deep learning backpropagation");
    engine.indexDocument(
        "doc2.txt", "Test Doc 2",
        "algorithm complexity binary search quicksort");

    ASSERT_TRUE(m_persistence->saveIndex(engine));

    // Load into a fresh engine
    SearchEngine loaded;
    ASSERT_TRUE(m_persistence->loadIndex(loaded));

    // Verify document count preserved
    EXPECT_EQ(loaded.documentCount(), 2u);

    // Verify search still works
    auto results = loaded.search("neural");
    EXPECT_FALSE(results.empty());
    EXPECT_EQ(results[0].path, "doc1.txt");

    results = loaded.search("algorithm");
    EXPECT_FALSE(results.empty());
    EXPECT_EQ(results[0].path, "doc2.txt");
}

TEST_F(PersistenceManagerTest, IndexRoundTripPreservesWordCount)
{
    SearchEngine engine;
    engine.indexDocument("a.txt", "A", "hello world hello world hello");
    // "hello" appears 3 times, "world" appears 2 times

    ASSERT_TRUE(m_persistence->saveIndex(engine));

    SearchEngine loaded;
    ASSERT_TRUE(m_persistence->loadIndex(loaded));

    // After loading, searching "hello" should return non-empty results
    auto results = loaded.search("hello");
    ASSERT_FALSE(results.empty());

    // totalTermFreq should reflect the 3 occurrences
    EXPECT_EQ(results[0].totalTermFreq, 3u);
}

TEST_F(PersistenceManagerTest, DeleteAllRemovesAllFiles)
{
    // Create all three files
    AppConfig cfg;
    m_persistence->saveConfig(cfg);

    SearchEngine engine;
    engine.indexDocument("x.txt", "X", "test content here");
    m_persistence->saveIndex(engine);

    std::vector<ArchivedDocument> docs;
    m_persistence->saveArchive(docs);

    EXPECT_TRUE(m_persistence->indexExists());
    EXPECT_TRUE(m_persistence->archiveExists());

    m_persistence->deleteAll();

    EXPECT_FALSE(m_persistence->indexExists());
    EXPECT_FALSE(m_persistence->archiveExists());
}
