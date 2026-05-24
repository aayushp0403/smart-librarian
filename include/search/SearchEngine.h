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
 * A single document in a search result set.
 * Returned in descending order of score.
 */
struct SearchResult
{
    DocumentID  docId;
    std::string title;
    std::string path;
    double      score;      // TF-IDF relevance score
    uint32_t    matchCount; // how many query terms matched

    // Sort descending by score (for std::sort)
    bool operator>(const SearchResult& other) const {
        return score > other.score;
    }
};

/**
 * SearchEngine
 *
 * Orchestrates TextProcessor, Trie, and InvertedIndex
 * to provide indexing and ranked search.
 *
 * Owns both the Trie and InvertedIndex by composition.
 * (Composition over inheritance — IS-A doesn't apply here.)
 *
 * Indexing:
 *   indexDocument(path, title, content) tokenizes the content,
 *   inserts each token into the Trie (for prefix search),
 *   and builds posting lists in the InvertedIndex.
 *
 * Searching:
 *   search(query) tokenizes the query, looks up each term in
 *   the InvertedIndex, computes TF-IDF scores per document,
 *   and returns results sorted by descending score.
 *
 * prefixSearch(prefix) uses the Trie to find all indexed words
 *   starting with the given prefix — for autocomplete.
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

    // ─────────────────────────────────────────
    // Indexing
    // ─────────────────────────────────────────

    /**
     * indexDocument — index a document's text content.
     *
     * @param path     filesystem path or unique identifier
     * @param title    human-readable document name
     * @param content  full text content to index
     * @return         the assigned DocumentID
     */
    DocumentID indexDocument(
        const std::string& path,
        const std::string& title,
        const std::string& content
    );

    // ─────────────────────────────────────────
    // Searching
    // ─────────────────────────────────────────

    /**
     * search — ranked keyword search.
     *
     * @param query      raw query string (e.g., "neural network learning")
     * @param maxResults maximum results to return (0 = no limit)
     * @return           results sorted by descending TF-IDF score
     */
    std::vector<SearchResult> search(
        std::string_view query,
        std::size_t      maxResults = 10
    ) const;

    /**
     * prefixSearch — find all indexed words starting with prefix.
     * Used for autocomplete / search suggestions.
     *
     * @param prefix     partial word
     * @param maxResults cap on suggestions
     */
    std::vector<std::string> prefixSearch(
        std::string_view prefix,
        std::size_t      maxResults = 10
    ) const;

    // ─────────────────────────────────────────
    // Statistics
    // ─────────────────────────────────────────

    std::size_t documentCount()  const;
    std::size_t uniqueWordCount() const;
    std::size_t trieNodeCount()  const;

    void clear();

private:
    /**
     * computeTfIdf — TF-IDF score for one term in one document.
     *
     * TF  = termFreq / totalWordsInDoc
     * IDF = log( (1 + totalDocs) / (1 + docFreq) ) + 1
     *
     * We use the smoothed IDF formula (add 1 to numerator and
     * denominator) to prevent division by zero and reduce the
     * impact of very common words that aren't stop words.
     */
    double computeTfIdf(
        uint32_t   termFreq,
        uint32_t   totalWordsInDoc,
        std::size_t docFreq,
        std::size_t totalDocs
    ) const;

    Trie          m_trie;
    InvertedIndex m_index;
};

} // namespace search
} // namespace sl
