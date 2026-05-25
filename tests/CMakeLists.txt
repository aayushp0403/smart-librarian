# ─────────────────────────────────────────────────────────────────────
# GoogleTest: support both vcpkg and system apt (libgtest-dev)
# ─────────────────────────────────────────────────────────────────────

option(SL_USE_SYSTEM_GTEST
    "Use system GoogleTest instead of vcpkg" OFF)

if(SL_USE_SYSTEM_GTEST)
    # System GoogleTest installed via apt libgtest-dev
    find_package(GTest REQUIRED)

    # apt installs gtest sources, not a compiled lib on some versions
    # This handles both cases
    if(NOT TARGET GTest::gtest_main)
        # Older apt: compile from source
        add_subdirectory(
            /usr/src/googletest
            ${CMAKE_BINARY_DIR}/googletest
            EXCLUDE_FROM_ALL
        )
    endif()
else()
    # vcpkg GoogleTest
    find_package(GTest CONFIG REQUIRED)
endif()

message(STATUS "GoogleTest ready for tests")

add_subdirectory(unit)
add_subdirectory(integration)
