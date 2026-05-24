#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cstddef>

namespace sl {
namespace search {

/**
 * DocumentID
 *
 * A unique integer identifier for each indexed document.
 * Using a typedef makes the intent clear and allows
 * future change (e.g., to uint64_t) in one place.
 */
using DocumentID = uint32_t;

/**
 * Posting
 *
 * A single entry in a posting list: one occurrence of a word
 * in one document, with metadata.
 *
 * Memory layout (no padding needed — fields are naturally aligned):
 *   docId:        4 bytes  offset 0
 *   termFreq:     4 bytes  offset 4
 *   firstPos:     4 bytes  offset 8
 *   Total:       12 bytes
 *
 * termFrequency: how many times this word appears in this document.
 *   Used in TF-IDF calculation as the numerator of TF.
 *
 * firstPosition: character offset of the word's first occurrence.
 *   Useful for future snippet/highlight generation.
 */
struct Posting
{
    DocumentID docId        { 0 };
    uint32_t   termFreq     { 0 };
    uint32_t   firstPosition{ 0 };
};

/**
 * DocumentInfo
 *
 * Metadata about an indexed document.
 * Stored separately from posting lists to avoid redundancy.
 *
 * totalWords: used as the denominator in TF calculation.
 *   TF = termFreq / totalWords
 */
struct DocumentInfo
{
    DocumentID  id;
    std::string path;        // filesystem path or identifier
    std::string title;       // display name
    uint32_t    totalWords;  // total word count (for TF normalization)
};

/**
 * InvertedIndex
 *
 * Maps words to the list of documents containing them.
 *
 * Internal structure:
 *
 *   m_index:
 *     unordered_map<string, vector<Posting>>
 *     key   = normalized word
 *     value = posting list (sorted by docId for future merge operations)
 *
 *   m_documents:
 *     unordered_map<DocumentID, DocumentInfo>
 *     Stores metadata for each indexed document.
 *
 *   m_documentCount:
 *     Total documents indexed. Used in IDF calculation.
 *
 * Why unordered_map for m_index?
 *   Hash map: O(1) average lookup. For a search engine that
 *   performs thousands of word lookups per second, this matters.
 *   std::map (tree) would give O(log n) — slower for pure lookup.
 *
 * Why vector<Posting> for posting lists?
 *   Contiguous memory = cache-friendly iteration.
 *   For a query scanning a posting list of 10,000 docs,
 *   sequential vector access uses CPU prefetching efficiently.
 *   A linked list would thrash the cache.
 */
class InvertedIndex
{
public:
    InvertedIndex()  = default;
    ~InvertedIndex() = default;

    InvertedIndex(const InvertedIndex&)            = delete;
    InvertedIndex& operator=(const InvertedIndex&) = delete;

    InvertedIndex(InvertedIndex&&)            = default;
    InvertedIndex& operator=(InvertedIndex&&) = default;

    // ─────────────────────────────────────────
    // Indexing operations
    // ─────────────────────────────────────────

    /**
     * registerDocument — add a document to the document store.
     * Must be called before addPosting() for this document.
     *
     * @return the assigned DocumentID
     */
    DocumentID registerDocument(
        const std::string& path,
        const std::string& title,
        uint32_t           totalWords
    );

    /**
     * addPosting — record that 'word' appears in 'docId'.
     *
     * If the word already has a posting for this document,
     * increments termFreq instead of creating a duplicate entry.
     *
     * @param word      normalized word (lowercase, alpha only)
     * @param docId     document identifier
     * @param position  character offset of this occurrence
     */
    void addPosting(
        const std::string& word,
        DocumentID         docId,
        uint32_t           position = 0
    );

    // ─────────────────────────────────────────
    // Query operations
    // ─────────────────────────────────────────

    /**
     * getPostings — return all postings for a word.
     * Returns empty vector if word not found.
     *
     * Returns const reference — no copy.
     */
    const std::vector<Posting>& getPostings(const std::string& word) const;

    /**
     * documentFrequency — how many documents contain this word.
     * Used in IDF calculation.
     */
    std::size_t documentFrequency(const std::string& word) const;

    /**
     * getDocumentInfo — retrieve metadata for a document.
     * Returns nullptr if not found.
     */
    const DocumentInfo* getDocumentInfo(DocumentID docId) const;

    // ─────────────────────────────────────────
    // Statistics
    // ─────────────────────────────────────────

    std::size_t totalDocuments()  const;
    std::size_t totalUniqueWords() const;
    bool        empty()           const;

    /**
     * clear — remove all indexed data.
     */
    void clear();

private:
    std::unordered_map<std::string, std::vector<Posting>> m_index;
    std::unordered_map<DocumentID, DocumentInfo>          m_documents;
    DocumentID                                            m_nextDocId { 1 };

    // Empty posting list returned by reference for missing words.
    // Returning a reference to a local static is safe and avoids
    // the overhead of returning by value in the hot query path.
    static const std::vector<Posting> s_emptyPostings;
};

} // namespace search
} // namespace sl
