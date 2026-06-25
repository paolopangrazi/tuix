#pragma once
#include <string>
#include <vector>

#include "cell.hpp"
#include "grid.hpp"

// Self-contained snapshot of one sheet's editable state.
// A Workbook owns many of these; Grid is a live "view" into one.
class Sheet {
  public:
    std::string                            name;
    std::vector<std::vector<Cell>>         cells;        // [rows][cols]
    std::vector<std::string>               col_names;
    std::vector<int>                       col_widths;
    std::vector<bool>                      col_manual;   // user-pinned column widths
    std::vector<int>                       row_heights;  // per-row height in lines
    int                                    cursor_row = 0;
    int                                    cursor_col = 0;
    int                                    offset_row = 0;
    int                                    offset_col = 0;
    std::vector<Grid::HistoryEntry>        undo_stack;
    std::vector<Grid::HistoryEntry>        redo_stack;
};
