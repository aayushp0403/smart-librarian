#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cstddef>

namespace sl {
namespace search {

using DocumentID = uint32_t;

struct Posting
{
    DocumentID docId         { 0 };
    uint32_t   termFreq      { 0 };
    uint32_t   firstPosition { 0 };
};

struct DocumentInfo
{
    DocumentID  id;
    std::string path;
    std::string title;
    uint32_t    totalWords;
};

class InvertedIndex
{
public:
    InvertedIndex()  = default;
    ~InvertedIndex() = default;

    InvertedIndex(const InvertedIndex&)            = delete;
    InvertedIndex& operator=(const InvertedIndex&) = delete;
    InvertedIndex(InvertedIndex&&)                 = default;
    InvertedIndex& operator=(InvertedIndex&&)      = default;

    // ── Normal indexing ───────────────────────────────────────────
    DocumentID registerDocument(const std::string& path,
                                const std::string& title,
                                uint32_t           totalWords);

    void addPosting(const std::string& word,
                    DocumentID         docId,
                    uint32_t           position = 0);

    // ── Query ─────────────────────────────────────────────────────
    const std::vector<Posting>& getPostings(const std::string& word) const;
    std::size_t documentFrequency(const std::string& word) const;
    const DocumentInfo* getDocumentInfo(DocumentID docId) const;

    std::size_t totalDocuments()   const;
    std::size_t totalUniqueWords() const;
    bool        empty()            const;
    void        clear();

    // ── Persistence accessors (used by PersistenceManager only) ──

    /**
     * getAllDocuments — read-only view of the document map.
     * PersistenceManager uses this to iterate and save all docs.
     */
    const std::unordered_map<DocumentID, DocumentInfo>&
        getAllDocuments() const { return m_documents; }

    /**
     * getAllPostings — read-only view of the entire inverted index.
     * PersistenceManager iterates this to save all posting lists.
     */
    const std::unordered_map<std::string, std::vector<Posting>>&
        getAllPostings() const { return m_index; }

    /**
     * registerDocumentWithId — restore a document with a known ID.
     * Used during deserialization to preserve original IDs.
     * Normal code always uses registerDocument() instead.
     */
    void registerDocumentWithId(DocumentID         id,
                                const std::string& path,
                                const std::string& title,
                                uint32_t           totalWords);

    /**
     * addPostingDirect — restore a posting with exact values.
     * Used during deserialization. Bypasses the duplicate-check
     * logic in addPosting() since we know the data is clean.
     */
    void addPostingDirect(const std::string& word,
                          DocumentID         docId,
                          uint32_t           termFreq,
                          uint32_t           firstPosition);

    /**
     * getOrCreateNextDocId — after deserialization, ensures
     * m_nextDocId is beyond all loaded IDs so new documents
     * get unique IDs that don't collide with restored ones.
     */
    void syncNextDocId();

private:
    std::unordered_map<std::string, std::vector<Posting>> m_index;
    std::unordered_map<DocumentID, DocumentInfo>          m_documents;
    DocumentID                                            m_nextDocId { 1 };

    static const std::vector<Posting> s_emptyPostings;
};

} // namespace search
} // namespace sl
