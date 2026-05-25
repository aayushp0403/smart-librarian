add_executable(test_pdf
    test_PdfStream.cpp
    test_PdfPage.cpp
    test_PdfDocument.cpp
)

target_link_libraries(test_pdf
    PRIVATE
        sl_pdf
        GTest::gtest_main
)

target_include_directories(test_pdf PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_discover_tests(test_pdf)
