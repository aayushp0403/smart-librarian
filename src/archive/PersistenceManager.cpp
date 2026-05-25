#include "archive/PersistenceManager.h"
#include "archive/BinaryWriter.h"
#include "archive/BinaryReader.h"
#include "search/SearchEngine.h"
#include "search/InvertedIndex.h"
#include "utils/Logger.h"

#include <cstdlib>
#include <stdexcept>
#include <cstring>

namespace sl {
namespace archive {

// ─────────────────────────────────────────────────────────────────────
// Constructor
//
// std::filesystem::path handles platform path differences
// (/ vs \ separators, path joining, existence checks).
//
// getenv("HOME") returns the user's home directory on Linux.
// We append ".smartlibrarian" to get ~/.smartlibrarian/.
//
// create_directories creates the full path recursively —
// like 'mkdir -p'. If it already exists, it does nothing.
// ─────────────────────────────────────────────────────────────────────

PersistenceManager::PersistenceManager(const std::string& dataDir)
{
    if (dataDir.empty()) {
        const char* home = std::getenv("HOME");
        if (!home) home = "/tmp";
        m_dataDir = std::filesystem::path(home) / ".smartlibrarian";
    } else {
        m_dataDir = std::filesystem::path(dataDir);
    }

    // Create the directory if it doesn't exist
    std::error_code ec;
    std::filesystem::create_directories(m_dataDir, ec);
    if (ec) {
        utils::Logger::warning(
            "PersistenceManager: could not create data dir: " +
            ec.message() + " — using /tmp");
        m_dataDir = "/tmp/.smartlibrarian";
        std::filesystem::create_directories(m_dataDir, ec);
    }

    utils::Logger::info(
        "PersistenceManager: data directory: " +
        m_dataDir.string());
}

// ─────────────────────────────────────────────────────────────────────
// Path helpers
// ─────────────────────────────────────────────────────────────────────

std::string PersistenceManager::dataDirectory() const {
    return m_dataDir.string();
}
std::string PersistenceManager::configPath() const {
    return (m_dataDir / "config.slcfg").string();
}
std::string PersistenceManager::indexPath() const {
    return (m_dataDir / "index.slidx").string();
}
std::string PersistenceManager::archivePath() const {
    return (m_dataDir / "archive.slarc").string();
}

bool PersistenceManager::indexExists() const {
    return std::filesystem::exists(m_dataDir / "index.slidx");
}
bool PersistenceManager::archiveExists() const {
    return std::filesystem::exists(m_dataDir / "archive.slarc");
}

// ─────────────────────────────────────────────────────────────────────
// verifyMagic
//
// Read the first 8 bytes and compare to the expected magic string.
// Returns false (don't throw) because a missing/wrong file is
// not an error — it just means we start fresh.
// ─────────────────────────────────────────────────────────────────────

bool PersistenceManager::verifyMagic(
    const std::string& filepath,
    const char*        expectedMagic)
{
    try {
        BinaryReader reader(filepath);
        char magic[8];
        reader.readRawBytes(magic, 8);
        return std::memcmp(magic, expectedMagic, 8) == 0;
    }
    catch (...) {
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────
// saveConfig
// ─────────────────────────────────────────────────────────────────────

bool PersistenceManager::saveConfig(const AppConfig& config)
{
    try {
        BinaryWriter w(configPath());

        w.writeRawBytes(MAGIC_CONFIG, 8);
        w.writeU32(FORMAT_VERSION);
        w.writeU32(static_cast<uint32_t>(config.version));
        w.writeString(config.dataDirectory);
        w.writeU32(static_cast<uint32_t>(config.confidenceThreshold));
        w.writeU8(config.autoIndexOnDrop ? 1u : 0u);
        w.writeU32(static_cast<uint32_t>(config.maxSearchResults));

        // Recent files list
        w.writeU32(static_cast<uint32_t>(config.recentFiles.size()));
        for (const auto& f : config.recentFiles) {
            w.writeString(f);
        }

        utils::Logger::info("Config saved: " + configPath());
        return true;
    }
    catch (const std::exception& ex) {
        utils::Logger::error(
            std::string("saveConfig failed: ") + ex.what());
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────
// loadConfig
// ─────────────────────────────────────────────────────────────────────

bool PersistenceManager::loadConfig(AppConfig& config)
{
    if (!std::filesystem::exists(configPath())) return false;
    if (!verifyMagic(configPath(), MAGIC_CONFIG)) {
        utils::Logger::warning("Config file magic mismatch — ignoring");
        return false;
    }

    try {
        BinaryReader r(configPath());

        char magic[8];
        r.readRawBytes(magic, 8);         // already verified, skip

        uint32_t version = r.readU32();
        if (version > FORMAT_VERSION) {
            utils::Logger::warning("Config version too new — ignoring");
            return false;
        }

        config.version             = static_cast<int>(r.readU32());
        config.dataDirectory       = r.readString();
        config.confidenceThreshold = static_cast<int>(r.readU32());
        config.autoIndexOnDrop     = (r.readU8() != 0);
        config.maxSearchResults    = static_cast<int>(r.readU32());

        uint32_t recentCount = r.readU32();
        config.recentFiles.clear();
        config.recentFiles.reserve(recentCount);
        for (uint32_t i = 0; i < recentCount; ++i) {
            config.recentFiles.push_back(r.readString());
        }

        utils::Logger::info("Config loaded: " + configPath());
        return true;
    }
    catch (const std::exception& ex) {
        utils::Logger::error(
            std::string("loadConfig failed: ") + ex.what());
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────
// saveIndex
//
// File layout:
//   [8]  magic "SLIDX001"
//   [4]  format version
//   [4]  document count
//   per document:
//     [4]  docId
//     [str] path
//     [str] title
//     [4]  totalWords
//   [4]  word count
//   per word:
//     [str] word
//     [4]  posting list length
//     per posting:
//       [4] docId
//       [4] termFreq
//       [4] firstPosition
// ─────────────────────────────────────────────────────────────────────

bool PersistenceManager::saveIndex(
    const sl::search::SearchEngine& engine)
{
    try {
        BinaryWriter w(indexPath());

        w.writeRawBytes(MAGIC_INDEX, 8);
        w.writeU32(FORMAT_VERSION);

        // ── Documents ─────────────────────────────────────────────
        const auto& index = engine.getInvertedIndex();
        const auto& docMap = index.getAllDocuments();

        w.writeU32(static_cast<uint32_t>(docMap.size()));

        for (const auto& [docId, info] : docMap) {
            w.writeU32(docId);
            w.writeString(info.path);
            w.writeString(info.title);
            w.writeU32(info.totalWords);
        }

        // ── Posting lists ─────────────────────────────────────────
        const auto& indexMap = index.getAllPostings();

        w.writeU32(static_cast<uint32_t>(indexMap.size()));

        for (const auto& [word, postings] : indexMap) {
            w.writeString(word);
            w.writeU32(static_cast<uint32_t>(postings.size()));
            for (const auto& posting : postings) {
                w.writeU32(posting.docId);
                w.writeU32(posting.termFreq);
                w.writeU32(posting.firstPosition);
            }
        }

        utils::Logger::info(
            "Index saved: " + indexPath() +
            "  (" + std::to_string(docMap.size()) + " docs, " +
            std::to_string(indexMap.size()) + " words)");
        return true;
    }
    catch (const std::exception& ex) {
        utils::Logger::error(
            std::string("saveIndex failed: ") + ex.what());
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────
// loadIndex
// ─────────────────────────────────────────────────────────────────────

bool PersistenceManager::loadIndex(
    sl::search::SearchEngine& engine)
{
    if (!indexExists()) return false;
    if (!verifyMagic(indexPath(), MAGIC_INDEX)) {
        utils::Logger::warning("Index magic mismatch — starting fresh");
        return false;
    }

    try {
        BinaryReader r(indexPath());

        char magic[8];
        r.readRawBytes(magic, 8);

        uint32_t version = r.readU32();
        if (version > FORMAT_VERSION) {
            utils::Logger::warning("Index version too new — ignoring");
            return false;
        }

        // ── Documents ─────────────────────────────────────────────
        uint32_t docCount = r.readU32();

        // We need to rebuild documents in the index directly.
        // We use a lower-level path: registerDocumentWithId()
        // which we'll add to InvertedIndex.
        auto& invertedIndex =
            const_cast<sl::search::InvertedIndex&>(
                engine.getInvertedIndex());

        for (uint32_t i = 0; i < docCount; ++i) {
            sl::search::DocumentID id = r.readU32();
            std::string path          = r.readString();
            std::string title         = r.readString();
            uint32_t    totalWords    = r.readU32();

            invertedIndex.registerDocumentWithId(
                id, path, title, totalWords);
        }

        // ── Posting lists ─────────────────────────────────────────
        uint32_t wordCount = r.readU32();

        for (uint32_t i = 0; i < wordCount; ++i) {
            std::string word     = r.readString();
            uint32_t    numPost  = r.readU32();

            // Re-insert into trie
            engine.insertWordIntoTrie(word);

            for (uint32_t j = 0; j < numPost; ++j) {
                sl::search::DocumentID docId = r.readU32();
                uint32_t termFreq            = r.readU32();
                uint32_t firstPos            = r.readU32();

                invertedIndex.addPostingDirect(
                    word, docId, termFreq, firstPos);
            }
        }

        utils::Logger::info(
            "Index loaded: " + std::to_string(docCount) +
            " docs, " + std::to_string(wordCount) + " words");
        return true;
    }
    catch (const std::exception& ex) {
        utils::Logger::error(
            std::string("loadIndex failed: ") + ex.what());
        engine.clear();   // don't leave partial state
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────
// saveArchive
//
// File layout:
//   [8]  magic "SLARC001"
//   [4]  format version
//   [4]  document count
//   per document:
//     [4]   id
//     [str] filename
//     [str] fullPath
//     [str] extractedText
//     [f32] ocrConfidence
//     [4]   wordCount
//     [str] timestamp
// ─────────────────────────────────────────────────────────────────────

bool PersistenceManager::saveArchive(
    const std::vector<sl::gui::ArchivedDocument>& docs)
{
    try {
        BinaryWriter w(archivePath());

        w.writeRawBytes(MAGIC_ARCHIVE, 8);
        w.writeU32(FORMAT_VERSION);
        w.writeU32(static_cast<uint32_t>(docs.size()));

        for (const auto& doc : docs) {
            w.writeU32(static_cast<uint32_t>(doc.id));
            w.writeString(doc.filename);
            w.writeString(doc.fullPath);
            w.writeString(doc.extractedText);
            w.writeF32(doc.ocrConfidence);
            w.writeU32(static_cast<uint32_t>(doc.wordCount));
            w.writeString(doc.timestamp);
        }

        utils::Logger::info(
            "Archive saved: " + std::to_string(docs.size()) +
            " documents → " + archivePath());
        return true;
    }
    catch (const std::exception& ex) {
        utils::Logger::error(
            std::string("saveArchive failed: ") + ex.what());
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────
// loadArchive
// ─────────────────────────────────────────────────────────────────────

bool PersistenceManager::loadArchive(
    std::vector<sl::gui::ArchivedDocument>& docs)
{
    if (!archiveExists()) return false;
    if (!verifyMagic(archivePath(), MAGIC_ARCHIVE)) {
        utils::Logger::warning("Archive magic mismatch — starting fresh");
        return false;
    }

    try {
        BinaryReader r(archivePath());

        char magic[8];
        r.readRawBytes(magic, 8);

        uint32_t version = r.readU32();
        if (version > FORMAT_VERSION) {
            utils::Logger::warning("Archive version too new — ignoring");
            return false;
        }

        uint32_t count = r.readU32();
        docs.clear();
        docs.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            sl::gui::ArchivedDocument doc;
            doc.id             = static_cast<int>(r.readU32());
            doc.filename       = r.readString();
            doc.fullPath       = r.readString();
            doc.extractedText  = r.readString();
            doc.ocrConfidence  = r.readF32();
            doc.wordCount      = static_cast<int>(r.readU32());
            doc.timestamp      = r.readString();
            docs.push_back(std::move(doc));
        }

        utils::Logger::info(
            "Archive loaded: " + std::to_string(docs.size()) +
            " documents");
        return true;
    }
    catch (const std::exception& ex) {
        utils::Logger::error(
            std::string("loadArchive failed: ") + ex.what());
        docs.clear();
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────
// deleteAll
// ─────────────────────────────────────────────────────────────────────

void PersistenceManager::deleteAll()
{
    std::error_code ec;
    for (const auto& p : {configPath(), indexPath(), archivePath()}) {
        if (std::filesystem::exists(p)) {
            std::filesystem::remove(p, ec);
            if (!ec) {
                utils::Logger::info("Deleted: " + p);
            }
        }
    }
}

} // namespace archive
} // namespace sl
