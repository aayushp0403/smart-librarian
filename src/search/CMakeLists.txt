add_library(sl_search STATIC
    TextProcessor.cpp
    Trie.cpp
    InvertedIndex.cpp
    SearchEngine.cpp
)

target_include_directories(sl_search PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(sl_search PUBLIC cxx_std_17)

# Link math library for std::log (needed on some Linux configs)
target_link_libraries(sl_search PUBLIC m)
