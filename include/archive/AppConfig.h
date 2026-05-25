#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace sl {
namespace archive {

/**
 * AppConfig
 *
 * Lightweight application settings that persist between sessions.
 *
 * Stored as a separate file from the index so the user can
 * delete the index without losing their preferences.
 *
 * recentFiles: last N files processed by OCR.
 *   We keep a maximum of MAX_RECENT entries.
 *   On save, we trim to MAX_RECENT. On load, newer entries first.
 */
struct AppConfig
{
    static constexpr int    MAX_RECENT         = 10;
    static constexpr int    CURRENT_VERSION    = 1;

    int         version             { CURRENT_VERSION };
    std::string dataDirectory;      // where we save index and archive
    int         confidenceThreshold { 60 };
    bool        autoIndexOnDrop     { true };
    int         maxSearchResults    { 20 };

    std::vector<std::string> recentFiles;

    // Add a file to the recent list, keeping it within MAX_RECENT
    void addRecentFile(const std::string& path)
    {
        // Remove if already present (move to front)
        for (auto it = recentFiles.begin();
             it != recentFiles.end(); ++it)
        {
            if (*it == path) {
                recentFiles.erase(it);
                break;
            }
        }
        recentFiles.insert(recentFiles.begin(), path);
        if (recentFiles.size() > static_cast<std::size_t>(MAX_RECENT)) {
            recentFiles.resize(static_cast<std::size_t>(MAX_RECENT));
        }
    }
};

} // namespace archive
} // namespace sl
