#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <string_view>

namespace sl {
namespace search {

/**
 * TextProcessor
 *
 * Converts raw document text into normalized tokens
 * suitable for indexing and search.
 *
 * Pipeline:
 *   raw text
 *     → lowercase
 *     → split on non-alpha characters
 *     → remove stop words ("the", "a", "is", ...)
 *     → remove tokens shorter than minLength
 *     → return token list
 *
 * Why normalize?
 *   Without normalization, "Algorithm", "algorithm", and
 *   "ALGORITHM" would be three separate index entries.
 *   We want them treated as the same word.
 *
 * Why remove stop words?
 *   "the" appears in virtually every document. Its IDF ≈ 0.
 *   It adds noise to rankings and wastes index space.
 *   Removing it before indexing is cleaner than scoring it away.
 *
 * Design choice — static interface:
 *   These are stateless operations. A static interface avoids
 *   needing to construct a TextProcessor object just to call
 *   tokenize(). Compare to Java where everything must be an
 *   object — C++ gives us the choice.
 */
class TextProcessor
{
public:
    /**
     * tokenize — split text into normalized tokens.
     *
     * @param text      raw input string
     * @param minLength tokens shorter than this are discarded
     * @return          vector of normalized tokens (no duplicates removed)
     */
    static std::vector<std::string> tokenize(
        std::string_view text,
        std::size_t minLength = 2
    );

    /**
     * normalize — lowercase and strip non-alpha characters from a token.
     */
    static std::string normalize(std::string_view token);

    /**
     * isStopWord — returns true if the word should be excluded from indexing.
     */
    static bool isStopWord(std::string_view word);

    /**
     * tokenizeQuery — same as tokenize but preserves order and
     * does NOT remove stop words (so "to be or not to be" still works
     * as a phrase query in the future).
     */
    static std::vector<std::string> tokenizeQuery(std::string_view query);

private:
    // Static stop word set — initialized once, shared across all calls.
    // unordered_set gives O(1) average lookup.
    static const std::unordered_set<std::string> s_stopWords;
};

} // namespace search
} // namespace sl
