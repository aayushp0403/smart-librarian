#include "search/SearchEngine.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace sl {
namespace search {

// ─────────────────────────────────────────────────────────────────────
// indexDocument
// ─────────────────────────────────────────────────────────────────────

DocumentID SearchEngine::indexDocument(
    const std::string& path,
    const std::string& title,
    const std::string& content
)
{
    std::vector<std::string> tokens = TextProcessor::tokenize(content);

    DocumentID docId = m_index.registerDocument(
        path,
        title,
        static_cast<uint32_t>(tokens.size())
    );

    for (uint32_t pos = 0; pos < static_cast<uint32_t>(tokens.size()); ++pos) {
        const std::string& token = tokens[pos];
        m_trie.insert(token);
        m_index.addPosting(token, docId, pos);
    }

    return docId;
}

// ─────────────────────────────────────────────────────────────────────
// search
//
// Accumulator now also sums totalTermFreq so the UI can display
// "appeared X times" rather than just the raw TF-IDF score.
// ─────────────────────────────────────────────────────────────────────

std::vector<SearchResult> SearchEngine::search(
    std::string_view query,
    std::size_t      maxResults
) const
{
    std::vector<std::string> queryTerms =
        TextProcessor::tokenizeQuery(query);

    if (queryTerms.empty()) return {};

    std::size_t totalDocs = m_index.totalDocuments();
    if (totalDocs == 0) return {};

    // Accumulator map: DocumentID → SearchResult
    std::unordered_map<DocumentID, SearchResult> scores;
    scores.reserve(256);

    for (const std::string& term : queryTerms) {
        std::string normalized = TextProcessor::normalize(term);
        if (normalized.empty()) continue;

        const std::vector<Posting>& postings =
            m_index.getPostings(normalized);
        if (postings.empty()) continue;

        std::size_t docFreq = postings.size();

        for (const Posting& posting : postings) {
            const DocumentInfo* docInfo =
                m_index.getDocumentInfo(posting.docId);
            if (!docInfo) continue;

            double tfidf = computeTfIdf(
                posting.termFreq,
                docInfo->totalWords,
                docFreq,
                totalDocs
            );

            auto& result = scores[posting.docId];

            // First time we see this document — initialize it
            if (result.docId == 0) {
                result.docId         = posting.docId;
                result.title         = docInfo->title;
                result.path          = docInfo->path;
                result.score         = 0.0;
                result.matchCount    = 0;
                result.totalTermFreq = 0;
            }

            result.score         += tfidf;
            result.matchCount    += 1;
            result.totalTermFreq += posting.termFreq;  // ← accumulate raw frequency
        }
    }

    // Collect, sort descending by score, trim
    std::vector<SearchResult> results;
    results.reserve(scores.size());
    for (auto& [id, r] : scores) {
        results.push_back(std::move(r));
    }

    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  return a.score > b.score;
              });

    if (maxResults > 0 && results.size() > maxResults) {
        results.resize(maxResults);
    }

    return results;
}

// ─────────────────────────────────────────────────────────────────────
// prefixSearch
// ─────────────────────────────────────────────────────────────────────

std::vector<std::string> SearchEngine::prefixSearch(
    std::string_view prefix,
    std::size_t      maxResults
) const
{
    std::string normalized = TextProcessor::normalize(prefix);
    if (normalized.empty()) return {};
    return m_trie.allWithPrefix(normalized, maxResults);
}

// ─────────────────────────────────────────────────────────────────────
// computeTfIdf — smoothed formula
// ─────────────────────────────────────────────────────────────────────

double SearchEngine::computeTfIdf(
    uint32_t    termFreq,
    uint32_t    totalWordsInDoc,
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

std::size_t SearchEngine::documentCount()   const {
    return m_index.totalDocuments();
}
std::size_t SearchEngine::uniqueWordCount() const {
    return m_index.totalUniqueWords();
}
std::size_t SearchEngine::trieNodeCount()   const {
    return m_trie.nodeCount();
}

void SearchEngine::clear()
{
    m_trie.clear();
    m_index.clear();
}

} // namespace search
} // namespace sl
