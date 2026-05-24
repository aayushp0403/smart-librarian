# Each module will be added here as we build them.
# For now, we build the main entry point only.

add_subdirectory(core)
add_subdirectory(utils)

add_executable(SmartLibrarian
    main.cpp
)

target_include_directories(SmartLibrarian PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(SmartLibrarian PRIVATE
    sl_core
    sl_utils
)