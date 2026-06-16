<div align="center">


<h1>tuiX</h1>

**tui eXcel-lent — a fast, keyboard-driven spreadsheet editor for the terminal.**

[![CI](https://github.com/paolopangrazi/tuix/actions/workflows/ci.yml/badge.svg)](https://github.com/paolopangrazi/tuix/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C.svg)](https://en.cppreference.com/w/cpp/17)
[![Built with FTXUI](https://img.shields.io/badge/built%20with-FTXUI-8A2BE2.svg)](https://github.com/ArthurSonzogni/FTXUI)

Open a CSV or XLSX file, move through it with vim keys, evaluate formulas, and save —
without leaving your terminal. tuiX is a single native C++ binary built on
[FTXUI](https://github.com/ArthurSonzogni/FTXUI), with no runtime dependencies.

<video src="https://github.com/user-attachments/assets/651a9673-4791-4660-a642-3d17b1e80670" autoplay loop muted playsinline width="800"></video>

**[Overview](#overview) · [Features](#features) · [Theming](#theming) · [Gallery](#theme-gallery) · [Installation](#installation) · [Usage](#usage) · [Key bindings](#key-bindings) · [Configuration](#configuration) · [License](#license)**

</div>

---

## Overview

tuiX is a terminal spreadsheet editor written in C++17. It opens CSV and XLSX files
as a single native binary — no browser engine, no Node runtime, no background daemon.
Navigation and editing follow vim conventions, and a built-in formula engine evaluates
expressions on the fly.

It is developed on and for [Omarchy](https://omarchy.org) ([see gallery](#theme-gallery))
and works on any compatible Linux distribution. Prebuilt binaries are available for Linux
and macOS; Windows is not yet supported.

**Highlights**

- **Native and lightweight** — a single binary with a sub-second cold start and no runtime dependencies.
- **vim-style editing** — `h j k l`, `gg`, `G`, `0`, `$`, `/` search, modal editing, and `:` commands.
- **Formula engine** — 26 functions, cell references, and ranges, evaluated live.
- **Multi-sheet** — XLSX workbooks open with a tab per worksheet; cycle, add, rename, and delete them.
- **Live feedback** — per-cell formula suggestions and range statistics update as you work.
- **Theme-aware** — colors map to your terminal's ANSI palette, so tuiX adopts your theme automatically.

---

## Features

### Files and navigation

- Opens **CSV and XLSX** files. CSV delimiters (comma, semicolon, tab, pipe) are auto-detected; XLSX files load **every worksheet** into a row of tabs.
- vim-style movement: `h j k l` or arrow keys, `gg` / `G` for first/last row, `0` / `$` for first/last column, and `PgUp` / `PgDn` to page.
- **Incremental search** with `/` — matches are highlighted as you type, with a live match count; `n` / `N` step through results, and `Esc` restores your position.
- **Find & replace** across the whole sheet with `:s/old/new/` — case-sensitive, undoable (`u`), with a replacement count reported in the status bar.
- **Jump to any cell** by typing its address in command mode, e.g. `:B12`.

### Editing

- Press `i` or `a` to edit a cell or a column header. A **sticky INSERT mode** lets you type and move across cells without returning to NORMAL.
- `Enter` commits and moves down; `Tab` commits and moves right; `Esc` returns to NORMAL.
- Insert or delete **rows and columns** with `+` / `-` (in the row gutter or column header).
- Select a range with `Shift`+arrows, **yank** with `y`, and **paste** with `p`.
- A single undo/redo stack covers both cell edits and column renames (`u` / `Ctrl+R`).

### Sheets

- XLSX workbooks open with **one tab per worksheet**. Cycle them with `Ctrl+PgDn` / `Ctrl+PgUp`, or click a tab to jump straight to it.
- Add a sheet with `Ctrl+T` (or the `+` button on the tab bar). Click the **active** tab to rename or delete it.

### Formulas

Cells beginning with `=` are evaluated through a lexer → parser → evaluator pipeline that
supports cell references, ranges, and 26 functions:

```
=A1 + B2 * 3                     arithmetic & precedence
=SUM(A1:A10)                     ranges
=IF(C2 > 100, "hi", "lo")        conditionals
=IFERROR(A1 / B1, 0)             error handling
=SUMIF(D1:D9, ">100", C1:C9)     conditional aggregates
=VLOOKUP("banana", A1:C9, 2)     lookups
=INDEX(B1:B9, MATCH(99, A1:A9))  index / match
=ROUND(AVERAGE(B1:B5), 2)        nesting
```

#### Supported functions

| Category | Function | Signature | Description |
|---|---|---|---|
| **Math** | `ABS` | `ABS(number)` | Absolute value (removes sign) |
| | `INT` | `INT(number)` | Truncate to integer toward zero |
| | `MOD` | `MOD(number, divisor)` | Remainder after division |
| | `ROUND` | `ROUND(number, digits)` | Round to given decimal places |
| | `SQRT` | `SQRT(number)` | Square root |
| **Aggregate** | `SUM` | `SUM(range)` | Sum all numeric values in range |
| | `AVERAGE` | `AVERAGE(range)` | Arithmetic mean of numeric cells |
| | `COUNT` | `COUNT(range)` | Count cells that contain numbers |
| | `COUNTA` | `COUNTA(range)` | Count non-empty cells |
| | `MIN` | `MIN(range)` | Smallest numeric value in range |
| | `MAX` | `MAX(range)` | Largest numeric value in range |
| **Conditional aggregate** | `COUNTIF` | `COUNTIF(range, criterion)` | Count cells meeting a criterion |
| | `SUMIF` | `SUMIF(range, criterion, [sum_range])` | Sum cells meeting a criterion |
| | `AVERAGEIF` | `AVERAGEIF(range, criterion, [avg_range])` | Mean of cells meeting a criterion |
| **Logical** | `IF` | `IF(cond, true, false)` | Return one of two values based on a test |
| | `IFS` | `IFS(cond1, val1, …)` | First value whose condition is true |
| | `IFERROR` | `IFERROR(expr, fallback)` | Return fallback if expr errors |
| | `IFNA` | `IFNA(expr, fallback)` | Return fallback if expr is `#N/A` |
| **Lookup** | `VLOOKUP` | `VLOOKUP(key, range, col, [exact])` | Look up key in range's first column |
| | `MATCH` | `MATCH(key, range, [type])` | Position of key within a 1-D range |
| | `INDEX` | `INDEX(range, row, [col])` | Cell at a position within a range |
| **Text** | `CONCATENATE` | `CONCATENATE(text1, text2, …)` | Join two or more text strings |
| | `LEN` | `LEN(text)` | Number of characters in text |
| | `UPPER` | `UPPER(text)` | Convert text to upper case |
| | `LOWER` | `LOWER(text)` | Convert text to lower case |
| | `TRIM` | `TRIM(text)` | Remove leading/trailing whitespace |

Criteria for `COUNTIF` / `SUMIF` / `AVERAGEIF` accept a comparison operator
(`">10"`, `"<=5"`, `"<>x"`) or a plain value for equality; text matching is
case-insensitive.

Typing `=` followed by a function name opens an **autocomplete popup** with signatures and
descriptions — `↑` / `↓` to browse, `Tab` / `Enter` to complete. Circular references are
detected and flagged rather than left to hang.

### Live suggestions and statistics

- Landing on a cell shows a **suggestion bar** with the results of the most relevant formulas for that value:
  - On a **number** → `ABS` · `INT` · `SQRT` · `ROUND` · `LEN` · `UPPER` · `TRIM`
  - On a **text cell** → `LEN` · `UPPER` · `LOWER` · `TRIM`
- Selecting **multiple cells** with `Shift`+arrows switches the bar to live range statistics — `SUM`, `AVG`, `COUNT`, `COUNTA`, `MIN`, `MAX` — recalculated as the selection changes.

### Interface

- **Mouse support**, if you want it: click to select, use the `+` / `-` action boxes to insert or delete rows and columns, and drag the scrollbar or use the wheel to scroll.
- **Command line**: `:w` saves, `:w file.csv` saves as, `:wq` saves and quits, `:e other.csv` opens another file. The output format follows the extension (`.xlsx` or `.csv`). An overwrite-confirmation prompt prevents accidental clobbering.
- A **titlebar** provides Undo / Redo / Open / Save / Save As / Exit buttons.
- `F1` opens a tabbed keybinding reference; `F12` opens a live configuration editor that writes changes to `config.toml`.

---

## Theming

tuiX never hardcodes RGB values. Every color it draws — cursor, selection, headers, mode
badges, formulas — references one of your terminal's **16 ANSI palette slots** (or a named
color / index `0–15`).

As a result, tuiX **inherits your terminal theme automatically**. On
[Omarchy](https://omarchy.org), switching your theme changes the terminal palette, and tuiX
follows — no application-specific theme files to maintain. Individual accents can still be
overridden; see [Configuration](#configuration).

---

## Theme gallery

tuiX on all 20 built-in Omarchy themes — the same app, your palette.

<div style="overflow-x: auto; scroll-snap-type: x mandatory; -webkit-overflow-scrolling: touch; display: flex; gap: 10px; padding: 10px 0;">
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Aether_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Aether">
    <p><strong>Aether</strong> &nbsp;·&nbsp; 1 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Catppuccin_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Catppuccin">
    <p><strong>Catppuccin</strong> &nbsp;·&nbsp; 2 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Catppuccin_Latte_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Catppuccin Latte">
    <p><strong>Catppuccin Latte</strong> &nbsp;·&nbsp; 3 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Ethereal_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Ethereal">
    <p><strong>Ethereal</strong> &nbsp;·&nbsp; 4 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Everforest_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Everforest">
    <p><strong>Everforest</strong> &nbsp;·&nbsp; 5 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Flexoki_Light_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Flexoki Light">
    <p><strong>Flexoki Light</strong> &nbsp;·&nbsp; 6 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Gruvbox_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Gruvbox">
    <p><strong>Gruvbox</strong> &nbsp;·&nbsp; 7 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Hackerman_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Hackerman">
    <p><strong>Hackerman</strong> &nbsp;·&nbsp; 8 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Kanagawa_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Kanagawa">
    <p><strong>Kanagawa</strong> &nbsp;·&nbsp; 9 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Lumon_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Lumon">
    <p><strong>Lumon</strong> &nbsp;·&nbsp; 10 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Matte_Black_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Matte Black">
    <p><strong>Matte Black</strong> &nbsp;·&nbsp; 11 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Miasma_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Miasma">
    <p><strong>Miasma</strong> &nbsp;·&nbsp; 12 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Nord_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Nord">
    <p><strong>Nord</strong> &nbsp;·&nbsp; 13 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Osaka_Jade_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Osaka Jade">
    <p><strong>Osaka Jade</strong> &nbsp;·&nbsp; 14 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Retro_82_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Retro 82">
    <p><strong>Retro 82</strong> &nbsp;·&nbsp; 15 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Ristretto_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Ristretto">
    <p><strong>Ristretto</strong> &nbsp;·&nbsp; 16 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Rose_Pine_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Rose Pine">
    <p><strong>Rose Pine</strong> &nbsp;·&nbsp; 17 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Tokyo_Night_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Tokyo Night">
    <p><strong>Tokyo Night</strong> &nbsp;·&nbsp; 18 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/Vantablack_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="Vantablack">
    <p><strong>Vantablack</strong> &nbsp;·&nbsp; 19 / 20</p>
  </div>
  <div style="scroll-snap-align: center; flex: 0 0 100%; text-align: center;">
    <img src="https://github.com/paolopangrazi/hub/raw/main/images/White_tuix.jpg" style="max-width: 100%; height: auto; border-radius: 8px;" alt="White">
    <p><strong>White</strong> &nbsp;·&nbsp; 20 / 20</p>
  </div>
</div>

---

## Installation

Prebuilt binaries are available for **Linux (x86_64)** and **macOS (Apple Silicon)**.
Windows is not yet supported — on Windows or any other platform, build from source.

### Download a release

Get the latest archive from the [**Releases**](https://github.com/paolopangrazi/tuix/releases)
page, or from the command line (set `VERSION` to the current release):

```bash
VERSION=v0.1.0

# Linux (x86_64)
curl -LO https://github.com/paolopangrazi/tuix/releases/download/$VERSION/tuix-$VERSION-linux-x86_64.tar.gz
tar -xzf tuix-$VERSION-linux-x86_64.tar.gz
install -Dm755 tuix-$VERSION-linux-x86_64/bin/tuix ~/.local/bin/tuix

# macOS (Apple Silicon)
curl -LO https://github.com/paolopangrazi/tuix/releases/download/$VERSION/tuix-$VERSION-macos-arm64.tar.gz
tar -xzf tuix-$VERSION-macos-arm64.tar.gz
install -Dm755 tuix-$VERSION-macos-arm64/bin/tuix ~/.local/bin/tuix
```

Make sure `~/.local/bin` is on your `PATH`. Each release also ships a `SHA256SUMS` file so
you can verify the download:

```bash
curl -LO https://github.com/paolopangrazi/tuix/releases/download/$VERSION/SHA256SUMS
sha256sum --check --ignore-missing SHA256SUMS   # macOS: shasum -a 256 -c SHA256SUMS
```

> **macOS:** if Gatekeeper blocks the unsigned binary, clear the quarantine flag with
> `xattr -d com.apple.quarantine ~/.local/bin/tuix`.

### Build from source

**Requirements:** CMake 3.14+, a C++17 compiler (GCC 8+ / Clang 7+), and Git. Every
dependency — [FTXUI](https://github.com/ArthurSonzogni/FTXUI),
[rapidcsv](https://github.com/d99kris/rapidcsv), [toml++](https://github.com/marzer/tomlplusplus),
and [OpenXLSX](https://github.com/troldal/OpenXLSX) — is vendored as a git submodule. No
system packages are required.

```bash
git clone --recurse-submodules https://github.com/paolopangrazi/tuix
cd tuix
cmake -B build
cmake --build build
./build/tuix samples/csv/employees.csv   # try it out
```

> Already cloned without submodules? Run `git submodule update --init --recursive` first.

Install it onto your `PATH`:

```bash
cmake --install build --prefix ~/.local   # → ~/.local/bin/tuix
```

---

## Usage

```bash
tuix                        # start with a blank sheet
tuix path/to/file.csv       # open a CSV
tuix path/to/file.xlsx      # open an Excel file
```

---

## Key bindings

### Navigate (NORMAL mode)

| Key | Action |
|---|---|
| `h` `j` `k` `l` / arrows | Move left / down / up / right |
| `gg` / `G` | Jump to first / last row |
| `0` `Home` / `$` `End` | Jump to first / last column |
| `PgUp` / `PgDn` | Scroll one page |
| `↑` from row 0 | Enter the column header |
| `/` | Search — type to filter, `Enter` to confirm, `Esc` to cancel |
| `n` / `N` | Jump to next / previous match |

### Edit

| Key | Action |
|---|---|
| `i` / `a` | Edit cell · rename column header (→ INSERT) |
| `Esc` | Back to NORMAL |
| `o` / `O` | Insert row below / above and edit |
| `Enter` | Commit & move down |
| `Tab` / `Shift+Tab` | Commit & move right / left |
| `x` / `Backspace` | Clear cell · delete row (gutter) · delete column (header) |

### Select, clipboard & structure

| Key | Action |
|---|---|
| `Shift`+arrows | Select a range |
| `y` / `p` | Yank / paste |
| `+` / `-` | Insert / delete row (gutter) or column (header) |

### Sheets (XLSX)

| Key | Action |
|---|---|
| `Ctrl+PgDn` / `Ctrl+PgUp` | Cycle to next / previous sheet |
| `Ctrl+T` | Add a new sheet |
| Click tab · click active tab | Switch · rename / delete |

### Mouse

| Action | Result |
|---|---|
| Click cell | Move cursor to that cell |
| Click column header | Select that column header |
| Wheel up / down | Scroll three rows |
| Click / drag scrollbar | Scroll to that position |
| Click `+` / `-` | Insert / delete row or column |

### Formulas, history & app

| Key | Action |
|---|---|
| `=` | Start a formula (opens autocomplete) |
| `↑`/`↓`, `Tab`/`Enter` | Browse / complete a formula |
| `u` / `Ctrl+R` | Undo / redo |
| `:` | Command mode — `:w`, `:w file`, `:wq`, `:q`, `:q!`, `:e file`, `:s/old/new/` (find & replace), `:B12` (jump to cell) |
| `F1` / `F12` | Help · live config editor |
| `Ctrl+E` | Toggle exit confirmation |

---

## Configuration

tuiX reads `~/.config/tuix/config.toml` at startup (XDG-compliant — it respects
`$XDG_CONFIG_HOME`). Every setting is optional; missing keys fall back to sensible defaults.
Colors accept an **ANSI name** or a **palette index `0–15`**, which is what allows tuiX to
track your terminal theme.

```toml
[colors]
# ANSI name (black, red, green, yellow, blue, magenta, cyan, white,
# gray_dark, gray_light, *_light variants) — or a palette index 0–15
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
cell_width = 12   # minimum column width (min 4)
```

> **Tip:** leave the colors as palette names and let your Omarchy theme drive them. Only pin
> a value here if you want tuiX to deviate from your terminal's palette.

---

## License

MIT — see [LICENSE](LICENSE).
