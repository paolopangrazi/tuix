// grid_search.cpp — `/` incremental search, n/N stepping, :goto, and
// replace-all. (Part of the Grid class; see grid.hpp.)
#include "grid.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "col_label.hpp"

bool Grid::goto_ref(const std::string& a1) {
    auto rc = parse_a1(a1);
    if (!rc) return false;
    const auto [r, c] = *rc;
    if (r < 0 || r >= m_rows || c < 0 || c >= m_cols) return false;
    commit_edit();
    m_has_selection = false;
    m_cursor_row = r;
    m_cursor_col = c;
    adjust_viewport();
    return true;
}

int Grid::replace_all(const std::string& find, const std::string& repl) {
    if (find.empty()) return 0;
    commit_edit();

    int total = 0;   // occurrences replaced
    int cells = 0;   // distinct cells touched
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c) {
            const std::string before = m_cells[r][c].value();
            std::string after = before;
            int hits = 0;
            for (size_t pos = after.find(find); pos != std::string::npos;
                 pos = after.find(find, pos + repl.size())) {
                after.replace(pos, find.size(), repl);
                ++hits;
            }
            if (hits == 0) continue;
            m_undo_stack.push_back({r, c, before, after});
            m_cells[r][c].set_value(after);
            total += hits;
            ++cells;
        }

    if (total > 0) m_redo_stack.clear();
    m_status_msg = total == 0
        ? "no matches for \"" + find + "\""
        : std::to_string(total) + (total == 1 ? " replacement" : " replacements")
              + " in " + std::to_string(cells) + (cells == 1 ? " cell" : " cells");
    return total;
}

void Grid::recompute_hits(const std::string& q) {
    m_search_hits.clear();
    if (q.empty()) return;
    std::string needle = q;
    for (char& ch : needle) ch = (char)std::tolower((unsigned char)ch);
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c) {
            std::string hay = cell_display(r, c);
            for (char& ch : hay) ch = (char)std::tolower((unsigned char)ch);
            if (hay.find(needle) != std::string::npos)
                m_search_hits.emplace_back(r, c);   // row-major → already sorted
        }
}

void Grid::start_search() {
    commit_edit();
    m_has_selection     = false;
    m_searching         = true;
    m_search_buf.clear();
    m_search_origin_row = std::max(0, m_cursor_row);
    m_search_origin_col = std::max(0, m_cursor_col);
}

void Grid::update_search() {
    m_search_query = m_search_buf;
    recompute_hits(m_search_query);
    if (m_search_hits.empty()) return;
    // Jump to the first match at or after the anchor the search began on.
    const std::pair<int, int> anchor{m_search_origin_row, m_search_origin_col};
    auto it = std::lower_bound(m_search_hits.begin(), m_search_hits.end(), anchor);
    if (it == m_search_hits.end()) it = m_search_hits.begin();
    m_cursor_row = it->first;
    m_cursor_col = it->second;
    adjust_viewport();
}

void Grid::commit_search() {
    m_searching    = false;
    m_search_query = m_search_buf;
    recompute_hits(m_search_query);   // keep highlights live for n/N
}

void Grid::cancel_search() {
    m_searching = false;
    m_search_buf.clear();
    m_search_query.clear();
    m_search_hits.clear();
    m_cursor_row = m_search_origin_row;
    m_cursor_col = m_search_origin_col;
    adjust_viewport();
}

void Grid::search_step(int dir) {
    recompute_hits(m_search_query);
    if (m_search_hits.empty()) return;
    const std::pair<int, int> cur{m_cursor_row, m_cursor_col};
    std::pair<int, int> target;
    if (dir > 0) {   // n: first match after the cursor, wrapping to the top
        auto it = std::upper_bound(m_search_hits.begin(), m_search_hits.end(), cur);
        if (it == m_search_hits.end()) it = m_search_hits.begin();
        target = *it;
    } else {         // N: last match before the cursor, wrapping to the bottom
        auto it = std::lower_bound(m_search_hits.begin(), m_search_hits.end(), cur);
        if (it == m_search_hits.begin()) it = m_search_hits.end();
        target = *--it;
    }
    m_has_selection = false;
    m_cursor_row = target.first;
    m_cursor_col = target.second;
    adjust_viewport();
}
