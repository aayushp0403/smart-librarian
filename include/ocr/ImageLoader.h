#pragma once

#include "ocr/ImageBuffer.h"
#include <string>

namespace sl {
namespace ocr {

/**
 * ImageLoader
 *
 * Loads image files from disk into ImageBuffer objects.
 *
 * Uses stb_image — a single-header C library that handles:
 *   PNG, JPEG, BMP, TGA, GIF, PSD, PIC, PNM
 *
 * stb_image memory model:
 *   stbi_load() allocates a buffer with malloc() and returns a pointer.
 *   You must call stbi_image_free() when done.
 *   We immediately copy into our std::vector<uint8_t> and free.
 *   This converts from C malloc ownership to C++ RAII ownership.
 *
 * Why copy immediately instead of holding the stbi pointer?
 *   - Our ImageBuffer is movable, copyable, and integrates with RAII
 *   - Lifetime management is cleaner
 *   - For typical document images (< 10MB), the copy cost is negligible
 *   - We retain full control over the memory allocator
 */
class ImageLoader
{
public:
    /**
     * load — load an image file into an ImageBuffer.
     *
     * @param filepath  path to image file
     * @param forceChannels  0 = keep original, 1 = force grayscale,
     *                       3 = force RGB, 4 = force RGBA
     * @return          ImageBuffer containing pixel data
     * @throws          std::runtime_error if file cannot be loaded
     */
    static ImageBuffer load(const std::string& filepath,
                            int forceChannels = 0);

    /**
     * loadFromMemory — decode image from an in-memory buffer.
     * Useful when image data comes from a network stream or archive.
     */
    static ImageBuffer loadFromMemory(const uint8_t* data,
                                      std::size_t    size,
                                      int forceChannels = 0);

    /**
     * isSupported — check if a file extension is supported.
     */
    static bool isSupported(const std::string& filepath);
};

} // namespace ocr
} // namespace sl
