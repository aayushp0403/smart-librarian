#include "ocr/ImageLoader.h"

// stb_image: define this in EXACTLY ONE .cpp file to generate
// the implementation. All other files that include stb_image.h
// get only the declarations.
//
// This is the "unity build" pattern for single-header libraries.
// The implementation is conditionally compiled based on this define.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace sl {
namespace ocr {

// ─────────────────────────────────────────────────────────────────────
// load
// ─────────────────────────────────────────────────────────────────────

ImageBuffer ImageLoader::load(const std::string& filepath, int forceChannels)
{
    int width    = 0;
    int height   = 0;
    int channels = 0;

    // stbi_load:
    //   - Reads the file, auto-detects format
    //   - Decodes into a flat uint8_t* buffer (malloc'd)
    //   - Fills width, height, channels
    //   - forceChannels: if non-zero, converts to that channel count
    //   - Returns nullptr on failure
    uint8_t* data = stbi_load(
        filepath.c_str(),
        &width, &height, &channels,
        forceChannels
    );

    if (!data) {
        throw std::runtime_error(
            "ImageLoader: failed to load '" + filepath +
            "': " + stbi_failure_reason()
        );
    }

    // If forceChannels was set, the actual output channels = forceChannels
    int outputChannels = (forceChannels > 0) ? forceChannels : channels;

    // Copy into our RAII ImageBuffer
    // After this, data (malloc) is no longer needed
    ImageBuffer buf(width, height, outputChannels);

    std::size_t byteCount =
        static_cast<std::size_t>(width) *
        static_cast<std::size_t>(height) *
        static_cast<std::size_t>(outputChannels);

    std::copy(data, data + byteCount, buf.pixels.begin());

    // Free the stbi-allocated buffer
    // MUST be stbi_image_free, not delete[] or free()
    // because stbi uses its own allocator internally
    stbi_image_free(data);

    return buf;  // NRVO: compiler elides the copy
}

// ─────────────────────────────────────────────────────────────────────
// loadFromMemory
// ─────────────────────────────────────────────────────────────────────

ImageBuffer ImageLoader::loadFromMemory(
    const uint8_t* data,
    std::size_t    size,
    int            forceChannels
)
{
    if (!data || size == 0) {
        throw std::invalid_argument("ImageLoader: null or empty memory buffer");
    }

    int width    = 0;
    int height   = 0;
    int channels = 0;

    uint8_t* decoded = stbi_load_from_memory(
        data,
        static_cast<int>(size),
        &width, &height, &channels,
        forceChannels
    );

    if (!decoded) {
        throw std::runtime_error(
            std::string("ImageLoader: failed to decode from memory: ") +
            stbi_failure_reason()
        );
    }

    int outputChannels = (forceChannels > 0) ? forceChannels : channels;
    ImageBuffer buf(width, height, outputChannels);

    std::size_t byteCount =
        static_cast<std::size_t>(width) *
        static_cast<std::size_t>(height) *
        static_cast<std::size_t>(outputChannels);

    std::copy(decoded, decoded + byteCount, buf.pixels.begin());
    stbi_image_free(decoded);

    return buf;
}

// ─────────────────────────────────────────────────────────────────────
// isSupported
// ─────────────────────────────────────────────────────────────────────

bool ImageLoader::isSupported(const std::string& filepath)
{
    // Extract extension and lowercase it
    auto pos = filepath.rfind('.');
    if (pos == std::string::npos) return false;

    std::string ext = filepath.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    static const std::vector<std::string> supported = {
        "png", "jpg", "jpeg", "bmp", "tga", "gif",
        "tif", "tiff", "pnm", "pgm", "ppm"
    };

    for (const auto& s : supported) {
        if (ext == s) return true;
    }
    return false;
}

} // namespace ocr
} // namespace sl
