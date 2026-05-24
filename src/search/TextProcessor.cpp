
#include "search/TextProcessor.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace sl {
namespace search {

// ─────────────────────────────────────────────────────────────────────
// Stop words — English function words with little discriminative value.
//
// Initialized once as a static const member.
// std::unordered_set stores strings in a hash table.
// Lookup is O(1) average — checking "the" costs one hash + comparison.
//
// Memory: each string in an unordered_set is heap-allocated.
// With C++17 string deduplication and short-string optimization (SSO),
// strings <= 15 chars (compiler-dependent) live in-place in the
// std::string object itself, no heap allocation.
// Most of these stop words are <= 15 chars, so SSO applies.
// ─────────────────────────────────────────────────────────────────────

const std::unordered_set<std::string> TextProcessor::s_stopWords = {
    "a", "an", "the", "and", "or", "but", "in", "on", "at", "to",
    "for", "of", "with", "by", "from", "as", "is", "was", "are",
    "were", "be", "been", "being", "have", "has", "had", "do", "does",
    "did", "will", "would", "could", "should", "may", "might", "shall",
    "can", "not", "no", "nor", "so", "yet", "both", "either", "neither",
    "it", "its", "this", "that", "these", "those", "i", "we", "you",
    "he", "she", "they", "me", "us", "him", "her", "them", "my", "our",
    "your", "his", "their", "what", "which", "who", "when", "where",
    "why", "how", "all", "each", "every", "any", "some", "such", "into",
    "than", "then", "up", "out", "if", "about", "also", "just"
};

// ─────────────────────────────────────────────────────────────────────
// normalize
//
// Two operations:
//   1. Lowercase: 'A'-'Z' → 'a'-'z' via std::tolower
//   2. Keep only alpha characters (remove punctuation, digits)
//
// Why char-by-char instead of a regex?
//   std::regex is heavy — it compiles a regex automaton on construction.
//   For a function called millions of times during indexing, a simple
//   loop over characters with std::tolower and std::isalpha is an
//   order of magnitude faster.
//
// std::tolower takes int (for historical C reasons) and returns int.
// We cast carefully: char → unsigned char → int → tolower → char.
// Passing a negative char (high-bit set) directly to tolower is UB.
// ─────────────────────────────────────────────────────────────────────

std::string TextProcessor::normalize(std::string_view token)
{
    std::string result;
    result.reserve(token.size());

    for (char c : token) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            result += static_cast<char>(
                std::tolower(static_cast<unsigned char>(c))
            );
        }
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────
// isStopWord
// ─────────────────────────────────────────────────────────────────────

bool TextProcessor::isStopWord(std::string_view word)
{
    // unordered_set::count() returns 0 or 1.
    // We can't use std::string_view directly as the key for an
    // unordered_set<std::string> lookup in C++17 without a
    // heterogeneous hash. Constructing a temporary std::string here
    // is the safe approach. C++20 would allow transparent hashing.
    return s_stopWords.count(std::string(word)) > 0;
}

// ─────────────────────────────────────────────────────────────────────
// tokenize
//
// Algorithm:
//   Walk the input character by character.
//   Accumulate alpha characters into the current token.
//   When a non-alpha character is encountered, if current token
//   is non-empty, normalize and check it. If it passes minLength
//   and is not a stop word, add to results. Reset token.
//
// Why not std::istringstream >> word?
//   The stream splits on whitespace only. "hello,world" would not
//   be split. We want to split on ANY non-alpha character.
//
// Complexity: O(n) where n = input length.
//   We visit each character exactly once.
// ─────────────────────────────────────────────────────────────────────

std::vector<std::string> TextProcessor::tokenize(
    std::string_view text,
    std::size_t minLength
)
{
    std::vector<std::string> tokens;
    tokens.reserve(text.size() / 5);  // rough estimate: avg word ~5 chars

    std::string current;
    current.reserve(32);  // pre-allocate for typical word lengths

    auto flushToken = [&]() {
        if (current.empty()) return;

        std::string normalized = normalize(current);
        current.clear();

        if (normalized.size() < minLength) return;
        if (isStopWord(normalized)) return;

        tokens.push_back(std::move(normalized));
    };

    for (char c : text) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            current += c;
        } else {
            flushToken();
        }
    }
    flushToken();  // handle token at end of input

    return tokens;
}

// ─────────────────────────────────────────────────────────────────────
// tokenizeQuery
//
// Same as tokenize but:
//   - Does NOT remove stop words (user intent should be preserved)
//   - Still normalizes (lowercase, alpha-only)
//
// Why keep stop words in queries?
//   A user searching for "to be or not to be" should find relevant
//   documents. Removing all words would yield an empty query.
//   For a simple keyword search this matters less, but for phrase
//   search it is critical.
// ─────────────────────────────────────────────────────────────────────

std::vector<std::string> TextProcessor::tokenizeQuery(std::string_view query)
{
    std::vector<std::string> tokens;

    std::string current;
    current.reserve(32);

    auto flushToken = [&]() {
        if (current.empty()) return;
        std::string normalized = normalize(current);
        current.clear();
        if (normalized.empty()) return;
        tokens.push_back(std::move(normalized));
    };

    for (char c : query) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            current += c;
        } else {
            flushToken();
        }
    }
    flushToken();

    return tokens;
}

} // namespace search
} // namespace sl
