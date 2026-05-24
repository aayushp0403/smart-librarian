#pragma once

#include "search/Trie.h"
#include "search/InvertedIndex.h"
#include "search/TextProcessor.h"

#include <string>
#include <vector>
#include <string_view>

namespace sl {
namespace search {

/**
 * SearchResult
 *
 * One document in a ranked result set.
 *
 * matchCount:     how many DISTINCT query terms appear in this document
 * totalTermFreq:  sum of how many times each matched term appears
 *                 e.g. query="neural network", doc has "neural" x4,
 *                 "network" x2 → totalTermFreq = 6
 * score:          TF-IDF relevance score (higher = more relevant)
 */
struct SearchResult
{
    DocumentID  docId         { 0 };
    std::string title;
    std::string path;
    double      score         { 0.0 };
    uint32_t    matchCount    { 0 };   // distinct query terms matched
    uint32_t    totalTermFreq { 0 };   // total occurrences across matched terms

    bool operator>(const SearchResult& other) const {
        return score > other.score;
    }
};

/**
 * SearchEngine
 *
 * Orchestrates TextProcessor, Trie, and InvertedIndex
 * to provide indexing and ranked full-text search.
 */
class SearchEngine
{
public:
    SearchEngine()  = default;
    ~SearchEngine() = default;

    SearchEngine(const SearchEngine&)            = delete;
    SearchEngine& operator=(const SearchEngine&) = delete;

    SearchEngine(SearchEngine&&)            = default;
    SearchEngine& operator=(SearchEngine&&) = default;

    // ── Indexing ──────────────────────────────────────────────────
    DocumentID indexDocument(
        const std::string& path,
        const std::string& title,
        const std::string& content
    );

    // ── Searching ─────────────────────────────────────────────────
    std::vector<SearchResult> search(
        std::string_view query,
        std::size_t      maxResults = 10
    ) const;

    std::vector<std::string> prefixSearch(
        std::string_view prefix,
        std::size_t      maxResults = 10
    ) const;

    // ── Stats ─────────────────────────────────────────────────────
    std::size_t documentCount()   const;
    std::size_t uniqueWordCount() const;
    std::size_t trieNodeCount()   const;

    void clear();

private:
    double computeTfIdf(
        uint32_t    termFreq,
        uint32_t    totalWordsInDoc,
        std::size_t docFreq,
        std::size_t totalDocs
    ) const;

    Trie          m_trie;
    InvertedIndex m_index;
};

} // namespace search
} // namespace sl
