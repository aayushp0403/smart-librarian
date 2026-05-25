# ─────────────────────────────────────────────────────────────────────
# Find GoogleTest
#
# GTest::gtest       — the main library
# GTest::gtest_main  — provides a main() that runs all tests
#                      Use this unless you need a custom main()
# GTest::gmock       — mocking framework (we'll use later)
# ─────────────────────────────────────────────────────────────────────
find_package(GTest CONFIG REQUIRED)

message(STATUS "GoogleTest found: ${GTest_VERSION}")

add_subdirectory(unit)
add_subdirectory(integration)
