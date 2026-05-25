#include "search/InvertedIndex.h"
#include <algorithm>
#include <stdexcept>

namespace sl {
namespace search {

// Static member definition
const std::vector<Posting> InvertedIndex::s_emptyPostings;

// ─────────────────────────────────────────────────────────────────────
// registerDocument
// ─────────────────────────────────────────────────────────────────────

DocumentID InvertedIndex::registerDocument(
    const std::string& path,
    const std::string& title,
    uint32_t           totalWords
)
{
    DocumentID id = m_nextDocId++;

    m_documents.emplace(id, DocumentInfo{
        id,
        path,
        title,
        totalWords
    });

    return id;
}

// ─────────────────────────────────────────────────────────────────────
// addPosting
//
// Two cases:
//
// Case 1: Word not yet in the index.
//   operator[] on unordered_map default-constructs the value
//   (empty vector) and returns a reference to it.
//   We then push_back a new Posting.
//
// Case 2: Word exists, but no posting for this docId yet.
//   Linear scan of the posting list for this word.
//   Find if docId already has an entry. If yes, increment freq.
//   If no, push_back a new Posting.
//
// Why linear scan instead of a second hash map (word → docId → freq)?
//   For typical documents, posting lists are short (< 1000 entries).
//   Linear scan of a contiguous vector is extremely cache-friendly.
//   A nested hash map would require two hash lookups and poor locality.
//   Profile before optimizing — premature complexity is the enemy.
//
// Complexity: O(k) where k = current size of word's posting list.
//   For most words, k is small. For very common words (after stop
//   word removal), k might be large — but those words are not indexed.
// ─────────────────────────────────────────────────────────────────────

void InvertedIndex::addPosting(
    const std::string& word,
    DocumentID         docId,
    uint32_t           position
)
{
    if (word.empty()) return;

    std::vector<Posting>& postingList = m_index[word];

    // Search for existing posting for this document
    for (Posting& posting : postingList) {
        if (posting.docId == docId) {
            // Already have a posting for this doc — just increment freq
            ++posting.termFreq;
            return;
        }
    }

    // No existing posting — add new entry
    postingList.push_back(Posting{
        docId,
        1,         // first occurrence: termFreq = 1
        position   // character offset of first occurrence
    });
}

// ─────────────────────────────────────────────────────────────────────
// getPostings
//
// Returns const reference to the posting list for 'word'.
// If word not found, returns reference to the static empty vector.
//
// Why return by const reference and not by value?
//   Posting lists can be large. Returning by value copies the entire
//   vector. The caller only needs to read the list. Const reference
//   is zero-copy and communicates that ownership stays with the index.
// ─────────────────────────────────────────────────────────────────────

const std::vector<Posting>& InvertedIndex::getPostings(
    const std::string& word
) const
{
    auto it = m_index.find(word);
    if (it == m_index.end()) {
        return s_emptyPostings;
    }
    return it->second;
}

// ─────────────────────────────────────────────────────────────────────
// documentFrequency
// ─────────────────────────────────────────────────────────────────────

std::size_t InvertedIndex::documentFrequency(const std::string& word) const
{
    auto it = m_index.find(word);
    if (it == m_index.end()) return 0;
    return it->second.size();
}

// ─────────────────────────────────────────────────────────────────────
// getDocumentInfo
// ─────────────────────────────────────────────────────────────────────

const DocumentInfo* InvertedIndex::getDocumentInfo(DocumentID docId) const
{
    auto it = m_documents.find(docId);
    if (it == m_documents.end()) return nullptr;
    return &it->second;
}

// ─────────────────────────────────────────────────────────────────────
// Statistics
// ─────────────────────────────────────────────────────────────────────

std::size_t InvertedIndex::totalDocuments()  const { return m_documents.size(); }
std::size_t InvertedIndex::totalUniqueWords() const { return m_index.size(); }
bool        InvertedIndex::empty()            const { return m_documents.empty(); }

void InvertedIndex::clear()
{
    m_index.clear();
    m_documents.clear();
    m_nextDocId = 1;
}



// ─────────────────────────────────────────────────────────────────────
// Persistence support methods
// ─────────────────────────────────────────────────────────────────────

void InvertedIndex::registerDocumentWithId(
    DocumentID         id,
    const std::string& path,
    const std::string& title,
    uint32_t           totalWords)
{
    m_documents.emplace(id, DocumentInfo{ id, path, title, totalWords });
    // Track highest ID seen so syncNextDocId works correctly
    if (id >= m_nextDocId) {
        m_nextDocId = id + 1;
    }
}

void InvertedIndex::addPostingDirect(
    const std::string& word,
    DocumentID         docId,
    uint32_t           termFreq,
    uint32_t           firstPosition)
{
    m_index[word].push_back(Posting{ docId, termFreq, firstPosition });
}

void InvertedIndex::syncNextDocId()
{
    DocumentID maxId = 0;
    for (const auto& [id, _] : m_documents) {
        if (id > maxId) maxId = id;
    }
    m_nextDocId = maxId + 1;
}
} // namespace search
} // namespace sl