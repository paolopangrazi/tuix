# ---------------------------------------------------------------------------
# Test framework: doctest, fetched on demand (only when TUIX_BUILD_TESTS=ON).
# ---------------------------------------------------------------------------
include(FetchContent)

# doctest v2.4.11 declares cmake_minimum_required(VERSION 3.0), which CMake >= 4
# rejects outright. Set here (not only at the top level, where the same variable
# is needed for miniz) so the test build stands on its own.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

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
    unit/headless_test.cpp
    unit/sheet_format_test.cpp
    unit/chart_test.cpp
    unit/numbers_test.cpp
    unit/cmd_mode_test.cpp
    unit/tips_catalog_test.cpp
    integration/cross_sheet_grid_test.cpp
    integration/sort_grid_test.cpp
    integration/chart_grid_test.cpp
    integration/config_theme_test.cpp
    integration/config_save_test.cpp
    integration/tips_config_test.cpp
)

target_link_libraries(tuix_tests PRIVATE tuix_core doctest::doctest)

# doctest 2.4.11 specializes std::tuple for its stringification hooks, which
# newer MSVC flags as C5285. Nothing we can fix on our side; mute it.
if(MSVC)
    target_compile_options(tuix_tests PRIVATE /wd5285)
endif()

# For #include "support/...". The tuix_core PUBLIC includes cover the app headers.
target_include_directories(tuix_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

doctest_discover_tests(tuix_tests)
