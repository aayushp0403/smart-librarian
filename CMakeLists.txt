cmake_minimum_required(VERSION 3.20)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain"
    )
endif()

project(SmartLibrarian
    VERSION 0.1.0
    DESCRIPTION "Intelligent Document Archival and PDF Pipeline"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wshadow -Wnon-virtual-dtor
        -Wunused -Wconversion -Wsign-conversion
        -Wnull-dereference -Wdouble-promotion
    )
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
message(STATUS "Qt6 version: ${Qt6_VERSION}")

# ─────────────────────────────────────────────────────────────────────
# Enable CTest — CMake's built-in test runner.
# Must be called BEFORE add_subdirectory so that test targets
# defined in subdirectories are registered with CTest.
# After this call, 'ctest' command works in the build directory.
# ─────────────────────────────────────────────────────────────────────
enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
