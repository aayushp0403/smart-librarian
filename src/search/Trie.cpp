#include "search/Trie.h"
#include <stdexcept>

namespace sl {
namespace search {

// ─────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────

Trie::Trie()
    : m_root(std::make_unique<TrieNode>())
    , m_wordCount(0)
    , m_nodeCount(1)  // count the root node
{
}

// ─────────────────────────────────────────────────────────────────────
// insert
//
// Walk the trie character by character.
// At each character, check if the child node exists.
// If not, create it (allocate a new TrieNode on the heap).
// At the end, mark isEndOfWord = true.
//
// Memory allocation pattern:
//   Each new TrieNode is heap-allocated via make_unique.
//   Its address is stored in the parent's unordered_map.
//   The unique_ptr owns the child. When the parent is destroyed,
//   the unique_ptr destructor frees the child, which recurses.
//
// Example: inserting "cat" into an empty trie:
//   root → create 'c' node
//   'c'  → create 'a' node
//   'a'  → create 't' node
//   mark 't' node as isEndOfWord = true
//   m_wordCount++, m_nodeCount += 3
// ─────────────────────────────────────────────────────────────────────

void Trie::insert(std::string_view word)
{
    if (word.empty()) return;

    TrieNode* current = m_root.get();
    bool newWordPath = false;

    for (char c : word) {
        auto it = current->children.find(c);
        if (it == current->children.end()) {
            // Child doesn't exist — create it
            // emplace returns {iterator, bool}
            auto [insertedIt, _] = current->children.emplace(
                c,
                std::make_unique<TrieNode>()
            );
            current = insertedIt->second.get();
            ++m_nodeCount;
            newWordPath = true;
        } else {
            current = it->second.get();
        }
    }

    if (!current->isEndOfWord) {
        current->isEndOfWord = true;
        ++m_wordCount;
    }

    // Track how many times this word has been inserted.
    // Useful for frequency-weighted autocomplete suggestions.
    ++current->insertCount;
}

// ─────────────────────────────────────────────────────────────────────
// contains
//
// Walk to the terminal node. If we fall off the trie at any point
// (child doesn't exist), return false.
// At the end, return isEndOfWord — because "car" and "card" share
// the path c→a→r, but only "card" has isEndOfWord at the 'd' node.
//
// "car" is a prefix of "card" but contains("car") should only
// return true if "car" was explicitly inserted.
// ─────────────────────────────────────────────────────────────────────

bool Trie::contains(std::string_view word) const
{
    const TrieNode* node = findNode(word);
    return node != nullptr && node->isEndOfWord;
}

// ─────────────────────────────────────────────────────────────────────
// startsWith
//
// Same traversal as contains, but we don't require isEndOfWord.
// If we can fully traverse the prefix without falling off, it exists.
// ─────────────────────────────────────────────────────────────────────

bool Trie::startsWith(std::string_view prefix) const
{
    return findNode(prefix) != nullptr;
}

// ─────────────────────────────────────────────────────────────────────
// allWithPrefix
//
// 1. Walk to the node representing the last char of prefix.
// 2. DFS from that node, collecting all complete words.
//
// collectWords builds the result strings by extending the prefix
// string as we go deeper, and recording it when isEndOfWord is true.
//
// Complexity: O(m) to reach prefix node + O(k * avg_word_length)
// to collect k results. Both are small in practice.
// ─────────────────────────────────────────────────────────────────────

std::vector<std::string> Trie::allWithPrefix(
    std::string_view prefix,
    std::size_t maxResults
) const
{
    std::vector<std::string> results;

    const TrieNode* node = findNode(prefix);
    if (!node) return results;  // prefix not in trie

    // The prefix itself may be a complete word
    std::string prefixStr(prefix);
    collectWords(node, prefixStr, results, maxResults);

    return results;
}

// ─────────────────────────────────────────────────────────────────────
// collectWords — DFS traversal
//
// We pass 'prefix' by value because each recursive call needs
// its own copy of the current path string.
//
// Alternative: pass by reference and undo the push_back after
// recursion (backtracking). That avoids copies but requires
// careful state management. For word lengths <= 30, copying
// is fine and simpler.
// ─────────────────────────────────────────────────────────────────────

void Trie::collectWords(
    const TrieNode*           node,
    const std::string&        prefix,
    std::vector<std::string>& results,
    std::size_t               maxResults
) const
{
    if (maxResults > 0 && results.size() >= maxResults) return;

    if (node->isEndOfWord) {
        results.push_back(prefix);
    }

    for (const auto& [ch, childPtr] : node->children) {
        if (maxResults > 0 && results.size() >= maxResults) break;
        collectWords(childPtr.get(), prefix + ch, results, maxResults);
    }
}

// ─────────────────────────────────────────────────────────────────────
// findNode (const and non-const versions)
//
// The two overloads exist for const-correctness.
// A const Trie should only return const TrieNode*.
// A non-const Trie can return mutable TrieNode*.
//
// We implement the const version and cast in the non-const version
// to avoid code duplication (the const_cast pattern):
//   Call const version → cast away const on result
// This is safe because we know the object is non-const.
// ─────────────────────────────────────────────────────────────────────

const TrieNode* Trie::findNode(std::string_view prefix) const
{
    const TrieNode* current = m_root.get();

    for (char c : prefix) {
        auto it = current->children.find(c);
        if (it == current->children.end()) {
            return nullptr;
        }
        current = it->second.get();
    }

    return current;
}

TrieNode* Trie::findNode(std::string_view prefix)
{
    // Const cast pattern:
    // Call the const version, then cast the result.
    // Avoids duplicating the traversal logic.
    return const_cast<TrieNode*>(
        static_cast<const Trie*>(this)->findNode(prefix)
    );
}

// ─────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────

std::size_t Trie::wordCount() const { return m_wordCount; }
std::size_t Trie::nodeCount() const { return m_nodeCount; }

void Trie::clear()
{
    m_root = std::make_unique<TrieNode>();
    m_wordCount = 0;
    m_nodeCount = 1;
}

} // namespace search
} // namespace sl
