# tuix

[![CI](https://github.com/paolopangrazi/tuix/actions/workflows/ci.yml/badge.svg)](https://github.com/paolopangrazi/tuix/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A terminal spreadsheet built on [FTXUI](https://github.com/ArthurSonzogni/FTXUI). Navigate and edit CSV files with vim-style keybindings, write formulas, and undo/redo changes — all in the terminal.

<!--
  Add a screenshot or demo GIF here. Suggested tools:
    - vhs       https://github.com/charmbracelet/vhs   (scriptable GIFs)
    - asciinema https://asciinema.org                  (terminal recordings)
    - simple    just paste a PNG of your terminal
  Once captured, save it as docs/demo.gif (or .png) and reference it:
    ![demo](docs/demo.gif)
-->
![demo](docs/demo.png)

## Requirements

- CMake 3.14+
- C++17 compiler (GCC 8+ or Clang 7+)
- Git (to fetch submodules)

All dependencies — [FTXUI](https://github.com/ArthurSonzogni/FTXUI), [rapidcsv](https://github.com/d99kris/rapidcsv), and [toml++](https://github.com/marzer/tomlplusplus) — are bundled as git submodules. No system packages are required.

## Build

```bash
git clone --recurse-submodules https://github.com/paolopangrazi/tuix
cd tuix
cmake -B build
cmake --build build
./build/tuix samples/csv/employees.csv   # try it out
```

If you already cloned without `--recurse-submodules`, run `git submodule update --init --recursive` first.

## Usage

```bash
./build/tuix                          # start with a blank sheet
./build/tuix path/to/file.csv         # open a CSV file
```

## Key Bindings

### Normal mode

| Key | Action |
|---|---|
| `h` `j` `k` `l` | Navigate left / down / up / right |
| `i` or `a` | Enter insert mode |
| `x` | Delete cell |
| `u` | Undo |
| `+` | Insert row / column |
| `-` | Delete row / column |
| `:` | Command mode |

### Insert mode

| Key | Action |
|---|---|
| `Enter` | Commit and move down |
| `Tab` | Commit and move right |
| `Escape` | Cancel edit |

### Formulas

Cells starting with `=` are evaluated as formulas. Supported:

- Arithmetic: `=A1 + B2 * 3`
- Cell references: `=A1`, `=Sheet1!B2` (same sheet)
- Ranges via functions: `=SUM(A1:A10)`, `=AVG(B1:B5)`, `=MIN(...)`, `=MAX(...)`

## Configuration

tuix reads `~/.config/tuix/config.toml` at startup (XDG-compliant: respects `$XDG_CONFIG_HOME`). All settings are optional; missing keys fall back to defaults.

```toml
[colors]
# ANSI name or palette index (0–15)
cursor_bg       = "green"
cursor_fg       = "black"
selection_bg    = "blue"
selection_fg    = "white"
header          = "green"
row_number      = "green"
dimmed          = "gray_light"
insert_badge_bg = "green"
insert_badge_fg = "black"
normal_badge_bg = "blue"
normal_badge_fg = "white"
titlebar_bg     = "green"
titlebar_fg     = "black"
formula_fg      = "cyan"

[keys]
# each binding is a list of single-character strings
nav_up      = ["k"]
nav_down    = ["j"]
nav_left    = ["h"]
nav_right   = ["l"]
insert_mode = ["i", "a"]
delete_cell = ["x"]
undo        = ["u"]
insert_row  = ["+"]
delete_row  = ["-"]
insert_col  = ["+"]
delete_col  = ["-"]
rename_col  = ["i", "a"]
cmd_mode    = [":"]

[grid]
cell_width = 12   # minimum 4
```

## Install

```bash
cmake --install build --prefix ~/.local
# binary is placed at ~/.local/bin/tuix
```

## License

MIT — see [LICENSE](LICENSE).
