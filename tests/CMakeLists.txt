# ---------------------------------------------------------------------------
# Test framework: doctest, fetched on demand (only when TUIX_BUILD_TESTS=ON).
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
)
FetchContent_MakeAvailable(doctest)

# Provides doctest_discover_tests(), registering each TEST_CASE as a CTest case.
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

add_executable(tuix_tests
    doctest_main.cpp
    unit/evaluator_test.cpp
    unit/sparkline_test.cpp
    unit/lookup_functions_test.cpp
    unit/cross_sheet_test.cpp
    unit/clipboard_test.cpp
    integration/cross_sheet_grid_test.cpp
    integration/sort_grid_test.cpp
)

target_link_libraries(tuix_tests PRIVATE tuix_core doctest::doctest)

# For #include "support/...". The tuix_core PUBLIC includes cover the app headers.
target_include_directories(tuix_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

doctest_discover_tests(tuix_tests)
