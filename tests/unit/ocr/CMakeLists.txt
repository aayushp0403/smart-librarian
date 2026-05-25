add_executable(test_ocr
    test_ImageBuffer.cpp
    test_ImagePreprocessor.cpp
)

target_link_libraries(test_ocr
    PRIVATE
        sl_ocr
        GTest::gtest_main
)

target_include_directories(test_ocr PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_discover_tests(test_ocr)
