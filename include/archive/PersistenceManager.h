#pragma once

#include "archive/AppConfig.h"
#include "gui/ArchiveWidget.h"   // ArchivedDocument

#include <string>
#include <vector>
#include <filesystem>

// Forward declarations
namespace sl {
namespace search { class SearchEngine; }
}

namespace sl {
namespace archive {

/**
 * PersistenceManager
 *
 * Owns the data directory and coordinates save/load operations
 * for all persistent application state.
 *
 * Data directory layout:
 *   ~/.smartlibrarian/
 *     config.slcfg      ← AppConfig (settings)
 *     index.slidx       ← SearchEngine index (inverted index + trie words)
 *     archive.slarc     ← ArchivedDocument list
 *
 * File format magic numbers:
 *   config.slcfg  → "SLCFG001"  (8 bytes)
 *   index.slidx   → "SLIDX001"  (8 bytes)
 *   archive.slarc → "SLARC001"  (8 bytes)
 *
 * The magic number is the first 8 bytes of every file.
 * On load, we verify it before reading anything else.
 * If the magic doesn't match, we return false and start fresh
 * rather than trying to parse garbage data.
 *
 * Version field (uint32_t after magic):
 *   Allows format upgrades. If the saved version > current version,
 *   we reject the file (we can't read a newer format).
 *   If saved version < current, we can apply migration logic.
 *
 * std::filesystem (C++17):
 *   We use std::filesystem::path for path manipulation and
 *   std::filesystem::create_directories to ensure the data
 *   directory exists. This is the modern C++ replacement for
 *   POSIX mkdir() and Windows CreateDirectory().
 */
class PersistenceManager
{
public:
    // Magic bytes for each file type
    static constexpr const char* MAGIC_CONFIG  = "SLCFG001";
    static constexpr const char* MAGIC_INDEX   = "SLIDX001";
    static constexpr const char* MAGIC_ARCHIVE = "SLARC001";
    static constexpr uint32_t    FORMAT_VERSION = 1;

    /**
     * Constructor — establishes the data directory.
     * Creates it if it doesn't exist.
     *
     * @param dataDir  path to store all persistent files.
     *                 Default: ~/.smartlibrarian/
     */
    explicit PersistenceManager(
        const std::string& dataDir = "");

    ~PersistenceManager() = default;

    PersistenceManager(const PersistenceManager&)            = delete;
    PersistenceManager& operator=(const PersistenceManager&) = delete;

    // ── Config ────────────────────────────────────────────────────

    bool saveConfig(const AppConfig& config);
    bool loadConfig(AppConfig& config);

    // ── Search Index ──────────────────────────────────────────────

    /**
     * saveIndex — serialize the entire inverted index to disk.
     *
     * We access the index internals via the SearchEngine's
     * public query interface rather than making PersistenceManager
     * a friend. This keeps the layering clean:
     * PersistenceManager doesn't need to know about Posting structs.
     *
     * What we save:
     *   - All registered documents (id, path, title, totalWords)
     *   - All indexed words with their complete posting lists
     */
    bool saveIndex(const sl::search::SearchEngine& engine);

    /**
     * loadIndex — reconstruct the search engine state from disk.
     *
     * Returns true if loaded successfully, false if file doesn't
     * exist or is corrupt (caller should start with empty engine).
     */
    bool loadIndex(sl::search::SearchEngine& engine);

    // ── Archive ───────────────────────────────────────────────────

    bool saveArchive(
        const std::vector<sl::gui::ArchivedDocument>& docs);

    bool loadArchive(
        std::vector<sl::gui::ArchivedDocument>& docs);

    // ── Utility ───────────────────────────────────────────────────

    std::string dataDirectory()    const;
    std::string configPath()       const;
    std::string indexPath()        const;
    std::string archivePath()      const;

    bool indexExists()   const;
    bool archiveExists() const;

    /**
     * deleteAll — remove all saved data.
     * Used by "Clear Archive" to fully reset the app state.
     */
    void deleteAll();

private:
    bool verifyMagic(const std::string& filepath,
                     const char*        expectedMagic);

    std::filesystem::path m_dataDir;
};

} // namespace archive
} // namespace sl
