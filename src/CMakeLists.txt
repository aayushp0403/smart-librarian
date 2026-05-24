add_subdirectory(core)
add_subdirectory(utils)
add_subdirectory(pdf)

add_executable(SmartLibrarian
    main.cpp
)

target_include_directories(SmartLibrarian PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(SmartLibrarian PRIVATE
    sl_core
    sl_utils
    sl_pdf
)
