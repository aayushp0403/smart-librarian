# Find Tesseract and Leptonica via vcpkg's CMake integration
find_package(Tesseract CONFIG REQUIRED)
find_package(Leptonica CONFIG REQUIRED)

add_library(sl_ocr STATIC
    ImageLoader.cpp
    ImagePreprocessor.cpp
    OcrEngine.cpp
)

target_include_directories(sl_ocr PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# Link Tesseract and Leptonica
# The vcpkg toolchain makes these targets available automatically
target_link_libraries(sl_ocr
    PUBLIC
        Tesseract::libtesseract
        ${Leptonica_LIBRARIES}
)

target_compile_features(sl_ocr PUBLIC cxx_std_17)
