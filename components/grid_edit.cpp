// grid_edit.cpp — data mutations: row/column structure, cell editing,
// undo/redo history, and yank/paste. (Part of the Grid class; see grid.hpp.)
#include "grid.hpp"

#include <algorithm>
#include <string>

#include "col_label.hpp"
#include "util/clipboard.hpp"

// ── Row / column structure ───────────────────────────────────────────────────

void Grid::add_row() {
    m_cells.emplace_back(m_cols);
    m_action_boxes.emplace_back();
    m_row_heights.push_back(1);
    ++m_rows;
    m_cursor_row = m_rows - 1;
    m_cursor_col = 0;
    after_structural_change();
}

void Grid::insert_row(int r) {
    m_cells.insert(m_cells.begin() + r + 1, std::vector<Cell>(m_cols));
    m_action_boxes.insert(m_action_boxes.begin() + r + 1, ActionBox{});
    m_row_heights.insert(m_row_heights.begin() + r + 1, 1);
    ++m_rows;
    m_cursor_row = r + 1;
    m_cursor_col = 0;
    after_structural_change();
}

void Grid::add_col() {
    for (auto& row : m_cells)
        row.emplace_back();
    m_col_names.push_back(col_letter(m_cols));
    m_col_widths.push_back(k_cell_w);
    m_col_manual.push_back(false);
    m_col_action_boxes.emplace_back();
    ++m_cols;
    m_cursor_col = m_cols - 1;
    m_cursor_row = -1;   // land on the new column's header
    after_structural_change();
}

void Grid::insert_col(int c) {
    const std::string new_name = unique_col_name(col_letter(c + 1), m_cols);
    for (auto& row : m_cells)
        row.insert(row.begin() + c + 1, Cell{});
    m_col_names.insert(m_col_names.begin() + c + 1, new_name);
    m_col_widths.insert(m_col_widths.begin() + c + 1, k_cell_w);
    m_col_manual.insert(m_col_manual.begin() + c + 1, false);
    m_col_action_boxes.insert(m_col_action_boxes.begin() + c + 1, ActionBox{});
    ++m_cols;
    m_cursor_col = c + 1;
    m_cursor_row = -1;   // land on the new column's header
    after_structural_change();
}

void Grid::delete_col(int c) {
    if (m_cols <= 1 || c < 0 || c >= m_cols) return;
    for (auto& row : m_cells)
        row.erase(row.begin() + c);
    m_col_names.erase(m_col_names.begin() + c);
    m_col_widths.erase(m_col_widths.begin() + c);
    m_col_manual.erase(m_col_manual.begin() + c);
    m_col_action_boxes.erase(m_col_action_boxes.begin() + c);
    --m_cols;
    m_cursor_col = std::min(m_cursor_col, m_cols - 1);
    after_structural_change();
}

void Grid::try_delete_row(int r) {
    for (int c = 0; c < m_cols; ++c)
        if (!m_cells[r][c].value().empty()) { m_pending_delete_row = r; return; }
    delete_row(r);
}

void Grid::try_delete_col(int c) {
    for (int r = 0; r < m_rows; ++r)
        if (!m_cells[r][c].value().empty()) { m_pending_delete_col = c; return; }
    delete_col(c);
}

void Grid::delete_row(int r) {
    if (m_rows <= 1 || r < 0 || r >= m_rows) return;
    m_cells.erase(m_cells.begin() + r);
    m_action_boxes.erase(m_action_boxes.begin() + r);
    m_row_heights.erase(m_row_heights.begin() + r);
    --m_rows;
    m_cursor_row = std::min(m_cursor_row, m_rows - 1);
    after_structural_change();
}

void Grid::after_structural_change() {
    m_editing = false;
    adjust_viewport();
    launch_build();
}

void Grid::apply_row_permutation(const std::vector<int>& order) {
    if ((int)order.size() != m_rows) return;
    std::vector<std::vector<Cell>> nc;  nc.reserve(m_rows);
    std::vector<int>               nh;  nh.reserve(m_rows);
    std::vector<ActionBox>         nb;  nb.reserve(m_rows);
    for (int i : order) {                       // order is a permutation: each i once
        nc.push_back(std::move(m_cells[i]));
        nh.push_back(m_row_heights[i]);
        nb.push_back(std::move(m_action_boxes[i]));
    }
    m_cells        = std::move(nc);
    m_row_heights  = std::move(nh);
    m_action_boxes = std::move(nb);
}

std::string Grid::unique_col_name(const std::string& name, int skip_col) const {
    auto taken = [&](const std::string& n) {
        for (int c = 0; c < m_cols; ++c)
            if (c != skip_col && m_col_names[c] == n) return true;
        return false;
    };
    if (!taken(name)) return name;
    for (int i = 1; ; ++i) {
        std::string candidate = name + std::to_string(i);
        if (!taken(candidate)) return candidate;
    }
}

// ── Cell editing ─────────────────────────────────────────────────────────────

std::string Grid::value_at(int r, int c) const {
    return (r < 0) ? m_col_names[c] : m_cells[r][c].value();
}

void Grid::set_value_at(int r, int c, const std::string& v) {
    if (r < 0) m_col_names[c] = v;
    else       m_cells[r][c].set_value(v);
}

void Grid::start_edit(bool clear) {
    if (m_cursor_col < 0) return;   // the row-index gutter has nothing to edit
    m_editing       = true;
    m_insert_sticky = true;
    m_edit_orig   = value_at(m_cursor_row, m_cursor_col);
    m_edit_buf    = clear ? "" : m_edit_orig;
    m_edit_cursor = (int)m_edit_buf.size();
    m_edit_typed  = false;
}

void Grid::commit_edit() {
    if (!m_editing) return;
    std::string after = m_edit_buf;
    if (m_cursor_row < 0)                    // editing a column name: keep names unique
        after = unique_col_name(after, m_cursor_col);
    if (after != m_edit_orig) {
        m_undo_stack.push_back({m_cursor_row, m_cursor_col, m_edit_orig, after});
        m_redo_stack.clear();
        set_value_at(m_cursor_row, m_cursor_col, after);
        refit_col(m_cursor_col);
        if (m_cursor_row >= 0)
            m_calc_cache.rebuild_cell(m_cursor_row, m_cursor_col, after);
    }
    m_editing = false;
}

void Grid::clear_cell(int r, int c) {
    Cell& cell = at(r, c);
    if (cell.value().empty()) return;
    m_undo_stack.push_back({r, c, cell.value(), ""});
    m_redo_stack.clear();
    cell.set_value("");
    refit_col(c);
}

// ── Undo / redo ──────────────────────────────────────────────────────────────

bool Grid::can_undo() const noexcept { return !m_undo_stack.empty(); }
bool Grid::can_redo() const noexcept { return !m_redo_stack.empty(); }

void Grid::apply_history(std::vector<HistoryEntry>& from,
                         std::vector<HistoryEntry>& to, bool use_after) {
    if (from.empty()) return;
    m_editing = false;
    auto e = std::move(from.back());
    from.pop_back();
    if (e.kind == HistoryKind::Reorder) {
        // Undo applies the inverse of the recorded permutation; redo re-applies it.
        std::vector<int> perm = e.order;
        if (!use_after) {
            std::vector<int> inv(perm.size());
            for (int i = 0; i < (int)perm.size(); ++i) inv[perm[i]] = i;
            perm = std::move(inv);
        }
        apply_row_permutation(perm);
        m_cursor_row = std::clamp(m_cursor_row, 0, m_rows - 1);
        to.push_back(std::move(e));
        adjust_viewport();
        launch_build();
        return;
    }
    const std::string& v = use_after ? e.after : e.before;
    set_value_at(e.row, e.col, v);
    refit_col(e.col);
    if (e.row >= 0) m_calc_cache.rebuild_cell(e.row, e.col, v);
    m_cursor_row = e.row;
    m_cursor_col = e.col;
    to.push_back(std::move(e));
    adjust_viewport();
}

void Grid::undo() { apply_history(m_undo_stack, m_redo_stack, /*use_after=*/false); }
void Grid::redo() { apply_history(m_redo_stack, m_undo_stack, /*use_after=*/true ); }

// ── Yank / paste ─────────────────────────────────────────────────────────────

void Grid::yank_selection() {
    int r0 = m_cursor_row, r1 = m_cursor_row;
    int c0 = m_cursor_col, c1 = m_cursor_col;
    if (m_has_selection) {
        r0 = std::min(m_cursor_row, m_sel_row); r1 = std::max(m_cursor_row, m_sel_row);
        c0 = std::min(m_cursor_col, m_sel_col); c1 = std::max(m_cursor_col, m_sel_col);
    }
    m_yank_data.clear();
    for (int r = r0; r <= r1; ++r) {
        m_yank_data.emplace_back();
        for (int c = c0; c <= c1; ++c)
            m_yank_data.back().push_back(at(r, c).value());
    }
    m_yank_row = r0;
    m_yank_col = c0;

    // Mirror the selection onto the system clipboard as TSV (tab between cells,
    // newline between rows) so it can be pasted into other applications. The
    // internal m_yank_data above still drives structured in-app paste (`p`).
    std::string tsv;
    for (size_t r = 0; r < m_yank_data.size(); ++r) {
        if (r) tsv += '\n';
        for (size_t c = 0; c < m_yank_data[r].size(); ++c) {
            if (c) tsv += '\t';
            tsv += m_yank_data[r][c];
        }
    }
    tuix::copy_to_clipboard(tsv);

    const int nr = r1 - r0 + 1, nc = c1 - c0 + 1;
    set_status(nr == 1 && nc == 1
                   ? "Yanked cell → clipboard"
                   : "Yanked " + std::to_string(nr) + "×" + std::to_string(nc) +
                         " → clipboard");
}

void Grid::paste_yanked() {
    const int need_rows = m_cursor_row + (int)m_yank_data.size();
    const int need_cols = m_cursor_col + (int)m_yank_data[0].size();
    while (m_rows < need_rows) {
        m_cells.emplace_back(m_cols);
        m_action_boxes.emplace_back();
        m_row_heights.push_back(1);
        ++m_rows;
    }
    while (m_cols < need_cols) {
        for (auto& row : m_cells) row.emplace_back();
        m_col_names.push_back(col_letter(m_cols));
        m_col_widths.push_back(k_cell_w);
        m_col_manual.push_back(false);
        m_col_action_boxes.emplace_back();
        ++m_cols;
    }
    m_redo_stack.clear();
    for (int dr = 0; dr < (int)m_yank_data.size(); ++dr) {
        for (int dc = 0; dc < (int)m_yank_data[dr].size(); ++dc) {
            auto& cell = at(m_cursor_row + dr, m_cursor_col + dc);
            m_undo_stack.push_back({m_cursor_row + dr, m_cursor_col + dc, cell.value(), m_yank_data[dr][dc]});
            cell.set_value(m_yank_data[dr][dc]);
        }
    }
    m_yank_row = -1;
    m_yank_col = -1;
}
