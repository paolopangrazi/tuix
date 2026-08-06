#pragma once
#include <string>
#include <vector>

#include "cell.hpp"
#include "history.hpp"

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
    std::vector<HistoryEntry>              undo_stack;
    std::vector<HistoryEntry>              redo_stack;
};

// Ensures at least one row and one column, and that col_names/col_widths/
// col_manual/row_heights all match cells' dimensions — used after building a
// sheet from a source that may be empty or ragged (a file with no rows, a
// legacy snapshot predating one of these fields, ...) so downstream code
// never has to special-case 0 rows/cols. Shared by Grid::load_from (which
// holds these as separate members, not a Sheet) and session.cpp's
// sheet_from_data.
void normalize_sheet_dims(std::vector<std::vector<Cell>>& cells,
                           std::vector<std::string>& col_names,
                           std::vector<int>& col_widths,
                           std::vector<bool>& col_manual,
                           std::vector<int>& row_heights,
                           int default_col_width);
