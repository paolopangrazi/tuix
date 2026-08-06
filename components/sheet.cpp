#include "sheet.hpp"

#include <algorithm>

#include "util/col_label.hpp"

void normalize_sheet_dims(std::vector<std::vector<Cell>>& cells,
                           std::vector<std::string>& col_names,
                           std::vector<int>& col_widths,
                           std::vector<bool>& col_manual,
                           std::vector<int>& row_heights,
                           int default_col_width) {
    int rows = static_cast<int>(cells.size());
    int cols = rows ? static_cast<int>(cells[0].size()) : static_cast<int>(col_names.size());
    if (rows == 0) {
        rows = 1;
        cells.assign(1, std::vector<Cell>(std::max(1, cols)));
    }
    if (cols == 0) {
        cols = 1;
        for (auto& row : cells) row.assign(1, Cell{});
        col_names.assign(1, col_letter(0));
        col_widths.assign(1, default_col_width);
    }
    col_names.resize(cols);
    col_widths.resize(cols, default_col_width);
    col_manual.resize(cols, false);
    row_heights.resize(rows, 1);
}
