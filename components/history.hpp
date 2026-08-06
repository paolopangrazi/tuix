#pragma once
#include <string>
#include <vector>

// A history step is either a single-cell edit or a whole-row reorder (sort).
// For Reorder, `order` is the permutation that was applied: new[i] = old[order[i]];
// undo applies its inverse, and row/col/before/after are unused.
enum class HistoryKind { Cell, Reorder };
struct HistoryEntry {
    HistoryEntry() = default;
    HistoryEntry(int r, int c, std::string b, std::string a)   // a cell edit
        : row(r), col(c), before(std::move(b)), after(std::move(a)) {}

    int row = 0, col = 0;
    std::string before, after;
    HistoryKind kind = HistoryKind::Cell;
    std::vector<int> order;
};

// One sort criterion: column index + ascending/descending.
struct SortKey { int col; bool descending; };
