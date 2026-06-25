#pragma once
#include <string>
#include <vector>

#include "cell.hpp"
#include "grid.hpp"

// Self-contained snapshot of one sheet's editable state.
// A Workbook owns many of these; Grid is a live "view" into one.
struct Sheet {
    std::string                            name;
    std::vector<std::vector<Cell>>         cells;        // [rows][cols]
    std::vector<std::string>               col_names;
    std::vector<int>                       col_widths;
    std::vector<bool>                      col_manual;   // user-pinned column widths
    int                                    cursor_row = 0;
    int                                    cursor_col = 0;
    int                                    offset_row = 0;
    int                                    offset_col = 0;
    std::vector<Grid::HistoryEntry>        undo_stack;
    std::vector<Grid::HistoryEntry>        redo_stack;
};
