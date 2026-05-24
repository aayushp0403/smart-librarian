add_library(sl_utils STATIC
    Logger.cpp
)

target_include_directories(sl_utils PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(sl_utils PUBLIC cxx_std_17)