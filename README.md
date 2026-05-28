<div align="center">

# ⚡ tuiX

### *tui eXcel-lent* — the spreadsheet that lives in your terminal, and feels instant.

[![CI](https://github.com/paolopangrazi/tuix/actions/workflows/ci.yml/badge.svg)](https://github.com/paolopangrazi/tuix/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C.svg)](https://en.cppreference.com/w/cpp/17)
[![Built with FTXUI](https://img.shields.io/badge/built%20with-FTXUI-8A2BE2.svg)](https://github.com/ArthurSonzogni/FTXUI)

*Open a CSV. Fly through it with vim keys. Write formulas. Undo anything. Never touch the mouse — unless you want to.*

![demo](screenshots/Hackerman_tuix.jpg)

</div>

---

## Why tuiX?

Opening a CSV shouldn't mean waiting on a 300 MB Electron app to boot, or fighting a web grid that lags one keystroke behind your fingers. tuiX is a **native C++ binary** built on [FTXUI](https://github.com/ArthurSonzogni/FTXUI). It launches in the time it takes your terminal to draw a frame, and **every single keystroke lands immediately** — no spinners, no debounce, no "syncing…", no flicker.

- 🚀 **Blazingly fast — always.** A single native binary with a cold start measured in milliseconds. Files are on screen before you finish blinking. Every action — navigation, editing, undo, formula evaluation — is **instantaneous**.
- ⌨️ **Zero-latency editing.** Type, navigate, undo — the grid redraws on every event with no perceptible delay, ever. Your terminal *is* the UI; there is nothing heavier between you and your data.
- 💡 **Immediate feedback everywhere.** Formula suggestions appear the instant you land on a cell. Range statistics update live on every keypress as you extend a selection. You never wait for tuiX.
- 🪶 **Featherweight.** No runtime dependencies, no Node, no browser engine, no background daemon. Just a binary and your CSV.
- 🎨 **Theme-native.** tuiX paints itself from your terminal's ANSI palette, so it adopts your [Omarchy](https://omarchy.org) theme automatically — switch themes, and tuiX follows. (More below.)
- 🧠 **vim muscle memory.** `h j k l`, `gg`, `G`, `0`, `$`, modes, `:w`, `:q` — if your fingers know vim, they already know tuiX.

If you live in the terminal, tuiX is the spreadsheet that finally feels like it belongs there.

---

## What you can actually do

### 📂 Open and move through data — instantly
Point tuiX at a CSV and it's on screen **before you blink**. Glide around with `h j k l` or arrow keys, jump to the top/bottom with `gg` / `G`, snap to the first/last column with `0` / `$`, and page through big files with `PgUp` / `PgDn`. Delimiters (comma, semicolon, tab, pipe) are detected for you — no configuration, no delay.

### ✏️ Edit like you mean it — with zero friction
Press `i` or `a` to start typing into a cell. The response is **immediate**: no input lag, no waiting for the cursor to appear. tuiX has a **sticky INSERT mode** — like a real spreadsheet, you can keep typing and arrowing from cell to cell without dropping back to navigation. `Enter` commits and moves down, `Tab` commits and moves right. `Esc` drops you back to NORMAL. Column **headers behave exactly like cells** — edit them the same way, with the same undo.

### ↩️ Undo & redo — instantly
Every edit — cell values *and* column renames — is on a single undo stack. `u` to undo, `Ctrl+R` to redo, and the grid snaps back **immediately**. Experiment freely; you can always walk it back.

### 🧮 Real formulas — evaluated on the spot
Start any cell with `=` and tuiX evaluates it **live, right away**. A full lexer → parser → evaluator pipeline backs **18 functions**, cell references, and ranges:

```
=A1 + B2 * 3                 arithmetic & precedence
=SUM(A1:A10)                 ranges
=IF(C2 > 100, "hi", "lo")    conditionals
=IFERROR(A1 / B1, 0)         error handling
=ROUND(AVERAGE(B1:B5), 2)    nesting
```

> **Functions:** `ABS` · `AVERAGE` · `CONCATENATE` · `COUNT` · `COUNTA` · `IF` · `IFERROR` · `INT` · `LEN` · `LOWER` · `MAX` · `MIN` · `MOD` · `ROUND` · `SQRT` · `SUM` · `TRIM` · `UPPER`

Type `=` and a name, and an **autocomplete popup** appears **instantly** with signatures and descriptions — `↑`/`↓` to browse, `Tab`/`Enter` to complete. Circular references are detected and flagged instead of hanging.

### 💡 Smart formula suggestions — always ready, always instant

Land on a cell and a **suggestion bar** appears below the grid **immediately**, showing you what the most useful formulas would produce *right now*, without you having to type anything:

- On a **number** → `ABS` · `INT` · `SQRT` · `ROUND` · `LEN` · `UPPER` · `TRIM`
- On a **text cell** → `LEN` · `UPPER` · `LOWER` · `TRIM`

Select **multiple cells** with `Shift`+arrows and the bar switches to **live range statistics** — `SUM`, `AVG`, `COUNT`, `COUNTA`, `MIN`, `MAX` — **recalculated on every single keystroke** as your selection grows or shrinks. No configuration, no extra step, no delay: select cells, see the numbers.

### 📐 Reshape your sheet — immediately
Insert and delete **rows and columns** on the fly with `+` / `-` (in the row-index gutter for rows, in the header for columns) — the grid reflows **instantly**. Select a region with `Shift`+arrows, **yank** it with `y`, and **paste** it elsewhere with `p`.

### 🖱️ Mouse, if you want it
Don't feel like reaching for the keyboard? Click any cell to select it — **it responds immediately**. Click the little `+`/`-` action boxes on rows and columns to insert/delete. Drag the scrollbar or spin the wheel to scroll. tuiX is keyboard-first, not keyboard-only.

### 💾 Save without leaving home row
`:w` saves, `:w newname.csv` saves as, `:wq` saves and quits, `:e other.csv` opens another file — all from the command line, vim-style, **with instant execution**. There's a titlebar with **Undo / Redo / Open / Save / Save As / Exit** buttons too, and an overwrite-confirmation prompt so you never clobber a file by accident.

### 🆘 Help & live config, built in
Hit `F1` for a tabbed keybinding reference, and `F2` for a **live configuration editor** that writes your changes straight to `config.toml` — changes take effect the next time you launch.

---

## 🎨 Made for Omarchy (and any themed terminal)

tuiX never hardcodes RGB values. Every color it draws — cursor, selection, headers, mode badges, formulas — is a reference to one of your terminal's **16 ANSI palette slots** (or a named color / index `0–15`).

That means tuiX **inherits your terminal theme for free**. On [Omarchy](https://omarchy.org), when you switch your selected theme, your terminal's palette changes — and tuiX instantly wears the same colors as the rest of your desktop. No tuiX-specific theme files to maintain, no mismatch between your editor and your spreadsheet. It just blends in, perfectly, with whatever you're running.

Want to override a specific accent anyway? You can — see [Configuration](#configuration).

---

## Theme gallery

tuiX on all 20 built-in Omarchy themes — same app, your palette.

---

<a id="aether"></a>

<div align="center">

### [← White](#white) &nbsp;&nbsp;&nbsp;&nbsp; 1 / 20 · **Aether** &nbsp;&nbsp;&nbsp;&nbsp; [Catppuccin →](#catppuccin)

<img src="screenshots/Aether_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="catppuccin"></a>

<div align="center">

### [← Aether](#aether) &nbsp;&nbsp;&nbsp;&nbsp; 2 / 20 · **Catppuccin** &nbsp;&nbsp;&nbsp;&nbsp; [Catppuccin Latte →](#catppuccin-latte)

<img src="screenshots/Catppuccin_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="catppuccin-latte"></a>

<div align="center">

### [← Catppuccin](#catppuccin) &nbsp;&nbsp;&nbsp;&nbsp; 3 / 20 · **Catppuccin Latte** &nbsp;&nbsp;&nbsp;&nbsp; [Ethereal →](#ethereal)

<img src="screenshots/Catppuccin_Latte_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="ethereal"></a>

<div align="center">

### [← Catppuccin Latte](#catppuccin-latte) &nbsp;&nbsp;&nbsp;&nbsp; 4 / 20 · **Ethereal** &nbsp;&nbsp;&nbsp;&nbsp; [Everforest →](#everforest)

<img src="screenshots/Ethereal_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="everforest"></a>

<div align="center">

### [← Ethereal](#ethereal) &nbsp;&nbsp;&nbsp;&nbsp; 5 / 20 · **Everforest** &nbsp;&nbsp;&nbsp;&nbsp; [Flexoki Light →](#flexoki-light)

<img src="screenshots/Everforest_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="flexoki-light"></a>

<div align="center">

### [← Everforest](#everforest) &nbsp;&nbsp;&nbsp;&nbsp; 6 / 20 · **Flexoki Light** &nbsp;&nbsp;&nbsp;&nbsp; [Gruvbox →](#gruvbox)

<img src="screenshots/Flexoki_Light_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="gruvbox"></a>

<div align="center">

### [← Flexoki Light](#flexoki-light) &nbsp;&nbsp;&nbsp;&nbsp; 7 / 20 · **Gruvbox** &nbsp;&nbsp;&nbsp;&nbsp; [Hackerman →](#hackerman)

<img src="screenshots/Gruvbox_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="hackerman"></a>

<div align="center">

### [← Gruvbox](#gruvbox) &nbsp;&nbsp;&nbsp;&nbsp; 8 / 20 · **Hackerman** &nbsp;&nbsp;&nbsp;&nbsp; [Kanagawa →](#kanagawa)

<img src="screenshots/Hackerman_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="kanagawa"></a>

<div align="center">

### [← Hackerman](#hackerman) &nbsp;&nbsp;&nbsp;&nbsp; 9 / 20 · **Kanagawa** &nbsp;&nbsp;&nbsp;&nbsp; [Lumon →](#lumon)

<img src="screenshots/Kanagawa_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="lumon"></a>

<div align="center">

### [← Kanagawa](#kanagawa) &nbsp;&nbsp;&nbsp;&nbsp; 10 / 20 · **Lumon** &nbsp;&nbsp;&nbsp;&nbsp; [Matte Black →](#matte-black)

<img src="screenshots/Lumon_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="matte-black"></a>

<div align="center">

### [← Lumon](#lumon) &nbsp;&nbsp;&nbsp;&nbsp; 11 / 20 · **Matte Black** &nbsp;&nbsp;&nbsp;&nbsp; [Miasma →](#miasma)

<img src="screenshots/Matte_Black_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="miasma"></a>

<div align="center">

### [← Matte Black](#matte-black) &nbsp;&nbsp;&nbsp;&nbsp; 12 / 20 · **Miasma** &nbsp;&nbsp;&nbsp;&nbsp; [Nord →](#nord)

<img src="screenshots/Miasma_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="nord"></a>

<div align="center">

### [← Miasma](#miasma) &nbsp;&nbsp;&nbsp;&nbsp; 13 / 20 · **Nord** &nbsp;&nbsp;&nbsp;&nbsp; [Osaka Jade →](#osaka-jade)

<img src="screenshots/Nord_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="osaka-jade"></a>

<div align="center">

### [← Nord](#nord) &nbsp;&nbsp;&nbsp;&nbsp; 14 / 20 · **Osaka Jade** &nbsp;&nbsp;&nbsp;&nbsp; [Retro 82 →](#retro-82)

<img src="screenshots/Osaka_Jade_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="retro-82"></a>

<div align="center">

### [← Osaka Jade](#osaka-jade) &nbsp;&nbsp;&nbsp;&nbsp; 15 / 20 · **Retro 82** &nbsp;&nbsp;&nbsp;&nbsp; [Ristretto →](#ristretto)

<img src="screenshots/Retro_82_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="ristretto"></a>

<div align="center">

### [← Retro 82](#retro-82) &nbsp;&nbsp;&nbsp;&nbsp; 16 / 20 · **Ristretto** &nbsp;&nbsp;&nbsp;&nbsp; [Rose Pine →](#rose-pine)

<img src="screenshots/Ristretto_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="rose-pine"></a>

<div align="center">

### [← Ristretto](#ristretto) &nbsp;&nbsp;&nbsp;&nbsp; 17 / 20 · **Rose Pine** &nbsp;&nbsp;&nbsp;&nbsp; [Tokyo Night →](#tokyo-night)

<img src="screenshots/Rose_Pine_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="tokyo-night"></a>

<div align="center">

### [← Rose Pine](#rose-pine) &nbsp;&nbsp;&nbsp;&nbsp; 18 / 20 · **Tokyo Night** &nbsp;&nbsp;&nbsp;&nbsp; [Vantablack →](#vantablack)

<img src="screenshots/Tokyo_Night_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="vantablack"></a>

<div align="center">

### [← Tokyo Night](#tokyo-night) &nbsp;&nbsp;&nbsp;&nbsp; 19 / 20 · **Vantablack** &nbsp;&nbsp;&nbsp;&nbsp; [White →](#white)

<img src="screenshots/Vantablack_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

<a id="white"></a>

<div align="center">

### [← Vantablack](#vantablack) &nbsp;&nbsp;&nbsp;&nbsp; 20 / 20 · **White** &nbsp;&nbsp;&nbsp;&nbsp; [Aether →](#aether)

<img src="screenshots/White_tuix.jpg" width="92%"/>

<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>

</div>

---

## Install & build

**Requirements:** CMake 3.14+, a C++17 compiler (GCC 8+ / Clang 7+), and Git. Every dependency — [FTXUI](https://github.com/ArthurSonzogni/FTXUI), [rapidcsv](https://github.com/d99kris/rapidcsv), and [toml++](https://github.com/marzer/tomlplusplus) — is vendored as a git submodule. **No system packages required.**

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

```bash
tuix                      # start with a blank sheet
tuix path/to/file.csv     # open a CSV
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

### Formulas, history & app

| Key | Action |
|---|---|
| `=` | Start a formula (opens autocomplete) |
| `↑`/`↓`, `Tab`/`Enter` | Browse / complete a formula |
| `u` / `Ctrl+R` | Undo / redo |
| `:` | Command mode — `:w`, `:w file`, `:wq`, `:q`, `:q!`, `:e file` |
| `F1` / `F2` | Help · live config editor |
| `Ctrl+E` | Toggle exit confirmation |

---

## Configuration

tuiX reads `~/.config/tuix/config.toml` at startup (XDG-compliant — respects `$XDG_CONFIG_HOME`). Every setting is optional; missing keys fall back to sensible defaults. Colors accept an **ANSI name** or a **palette index `0–15`**, which is exactly what lets tuiX track your terminal theme.

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

> 💡 Tip: leave the colors as palette names and let your Omarchy theme drive them. Only pin a value here if you want tuiX to deviate from your terminal's palette.

---

## License

MIT — see [LICENSE](LICENSE).

<div align="center">

**Built for people who never leave the terminal.**

</div>
