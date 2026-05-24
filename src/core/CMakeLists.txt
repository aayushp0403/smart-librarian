add_library(sl_core STATIC
    Application.cpp
)

target_include_directories(sl_core PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# When other targets link sl_core, they automatically
# get the include path too (PUBLIC propagation).
target_compile_features(sl_core PUBLIC cxx_std_17)