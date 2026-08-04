cmake_minimum_required(VERSION 3.14)
project(tuix VERSION 1.1.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# On MSVC, statically link the C runtime so the released tuix.exe is fully
# self-contained (no Visual C++ redistributable needed). Set before any
# add_subdirectory so the vendored deps inherit the same runtime.
if(MSVC)
    if(POLICY CMP0091)
        cmake_policy(SET CMP0091 NEW)
    endif()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

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
set(OPENXLSX_CREATE_DOCS   OFF CACHE BOOL "" FORCE)  # don't require Doxygen/Graphviz
# v0.5.1's manage_dependency does a find_package() before fetching; a system
# pugixml then collides with the fetched one ("imported target already exists").
# Fetch all deps to match tuix's vendored-deps model and dodge that collision.
set(FORCE_FETCH_ALL        ON  CACHE BOOL "" FORCE)
add_subdirectory(deps/openxlsx)

# ---------------------------------------------------------------------------
# tuix_core — all application logic, minus main(). The executable is a thin
# shell over this library so the same code can be linked into the test suite.
# ---------------------------------------------------------------------------
find_package(Threads REQUIRED)

add_library(tuix_core STATIC
    config/config.cpp
    config/themes.cpp
    components/actionbox.cpp
    components/calc_cache.cpp
    components/cell.cpp
    components/file_bar.cpp
    components/formula_catalog.cpp
    components/heatmap.cpp
    components/chart_panel.cpp
    components/grid.cpp
    components/grid_edit.cpp
    components/grid_view.cpp
    components/grid_search.cpp
    components/grid_analytics.cpp
    components/grid_render.cpp
    components/grid_input.cpp
    components/titlebar.cpp
    components/body.cpp
    components/status_area.cpp
    components/suggestion_bar.cpp
    components/column_stats.cpp
    components/session.cpp
    components/workbook.cpp
    components/tab_bar.cpp
    components/cmd_mode.cpp
    cli/headless.cpp
    dialogs/dialog_shell.cpp
    dialogs/confirm_dialog.cpp
    dialogs/path_input_dialog.cpp
    dialogs/help_dialog.cpp
    dialogs/config_dialog.cpp
    dialogs/sheet_actions_dialog.cpp
    util/clipboard.cpp
    util/native_file_dialog.cpp
    util/text_width.cpp
    loader/csv_loader.cpp
    loader/xlsx_loader.cpp
    writer/csv_writer.cpp
    writer/xlsx_writer.cpp
    formulas/value.cpp
    formulas/lexer.cpp
    formulas/parser.cpp
    formulas/evaluator.cpp
    formulas/evaluator_builtins.cpp
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

# Windows: the native Explorer file picker (util/native_file_dialog.cpp) uses
# the COM shell dialog APIs from ole32.
if(WIN32)
    target_link_libraries(tuix_core PUBLIC ole32)
endif()

# Project code builds warning-clean; keep it that way (deps are untouched).
# TUIX_WERROR promotes that to an error — CI turns it on so a regression can't
# land unnoticed; it stays off locally so a new compiler's fresh warnings don't
# block day-to-day work. PRIVATE: only our own sources, never the vendored deps.
option(TUIX_WERROR "Treat warnings in tuix's own sources as errors" OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tuix_core PRIVATE -Wall -Wextra)
    if(TUIX_WERROR)
        target_compile_options(tuix_core PRIVATE -Werror)
    endif()
elseif(MSVC)
    if(TUIX_WERROR)
        target_compile_options(tuix_core PRIVATE /WX)
    endif()
endif()

# MSVC reports narrowing conversions inside the OpenXLSX headers (C4244/C4267)
# against whichever of our TUs includes them. Silence them at the two include
# sites rather than patching the submodule.
if(MSVC)
    set_source_files_properties(
        loader/xlsx_loader.cpp
        writer/xlsx_writer.cpp
        PROPERTIES COMPILE_OPTIONS "/wd4244;/wd4267"
    )
endif()

# ---------------------------------------------------------------------------
# tuix — the executable
# ---------------------------------------------------------------------------
add_executable(tuix main.cpp)
target_link_libraries(tuix PRIVATE tuix_core)

# COMPONENT lets the release packaging install *only* the binary: the vendored
# deps (OpenXLSX, and doctest when tests are on) register their own install
# rules, which would otherwise drop headers and static libs into the tarball.
#   cmake --install build --component tuix --prefix ...
install(TARGETS tuix DESTINATION bin COMPONENT tuix)

# ---------------------------------------------------------------------------
# Tests — opt-in (off by default to keep the standard build lean).
#   cmake -B build -DTUIX_BUILD_TESTS=ON && cmake --build build && ctest --test-dir build
# ---------------------------------------------------------------------------
option(TUIX_BUILD_TESTS "Build the tuix test suite" OFF)
if(TUIX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
