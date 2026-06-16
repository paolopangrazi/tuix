cmake_minimum_required(VERSION 3.14)
project(tuix VERSION 0.2.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release if no build type specified (common quality-of-life)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Always emit compile_commands.json for clangd / IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Submodule sanity check — all deps live in deps/ as git submodules
# ---------------------------------------------------------------------------
foreach(dep ftxui rapidcsv tomlplusplus openxlsx)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/${dep}/CMakeLists.txt"
       AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/${dep}/src")
        message(FATAL_ERROR
            "Submodule deps/${dep} is missing. "
            "Run: git submodule update --init --recursive")
    endif()
endforeach()

# ---------------------------------------------------------------------------
# ftxui (deps/ftxui)
# ---------------------------------------------------------------------------
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)  # don't leak into our install
add_subdirectory(deps/ftxui)

# ---------------------------------------------------------------------------
# rapidcsv (deps/rapidcsv) — header-only
# ---------------------------------------------------------------------------
set(RAPIDCSV_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/rapidcsv/src")

# ---------------------------------------------------------------------------
# tomlplusplus (deps/tomlplusplus) — header-only
# ---------------------------------------------------------------------------
add_subdirectory(deps/tomlplusplus)

# ---------------------------------------------------------------------------
# OpenXLSX (deps/openxlsx)
# ---------------------------------------------------------------------------
# miniz (fetched by OpenXLSX) requires this policy flag on CMake >= 3.30
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(OPENXLSX_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/openxlsx)

# ---------------------------------------------------------------------------
# tuix_core — all application logic, minus main(). The executable is a thin
# shell over this library so the same code can be linked into the test suite.
# ---------------------------------------------------------------------------
find_package(Threads REQUIRED)

add_library(tuix_core STATIC
    config/config.cpp
    components/actionbox.cpp
    components/calc_cache.cpp
    components/cell.cpp
    components/file_bar.cpp
    components/grid.cpp
    components/titlebar.cpp
    components/body.cpp
    components/statusbar.cpp
    components/status_area.cpp
    components/suggestion_bar.cpp
    components/column_stats.cpp
    components/session.cpp
    components/workbook.cpp
    components/tab_bar.cpp
    components/cmd_mode.cpp
    dialogs/dialog_shell.cpp
    dialogs/confirm_dialog.cpp
    dialogs/path_input_dialog.cpp
    dialogs/help_dialog.cpp
    dialogs/config_dialog.cpp
    dialogs/sheet_actions_dialog.cpp
    loader/csv_loader.cpp
    loader/xlsx_loader.cpp
    writer/csv_writer.cpp
    writer/xlsx_writer.cpp
    formulas/value.cpp
    formulas/lexer.cpp
    formulas/parser.cpp
    formulas/evaluator.cpp
)

# PUBLIC so consumers (the executable and the tests) inherit them.
target_include_directories(tuix_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}            # for loader/csv_loader.hpp from within components/
    ${CMAKE_CURRENT_SOURCE_DIR}/components # for body.hpp, grid.hpp, etc.
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs    # for dialog_shell.hpp etc.
    ${RAPIDCSV_INCLUDE_DIR}
)

target_link_libraries(tuix_core PUBLIC
    ftxui::component
    tomlplusplus::tomlplusplus
    OpenXLSX::OpenXLSX
    Threads::Threads
)

# ---------------------------------------------------------------------------
# tuix — the executable
# ---------------------------------------------------------------------------
add_executable(tuix main.cpp)
target_link_libraries(tuix PRIVATE tuix_core)

install(TARGETS tuix DESTINATION bin)

# ---------------------------------------------------------------------------
# Tests — opt-in (off by default to keep the standard build lean).
#   cmake -B build -DTUIX_BUILD_TESTS=ON && cmake --build build && ctest --test-dir build
# ---------------------------------------------------------------------------
option(TUIX_BUILD_TESTS "Build the tuix test suite" OFF)
if(TUIX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
