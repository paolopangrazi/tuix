# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
cmake -B build          # configure (first time)
cmake --build build     # build
./build/tuix            # run
./build/tuix samples/csv/employees.csv  # open a file
```

There are no tests. clangd uses `build/compile_commands.json` (configured in `.clangd`).

## Architecture

**tuix** is a terminal spreadsheet (TUI) built on [FTXUI](https://github.com/ArthurSonzogni/FTXUI). All state lives in-process; there is no persistence layer beyond file load/save.

### Data flow

```
File  →  loader/  →  Grid::load(CsvData)  →  Grid (in-memory cells)
Grid  →  writer/  →  File
```

`CsvData` (defined in `loader/csv_loader.hpp`) is the interchange struct between loaders, writers, and `Grid`. When adding a new file format, produce/consume `CsvData` — the grid doesn't need to change.

### Component hierarchy

```
main.cpp
├── Body  →  Grid          (spreadsheet data + all editing logic)
├── TitleBar               (menu buttons: undo/redo/open/save/exit)
└── StatusBar              (one-line message display)
```

`Body` is a thin shell around `Grid`. Almost all logic — navigation, editing, selection, undo/redo, formula evaluation, rendering — is in `Grid` (components/grid.cpp, ~900 lines).

### Formula pipeline

Cells starting with `=` are evaluated lazily on render via a full expression pipeline:

```
raw string  →  Lexer  →  []Token  →  Parser  →  Expr (AST)  →  Evaluator  →  Value
```

`Grid` implements `EvalContext` (providing `cell_value(r, c)`, `rows()`, `cols()`), which `Evaluator` calls back into for cell references and ranges. Circular references are guarded by a per-call static set in `Grid::cell_value`.

### Undo/redo

Single-cell granularity. Each committed edit pushes a `HistoryEntry {row, col, before, after}` onto `m_undo_stack` and clears `m_redo_stack`. Column renames are tracked the same way (row = -1 sentinel).

### Configuration

`Config::load()` reads `~/.config/tuix/config.toml` at startup (before FTXUI takes over the terminal). All color and keybinding defaults live in `config/config.hpp`; the toml only overrides them. Colors use FTXUI's `Color` type; the parser (`config/config.cpp`) maps ANSI name strings and palette indices 0–15.

### Vendored dependencies

| Dep | Location | How used |
|---|---|---|
| ftxui | `deps/ftxui/` (submodule) or FetchContent | Rendering, components, events |
| rapidcsv | `deps/rapidcsv/` (header-only) | CSV parsing in `loader/csv_loader.cpp` |
| tinyfiledialogs | `deps/tinyfiledialogs/` | Native file-open dialog (linked via `_tinyfd` CMake target) |
| toml++ | system (`find_package`) | Config file parsing |
