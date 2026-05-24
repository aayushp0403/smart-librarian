#include "search/SearchEngine.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace sl {
namespace search {

// ─────────────────────────────────────────────────────────────────────
// indexDocument
//
// Full pipeline:
//   1. Tokenize and normalize the content
//   2. Register the document in the InvertedIndex (get DocID)
//   3. For each token:
//      a. Insert into Trie (for prefix search)
//      b. Add posting to InvertedIndex
//
// Why register BEFORE tokenizing?
//   We need the document ID to create postings.
//   But we need the total word count for the DocumentInfo.
//   Solution: tokenize first (to count words), then register.
// ─────────────────────────────────────────────────────────────────────

DocumentID SearchEngine::indexDocument(
    const std::string& path,
    const std::string& title,
    const std::string& content
)
{
    // Step 1: tokenize
    std::vector<std::string> tokens = TextProcessor::tokenize(content);

    // Step 2: register document with total word count
    DocumentID docId = m_index.registerDocument(
        path,
        title,
        static_cast<uint32_t>(tokens.size())
    );

    // Step 3: build index
    // Track position (token index, not character offset — simpler)
    for (uint32_t pos = 0; pos < static_cast<uint32_t>(tokens.size()); ++pos)
    {
        const std::string& token = tokens[pos];

        // Insert into Trie (idempotent — re-inserting increments count)
        m_trie.insert(token);

        // Add posting to inverted index
        m_index.addPosting(token, docId, pos);
    }

    return docId;
}

// ─────────────────────────────────────────────────────────────────────
// search
//
// Multi-term ranked search algorithm:
//
//   1. Tokenize the query into individual terms
//   2. For each term, retrieve its posting list
//   3. Accumulate TF-IDF scores per document across all terms
//   4. Sort results by descending score
//   5. Return top maxResults
//
// Data structure for score accumulation:
//   unordered_map<DocumentID, SearchResult>
//   Key: document ID
//   Value: accumulator for that document's score and match count
//
//   Why unordered_map here?
//     We process one term at a time. For each term's posting list,
//     we update scores for potentially thousands of documents.
//     Hash map gives O(1) insert/update per document per term.
//
// TF-IDF computation:
//   For each (term, document) pair:
//     TF  = posting.termFreq / docInfo.totalWords
//     IDF = log((1 + N) / (1 + df)) + 1  (smoothed)
//     contribution = TF * IDF
//   Document score = sum of contributions across all matching terms
// ─────────────────────────────────────────────────────────────────────

std::vector<SearchResult> SearchEngine::search(
    std::string_view query,
    std::size_t      maxResults
) const
{
    // Step 1: tokenize query (preserve stop words in query)
    std::vector<std::string> queryTerms = TextProcessor::tokenizeQuery(query);

    if (queryTerms.empty()) return {};

    // Step 2: accumulate scores
    // DocumentID → SearchResult accumulator
    std::unordered_map<DocumentID, SearchResult> scores;
    scores.reserve(256);  // rough pre-allocation

    std::size_t totalDocs = m_index.totalDocuments();
    if (totalDocs == 0) return {};

    for (const std::string& term : queryTerms) {
        // Normalize the query term the same way we normalized during indexing
        std::string normalizedTerm = TextProcessor::normalize(term);
        if (normalizedTerm.empty()) continue;

        const std::vector<Posting>& postings = m_index.getPostings(normalizedTerm);
        if (postings.empty()) continue;

        std::size_t docFreq = postings.size();

        for (const Posting& posting : postings) {
            const DocumentInfo* docInfo = m_index.getDocumentInfo(posting.docId);
            if (!docInfo) continue;

            double tfidf = computeTfIdf(
                posting.termFreq,
                docInfo->totalWords,
                docFreq,
                totalDocs
            );

            // Accumulate into the scores map
            auto& result = scores[posting.docId];
            if (result.docId == 0) {
                // First time we see this document — initialize it
                result.docId      = posting.docId;
                result.title      = docInfo->title;
                result.path       = docInfo->path;
                result.score      = 0.0;
                result.matchCount = 0;
            }

            result.score      += tfidf;
            result.matchCount += 1;
        }
    }

    // Step 3: collect into vector and sort
    std::vector<SearchResult> results;
    results.reserve(scores.size());

    for (auto& [docId, result] : scores) {
        results.push_back(std::move(result));
    }

    // Sort descending by score
    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  return a.score > b.score;
              });

    // Step 4: trim to maxResults
    if (maxResults > 0 && results.size() > maxResults) {
        results.resize(maxResults);
    }

    return results;
}

// ─────────────────────────────────────────────────────────────────────
// prefixSearch
//
// Delegate to the Trie's allWithPrefix().
// The Trie handles the traversal; we just normalize the prefix.
// ─────────────────────────────────────────────────────────────────────

std::vector<std::string> SearchEngine::prefixSearch(
    std::string_view prefix,
    std::size_t      maxResults
) const
{
    std::string normalizedPrefix = TextProcessor::normalize(prefix);
    if (normalizedPrefix.empty()) return {};
    return m_trie.allWithPrefix(normalizedPrefix, maxResults);
}

// ─────────────────────────────────────────────────────────────────────
// computeTfIdf
//
// Smoothed TF-IDF formula:
//
//   TF  = termFreq / totalWordsInDoc
//        (proportion of this doc that is this term)
//
//   IDF = log( (1 + totalDocs) / (1 + docFreq) ) + 1
//        The +1 additions prevent:
//          - Division by zero (if docFreq = 0, impossible here but safe)
//          - Negative IDF (if docFreq = totalDocs, raw formula gives 0)
//        The trailing +1 ensures IDF is always >= 1 (never zero score
//        for a matched term).
//
//   score = TF * IDF
//
// We use std::log (natural log). Some implementations use log2 or
// log10 — the base only changes the scale, not the ranking order.
// ─────────────────────────────────────────────────────────────────────

double SearchEngine::computeTfIdf(
    uint32_t   termFreq,
    uint32_t   totalWordsInDoc,
    std::size_t docFreq,
    std::size_t totalDocs
) const
{
    if (totalWordsInDoc == 0) return 0.0;

    double tf = static_cast<double>(termFreq) /
                static_cast<double>(totalWordsInDoc);

    double idf = std::log(
        (1.0 + static_cast<double>(totalDocs)) /
        (1.0 + static_cast<double>(docFreq))
    ) + 1.0;

    return tf * idf;
}

// ─────────────────────────────────────────────────────────────────────
// Statistics
// ─────────────────────────────────────────────────────────────────────

std::size_t SearchEngine::documentCount()  const { return m_index.totalDocuments(); }
std::size_t SearchEngine::uniqueWordCount() const { return m_index.totalUniqueWords(); }
std::size_t SearchEngine::trieNodeCount()  const { return m_trie.nodeCount(); }

void SearchEngine::clear()
{
    m_trie.clear();
    m_index.clear();
}

} // namespace search
} // namespace sl
