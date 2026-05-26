cmake_minimum_required(VERSION 3.14)
project(tuix VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

# ---------------------------------------------------------------------------
# ftxui — use git submodule in deps/ftxui if present, otherwise FetchContent
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/ftxui/CMakeLists.txt")
    add_subdirectory(deps/ftxui)
else()
    include(FetchContent)
    FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
        GIT_TAG v5.0.0
    )
    FetchContent_MakeAvailable(ftxui)
    # expose the include dir so compile_commands.json gets the -I flag
    set(FTXUI_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/ftxui-src/include")
endif()

# ---------------------------------------------------------------------------
# rapidcsv — vendored header-only library in deps/rapidcsv/
# ---------------------------------------------------------------------------
set(RAPIDCSV_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/rapidcsv/src")

# ---------------------------------------------------------------------------
# tomlplusplus — system-installed header-only library
# ---------------------------------------------------------------------------
find_package(tomlplusplus REQUIRED)

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
    $<$<BOOL:${FTXUI_INCLUDE_DIR}>:${FTXUI_INCLUDE_DIR}>
)

target_link_libraries(tuix PRIVATE
    ftxui::component
    ftxui::dom
    ftxui::screen
    tomlplusplus::tomlplusplus
)

install(TARGETS tuix DESTINATION bin)
