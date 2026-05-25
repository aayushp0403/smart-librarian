# ─────────────────────────────────────────────────────────────────────
# Tesseract: support both vcpkg (local dev) and system apt (CI)
#
# SL_USE_SYSTEM_TESSERACT=ON  → use pkg-config / system headers
# SL_USE_SYSTEM_TESSERACT=OFF → use vcpkg find_package (default)
# ─────────────────────────────────────────────────────────────────────

option(SL_USE_SYSTEM_TESSERACT
    "Use system-installed Tesseract instead of vcpkg" OFF)

option(SL_USE_SYSTEM_GTEST
    "Use system-installed GoogleTest instead of vcpkg" OFF)

add_library(sl_ocr STATIC
    ImageLoader.cpp
    ImagePreprocessor.cpp
    OcrEngine.cpp
)

target_include_directories(sl_ocr PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

if(SL_USE_SYSTEM_TESSERACT)
    # ── System Tesseract (installed via apt libtesseract-dev) ────────
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(TESSERACT REQUIRED tesseract)
    pkg_check_modules(LEPTONICA REQUIRED lept)

    target_include_directories(sl_ocr PUBLIC
        ${TESSERACT_INCLUDE_DIRS}
        ${LEPTONICA_INCLUDE_DIRS}
    )
    target_link_libraries(sl_ocr PUBLIC
        ${TESSERACT_LIBRARIES}
        ${LEPTONICA_LIBRARIES}
    )
    target_compile_options(sl_ocr PUBLIC
        ${TESSERACT_CFLAGS_OTHER}
    )
else()
    # ── vcpkg Tesseract (local development) ──────────────────────────
    find_package(Tesseract CONFIG REQUIRED)
    find_package(Leptonica  CONFIG REQUIRED)

    target_link_libraries(sl_ocr PUBLIC
        Tesseract::libtesseract
        ${Leptonica_LIBRARIES}
    )
endif()

target_compile_features(sl_ocr PUBLIC cxx_std_17)
