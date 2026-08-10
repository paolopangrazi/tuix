#include "tips_catalog.hpp"

namespace tuix {

const std::vector<Tip>& tips() {
    static const std::vector<Tip> k_tips = {
        { "Two modes, like vi",
          "i  a  F2  /  Esc",
          "tuix starts in NORMAL mode, where the letter keys navigate and act on "
          "cells. Press i or a to type into the current cell (INSERT mode), and "
          "Esc to come back." },
        { "Move without arrow keys",
          "h  j  k  l",
          "The vi keys move the cursor in NORMAL mode. gg and G jump to the first "
          "and last row; 0 / $ (or Home / End) to the first and last column." },
        { "The command line",
          ":",
          "Press : for vi-style commands — :w saves, :w path saves as, :e path "
          "opens, :q quits and :q! quits without asking." },
        { "Jump to a cell",
          ":B12",
          "A bare A1-style reference on the command line moves the cursor there, "
          "so :B12 takes you straight to column B, row 12." },
        { "Find and replace",
          "/  n  N  /  :s/old/new/",
          "/ starts an incremental search and n / N step through the matches. To "
          "rewrite every match at once, use :s/old/new/." },
        { "Formulas with autocomplete",
          "=",
          "A cell starting with = is a formula. Typing = opens a searchable "
          "function list — ↑ / ↓ to pick, Tab or Enter to complete." },
        { "Other sheets are one ! away",
          "=Sheet2!A1",
          "Formulas can reference another sheet by name, as in =SUM(Sheet2!A1:A9). "
          "Sheet names are matched case-insensitively." },
        { "Copy between apps",
          "y  /  p",
          "y yanks the current cell or selection to the system clipboard and p "
          "pastes it back, so a range can travel to and from other programs." },
        { "See the shape of a column",
          "H  /  c",
          "H shades numeric cells as a heatmap. c opens the chart panel and cycles "
          "bar, line and histogram over the selection or column." },
        { "Sort from the header",
          "s",
          "Move up into the column header and press s (or click the header) to "
          "sort by that column; press again to flip the direction. u undoes it." },
        { "Undo covers structure too",
          "u  /  Ctrl+R",
          "u undoes and Ctrl+R redoes — not just cell edits but column renames and "
          "whole sorts, which are undone as a single step." },
        { "Workbooks with many sheets",
          "Ctrl+PgUp / Ctrl+PgDn / Ctrl+T",
          "Opening an .xlsx file shows a tab per sheet. Ctrl+PgUp / Ctrl+PgDn "
          "cycle sheets, Ctrl+T adds one, and clicking the active tab renames or "
          "deletes it." },
        { "The mouse works",
          "click · drag · wheel",
          "Click a cell to move there, drag to select a range, drag a column or "
          "row border to resize it, and use the wheel to scroll." },
        { "Make it yours",
          "F12",
          "The configuration editor edits ~/.config/tuix/config.toml — colors, "
          "themes and keybindings — and merges your changes into the file." },
        { "tuix in a pipeline",
          "tuix --filter ...",
          "Outside the TUI, tuix is a CSV/XLSX filter: "
          "tuix --filter 'salary > 50000' --sort dept --select name,dept in.csv" },
        { "More where this came from",
          "F1  /  F3",
          "F1 opens the full keybinding reference, grouped by topic. F3 brings "
          "these tips back at any time." },
    };
    return k_tips;
}

}  // namespace tuix
