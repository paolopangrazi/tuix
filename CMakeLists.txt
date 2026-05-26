cmake_minimum_required(VERSION 3.14)
project(tuix VERSION 0.1.0 LANGUAGES CXX)

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
foreach(dep ftxui rapidcsv tomlplusplus)
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
# tuix
# ---------------------------------------------------------------------------
add_executable(tuix
    main.cpp
    config/config.cpp
    components/actionbox.cpp
    components/cell.cpp
    components/grid.cpp
    components/titlebar.cpp
    components/body.cpp
    components/statusbar.cpp
    loader/csv_loader.cpp
    writer/csv_writer.cpp
    formulas/value.cpp
    formulas/lexer.cpp
    formulas/parser.cpp
    formulas/evaluator.cpp
)

target_include_directories(tuix PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}            # for loader/csv_loader.hpp from within components/
    ${CMAKE_CURRENT_SOURCE_DIR}/components # for body.hpp, grid.hpp, etc. from main.cpp
    ${RAPIDCSV_INCLUDE_DIR}
)

target_link_libraries(tuix PRIVATE
    ftxui::component
    ftxui::dom
    ftxui::screen
    tomlplusplus::tomlplusplus
)

install(TARGETS tuix DESTINATION bin)
