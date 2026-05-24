#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace sl {
namespace search {

/**
 * TrieNode
 *
 * A single node in the prefix tree.
 *
 * Memory design choice — unordered_map<char, unique_ptr<TrieNode>>:
 *
 *   Option A: children[26] (fixed array)
 *     + O(1) child access (index directly by char - 'a')
 *     - 26 * 8 bytes = 208 bytes per node regardless of actual children
 *     - For sparse tries (most tries are sparse), this wastes memory
 *
 *   Option B: unordered_map<char, unique_ptr<TrieNode>> (chosen)
 *     + Only allocates memory for actual children
 *     + Works with any character set (not just a-z)
 *     - O(1) amortized child access (hash lookup)
 *     - Slightly higher memory overhead per child (hash table metadata)
 *
 *   For a document search engine, words contain only 26 lowercase chars.
 *   Most nodes have 1-3 children. Option B saves significant memory.
 *
 * m_count: how many times this word was inserted.
 *   Useful for frequency-weighted suggestions.
 *   0 means this node is a prefix but not a complete word.
 */
struct TrieNode
{
    std::unordered_map<char, std::unique_ptr<TrieNode>> children;
    bool     isEndOfWord { false };
    uint32_t insertCount { 0 };     // how many times this word was inserted

    TrieNode() = default;

    // Non-copyable (owns child nodes via unique_ptr)
    TrieNode(const TrieNode&)            = delete;
    TrieNode& operator=(const TrieNode&) = delete;

    // Movable
    TrieNode(TrieNode&&)            = default;
    TrieNode& operator=(TrieNode&&) = default;
};

/**
 * Trie
 *
 * A prefix tree for fast word lookup and prefix-based autocomplete.
 *
 * Complexities:
 *   insert(word):      O(m)  where m = word length
 *   contains(word):    O(m)
 *   startsWith(prefix):O(m)
 *   allWithPrefix():   O(m + k) where k = number of matching words
 *
 * All operations are independent of the number of stored words (n).
 * This is the defining property that makes Tries useful.
 *
 * Ownership:
 *   The root TrieNode owns all child nodes via unique_ptr chains.
 *   Destroying the Trie (or its root) recursively destroys
 *   the entire tree — RAII in a tree structure.
 *
 *   Be aware: for a very deep trie with millions of nodes,
 *   recursive destruction via unique_ptr could overflow the stack.
 *   For our use case (English words, max ~30 chars deep), this is safe.
 */
class Trie
{
public:
    Trie();
    ~Trie() = default;

    Trie(const Trie&)            = delete;
    Trie& operator=(const Trie&) = delete;

    Trie(Trie&&)            = default;
    Trie& operator=(Trie&&) = default;

    /**
     * insert — add a word to the trie.
     * If the word already exists, increments its count.
     *
     * @param word  must be normalized (lowercase, alpha only)
     */
    void insert(std::string_view word);

    /**
     * contains — returns true if this exact word was inserted.
     * (isEndOfWord must be true at the terminal node)
     */
    bool contains(std::string_view word) const;

    /**
     * startsWith — returns true if any inserted word begins with prefix.
     * The prefix itself need not be a complete word.
     */
    bool startsWith(std::string_view prefix) const;

    /**
     * allWithPrefix — returns all inserted words beginning with prefix.
     * Results are unsorted (DFS traversal order).
     *
     * @param prefix    search prefix
     * @param maxResults cap results (0 = no limit)
     */
    std::vector<std::string> allWithPrefix(
        std::string_view prefix,
        std::size_t maxResults = 20
    ) const;

    /**
     * wordCount — total unique words stored.
     */
    std::size_t wordCount() const;

    /**
     * nodeCount — total nodes allocated (memory usage indicator).
     */
    std::size_t nodeCount() const;

    /**
     * clear — remove all words.
     */
    void clear();

private:
    /**
     * findNode — traverse to the node representing the last
     * character of 'prefix'. Returns nullptr if path doesn't exist.
     */
    const TrieNode* findNode(std::string_view prefix) const;
    TrieNode*       findNode(std::string_view prefix);

    /**
     * collectWords — DFS from 'node', collecting complete words
     * into 'results'. 'prefix' is the string path that led to 'node'.
     */
    void collectWords(
        const TrieNode*       node,
        const std::string&    prefix,
        std::vector<std::string>& results,
        std::size_t           maxResults
    ) const;

    std::unique_ptr<TrieNode> m_root;
    std::size_t               m_wordCount { 0 };
    std::size_t               m_nodeCount { 0 };
};

} // namespace search
} // namespace sl
