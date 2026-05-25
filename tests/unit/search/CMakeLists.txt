add_executable(test_search
    test_TextProcessor.cpp
    test_Trie.cpp
    test_InvertedIndex.cpp
    test_SearchEngine.cpp
)

target_link_libraries(test_search
    PRIVATE
        sl_search
        GTest::gtest_main
)

target_include_directories(test_search PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

# Register with CTest.
# include(GoogleTest) provides gtest_discover_tests() which
# automatically finds all TEST() macros in the binary and
# registers each as a separate CTest test case.
# This means 'ctest' can run individual tests, not just the whole binary.
include(GoogleTest)
gtest_discover_tests(test_search)
