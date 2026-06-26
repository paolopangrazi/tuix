// grid_view.cpp — cursor navigation, selection geometry, viewport scrolling,
// and column-width / row-height sizing. (Part of the Grid class; see grid.hpp.)
#include "grid.hpp"

#include <algorithm>
#include <string>

#include <ftxui/screen/terminal.hpp>

#include "col_label.hpp"

using namespace ftxui;

// ── Cursor movement ──────────────────────────────────────────────────────────

void Grid::move(int dr, int dc) {
    m_status_msg.clear();                                    // dismiss transient message
    int nr = std::clamp(m_cursor_row + dr, -1, m_rows - 1);  // row -1 = header
    int nc = std::clamp(m_cursor_col + dc, -1, m_cols - 1);  // col -1 = row index
    // There is no cell at the (header, row-index) corner; stay on the axis we came from.
    if (nr < 0 && nc < 0) {
        if (m_cursor_col < 0) nr = m_cursor_row;  // moving up the row-index gutter: don't enter header
        else                  nc = m_cursor_col;  // moving left along the header: don't enter row index
    }
    m_cursor_row = nr;
    m_cursor_col = nc;
    adjust_viewport();
}

void Grid::move_home()  { m_cursor_col = 0;          adjust_viewport(); }
void Grid::move_end()   { m_cursor_col = m_cols - 1; adjust_viewport(); }
void Grid::page_up()    { move(-vis_rows(), 0); }
void Grid::page_down()  { move( vis_rows(), 0); }

// ── Selection ────────────────────────────────────────────────────────────────

bool Grid::in_selection(int r, int c) const noexcept {
    if (!m_has_selection || m_cursor_row < 0 || m_cursor_col < 0) return false;
    const int r0 = std::min(m_cursor_row, m_sel_row), r1 = std::max(m_cursor_row, m_sel_row);
    const int c0 = std::min(m_cursor_col, m_sel_col), c1 = std::max(m_cursor_col, m_sel_col);
    return r >= r0 && r <= r1 && c >= c0 && c <= c1;
}

std::string Grid::cursor_label() const {
    if (m_cursor_row < 0) return "Col " + col_letter(m_cursor_col);
    if (m_cursor_col < 0) return "Row " + std::to_string(m_cursor_row + 1);
    return m_col_names[m_cursor_col] + std::to_string(m_cursor_row + 1);
}

void Grid::extend_selection(int dr, int dc) {
    commit_edit();
    if (!m_has_selection) {
        m_sel_row = m_cursor_row;
        m_sel_col = m_cursor_col;
        m_has_selection = true;
    }
    move(dr, dc);
}

void Grid::commit_and_step(int dr, int dc) {
    commit_edit();
    m_has_selection = false;
    const int prev_r = m_cursor_row, prev_c = m_cursor_col;
    move(dr, dc);
    if (m_cursor_row != prev_r || m_cursor_col != prev_c)
        start_edit(false);
}

// ── Viewport ─────────────────────────────────────────────────────────────────

int Grid::vis_rows() const {
    return rows_fitting_from(m_offset_row);
}

int Grid::vis_cols() const {
    const int gutter = ActionBox::k_width + k_rownum_w;
    int w = m_box.x_max - m_box.x_min + 1;
    if (w <= 2 + gutter) w = Terminal::Size().dimx;
    const int available = w - 2 - gutter;
    int cols = 0, used = 0;
    for (int c = m_offset_col; c < m_cols; ++c) {
        const int needed = 1 + m_col_widths[c]; // separator + cell
        if (used + needed > available && cols > 0) break;
        used += needed;
        ++cols;
    }
    return std::max(1, cols);
}

void Grid::adjust_viewport() {
    const int vc = vis_cols();
    if (m_cursor_row >= 0) {   // header row (-1) is always pinned above the grid
        if (m_cursor_row < m_offset_row) m_offset_row = m_cursor_row;
        // Rows have variable height, so the count that fits depends on the
        // offset; scroll down one row at a time until the cursor is in view.
        while (m_cursor_row >= m_offset_row + rows_fitting_from(m_offset_row))
            ++m_offset_row;
    }
    if (m_cursor_col >= 0) {
        if (m_cursor_col < m_offset_col)         m_offset_col = m_cursor_col;
        if (m_cursor_col >= m_offset_col + vc)   m_offset_col = m_cursor_col - vc + 1;
    }
}

int Grid::rows_fitting_from(int off) const {
    const int avail = (m_box.y_max - m_box.y_min + 1) - 4;  // minus borders+header+heavy sep
    if (avail <= 0) return std::max(1, Terminal::Size().dimy / 3);
    int used = 0, count = 0;
    for (int r = off; r < m_rows; ++r) {
        const int need = m_row_heights[r] + 1;  // content lines + separator
        if (used + need > avail && count > 0) break;
        used += need;
        ++count;
    }
    return std::max(1, count);
}

// ── Column width / row height ────────────────────────────────────────────────

int Grid::compute_col_width(int c) const {
    // Always leave room for the full header name and never clip it: the header
    // cell is laid out as  name + " " + [+/- box] + " "  and may also show a
    // " ▲"/" ▼" sort marker, so reserve name + k_width + 2 spaces + 2 for the
    // marker. Content can only make the column wider, never narrower.
    int w = static_cast<int>(m_col_names[c].size()) + ActionBox::k_width + 4;
    for (int r = 0; r < m_rows; ++r)
        w = std::max(w, static_cast<int>(cell_display(r, c).size()));
    return std::max(w, ActionBox::k_width + 3);
}

void Grid::refit_col(int c) {
    if (c < 0 || c >= m_cols) return;
    if (m_col_manual[c]) return;   // user pinned this width — leave it alone
    m_col_widths[c] = compute_col_width(c);
}

void Grid::set_col_width(int c, int w) {
    if (c < 0 || c >= m_cols) return;
    constexpr int k_min_w = ActionBox::k_width + 3;   // matches compute_col_width's floor
    constexpr int k_max_w = 120;
    w = std::clamp(w, k_min_w, k_max_w);
    m_col_widths[c] = w;
    m_col_manual[c] = true;        // pin it: edits no longer auto-refit this column
    set_status(m_col_names[c] + " width " + std::to_string(w));
}

void Grid::resize_col(int c, int delta) {
    if (c < 0 || c >= m_cols) return;
    set_col_width(c, m_col_widths[c] + delta);
}

int Grid::border_hit(int rx) const {
    if (rx < 0) return -1;
    int b = ActionBox::k_width + k_rownum_w;   // gutter, then visible columns
    for (int c = m_offset_col; c < m_cols; ++c) {
        b += 1 + m_col_widths[c];              // rx of the separator just past column c
        if (rx >= b - 1 && rx <= b + 1) return c;
        if (b - 1 > rx) break;                 // boundaries only grow → none further can match
    }
    return -1;
}

void Grid::set_row_height(int r, int h) {
    if (r < 0 || r >= m_rows) return;
    constexpr int k_min_h = 1;
    constexpr int k_max_h = 20;
    h = std::clamp(h, k_min_h, k_max_h);
    m_row_heights[r] = h;
    set_status("row " + std::to_string(r + 1) + " height " + std::to_string(h));
}

void Grid::resize_row(int r, int delta) {
    if (r < 0 || r >= m_rows) return;
    set_row_height(r, m_row_heights[r] + delta);
}

// The grid's content lines run: line 0 = header, line 1 = heavy separator, then
// each row occupies (height) content lines followed by 1 separator line. ry is
// the mouse y relative to the grid content (my - y_min - 1). Returns the row
// whose bottom separator sits on ry (its draggable border), else -1.
int Grid::row_border_hit(int ry) const {
    int line = 2;                              // first row's first content line
    for (int r = m_offset_row; r < m_rows; ++r) {
        const int sep = line + m_row_heights[r];   // separator line below row r
        if (ry == sep) return r;
        if (sep > ry) break;                       // separators only grow downward
        line = sep + 1;
    }
    return -1;
}
