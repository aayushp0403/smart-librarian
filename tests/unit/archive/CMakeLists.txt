add_executable(test_archive
    test_BinaryWriterReader.cpp
    test_PersistenceManager.cpp
)

target_link_libraries(test_archive
    PRIVATE
        sl_archive
        sl_search
        sl_gui
        GTest::gtest_main
        Qt6::Core
        Qt6::Widgets
)

target_include_directories(test_archive PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_discover_tests(test_archive)
