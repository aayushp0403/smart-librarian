#pragma once

#include "search/Trie.h"
#include "search/InvertedIndex.h"
#include "search/TextProcessor.h"

#include <string>
#include <vector>
#include <string_view>

namespace sl {
namespace search {

struct SearchResult
{
    DocumentID  docId         { 0 };
    std::string title;
    std::string path;
    double      score         { 0.0 };
    uint32_t    matchCount    { 0 };
    uint32_t    totalTermFreq { 0 };

    bool operator>(const SearchResult& other) const {
        return score > other.score;
    }
};

class SearchEngine
{
public:
    SearchEngine()  = default;
    ~SearchEngine() = default;

    SearchEngine(const SearchEngine&)            = delete;
    SearchEngine& operator=(const SearchEngine&) = delete;
    SearchEngine(SearchEngine&&)                 = default;
    SearchEngine& operator=(SearchEngine&&)      = default;

    // ── Indexing ──────────────────────────────────────────────────
    DocumentID indexDocument(const std::string& path,
                             const std::string& title,
                             const std::string& content);

    // ── Search ────────────────────────────────────────────────────
    std::vector<SearchResult> search(std::string_view query,
                                     std::size_t maxResults = 10) const;

    std::vector<std::string> prefixSearch(std::string_view prefix,
                                          std::size_t maxResults = 10) const;

    // ── Stats ─────────────────────────────────────────────────────
    std::size_t documentCount()   const;
    std::size_t uniqueWordCount() const;
    std::size_t trieNodeCount()   const;
    void        clear();

    // ── Persistence accessors (PersistenceManager only) ───────────

    /**
     * getInvertedIndex — expose the index for serialization.
     * const version used by saveIndex.
     * non-const used by loadIndex (via const_cast in PersistenceManager).
     */
    const InvertedIndex& getInvertedIndex() const { return m_index; }
          InvertedIndex& getInvertedIndex()       { return m_index; }

    /**
     * insertWordIntoTrie — re-populate trie during deserialization.
     * During normal indexDocument(), words go into the trie automatically.
     * During load, we restore words directly into the inverted index
     * and must separately rebuild the trie.
     */
    void insertWordIntoTrie(const std::string& word) {
        m_trie.insert(word);
    }

private:
    double computeTfIdf(uint32_t termFreq,
                        uint32_t totalWordsInDoc,
                        std::size_t docFreq,
                        std::size_t totalDocs) const;

    Trie          m_trie;
    InvertedIndex m_index;
};

} // namespace search
} // namespace sl
