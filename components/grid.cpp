// grid.cpp — Grid lifecycle (construction, load/save, the background calc
// thread) and the EvalContext implementation (formula evaluation, cross-sheet
// references). The rest of the class is split across grid_edit.cpp,
// grid_view.cpp, grid_search.cpp, grid_analytics.cpp, grid_render.cpp, and
// grid_input.cpp.
#include "grid.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <tuple>

#include "formulas/evaluator.hpp"
#include "col_label.hpp"
#include "sheet.hpp"

// ── Construction & calc thread ───────────────────────────────────────────────

Grid::Grid(int rows, int cols, const Config& cfg)
    : k_cell_w(cfg.grid.cell_width),
      m_rows(rows), m_cols(cols),
      m_cfg(cfg),
      m_cells(rows, std::vector<Cell>(cols)),
      m_col_names(cols),
      m_col_widths(cols, k_cell_w),
      m_col_manual(cols, false),
      m_row_heights(rows, 1),
      m_action_boxes(rows),
      m_col_action_boxes(cols) {
    for (int c = 0; c < cols; ++c)
        m_col_names[c] = col_letter(c);
    if (cfg.grid.start_insert) start_edit(false);
}

Grid::~Grid() {
    if (m_bg_thread.joinable()) m_bg_thread.join();
}

void Grid::set_calc_ready_cb(std::function<void()> cb) {
    m_on_calc_ready = std::move(cb);
}

void Grid::launch_build() {
    m_calc_cache.cancel();
    if (m_bg_thread.joinable()) m_bg_thread.join();
    m_calc_cache.reset();
    std::vector<std::vector<std::string>> raw(m_rows, std::vector<std::string>(m_cols));
    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c)
            raw[r][c] = m_cells[r][c].value();
    m_bg_thread = std::thread([this, raw = std::move(raw)]() mutable {
        m_calc_cache.build(std::move(raw));
        if (m_on_calc_ready) m_on_calc_ready();
    });
}

Grid::Mode Grid::mode() const noexcept {
    return (m_editing || m_insert_sticky) ? Mode::INSERT : Mode::NORMAL;
}

// ── Load / save ──────────────────────────────────────────────────────────────

void Grid::load(const SheetData& data) {
    const int new_cols = static_cast<int>(data.headers.size());
    const int new_rows = static_cast<int>(data.rows.size());

    m_cols = std::max(1, new_cols);
    m_rows = std::max(1, new_rows);

    m_cells.assign(m_rows, std::vector<Cell>(m_cols));

    m_col_names.resize(m_cols);
    for (int c = 0; c < m_cols; ++c)
        m_col_names[c] = (c < new_cols) ? data.headers[c] : col_letter(c);

    for (int r = 0; r < new_rows; ++r)
        for (int c = 0; c < std::min(m_cols, (int)data.rows[r].size()); ++c)
            m_cells[r][c].set_value(data.rows[r][c]);

    m_cursor_row     = 0;
    m_cursor_col     = 0;
    m_offset_row     = 0;
    m_offset_col     = 0;
    m_editing        = false;
    m_edit_buf.clear();
    m_edit_orig.clear();
    m_undo_stack.clear();
    m_redo_stack.clear();
    m_action_boxes.assign(m_rows, ActionBox{});
    m_col_action_boxes.assign(m_cols, ActionBox{});
    m_pending_delete_row = -1;
    m_pending_delete_col = -1;
    m_col_widths.resize(m_cols);
    m_col_manual.assign(m_cols, false);   // a fresh load auto-fits every column
    m_row_heights.assign(m_rows, 1);      // every row starts one line tall
    for (int c = 0; c < m_cols; ++c)
        m_col_widths[c] = compute_col_width(c);

    launch_build();
}

void Grid::save_to(Sheet& s) const {
    s.cells       = m_cells;
    s.col_names   = m_col_names;
    s.col_widths  = m_col_widths;
    s.col_manual  = m_col_manual;
    s.row_heights = m_row_heights;
    s.cursor_row  = m_cursor_row;
    s.cursor_col  = m_cursor_col;
    s.offset_row  = m_offset_row;
    s.offset_col  = m_offset_col;
    s.undo_stack  = m_undo_stack;
    s.redo_stack  = m_redo_stack;
}

void Grid::load_from(const Sheet& s) {
    commit_edit();
    m_editing            = false;
    m_insert_sticky      = false;
    m_pending_g          = false;
    m_pending_delete_row = -1;
    m_pending_delete_col = -1;
    m_has_selection      = false;
    m_edit_buf.clear();
    m_edit_orig.clear();
    m_edit_cursor        = 0;

    m_sheet_name = s.name;
    m_cells      = s.cells;
    m_col_names  = s.col_names;
    m_col_widths = s.col_widths;
    m_col_manual = s.col_manual;
    m_row_heights = s.row_heights;
    m_rows       = static_cast<int>(m_cells.size());
    m_cols       = m_rows ? static_cast<int>(m_cells[0].size()) : static_cast<int>(m_col_names.size());
    if (m_rows == 0) { m_rows = 1; m_cells.assign(1, std::vector<Cell>(std::max(1, m_cols))); }
    if (m_cols == 0) { m_cols = 1; for (auto& r : m_cells) r.assign(1, Cell{}); m_col_names.assign(1, col_letter(0)); m_col_widths.assign(1, k_cell_w); }
    m_col_manual.resize(m_cols, false);   // legacy snapshots predate this field
    m_row_heights.resize(m_rows, 1);       // ditto; default every row to one line

    // Sheets arrive from the workbook with a fixed default column width, so
    // auto-fit every column the user hasn't manually pinned — otherwise freshly
    // opened files clip all content to that default. Pinned widths (tracked by
    // col_manual and preserved across sheet switches) are left untouched.
    m_col_widths.resize(m_cols, k_cell_w);
    for (int c = 0; c < m_cols; ++c)
        if (!m_col_manual[c]) m_col_widths[c] = compute_col_width(c);

    m_action_boxes.assign(m_rows, ActionBox{});
    m_col_action_boxes.assign(m_cols, ActionBox{});

    m_cursor_row = std::min(s.cursor_row, m_rows - 1);
    m_cursor_col = std::min(s.cursor_col, m_cols - 1);
    m_offset_row = std::max(0, std::min(s.offset_row, m_rows - 1));
    m_offset_col = std::max(0, std::min(s.offset_col, m_cols - 1));
    m_undo_stack = s.undo_stack;
    m_redo_stack = s.redo_stack;

    launch_build();
}

SheetData Grid::to_csv_data(char delimiter) const {
    SheetData data;
    data.delimiter = delimiter;
    data.headers   = m_col_names;
    data.rows.resize(m_rows);
    for (int r = 0; r < m_rows; ++r) {
        data.rows[r].resize(m_cols);
        for (int c = 0; c < m_cols; ++c)
            data.rows[r][c] = cell_display(r, c);
    }
    return data;
}

// ── Basic accessors ──────────────────────────────────────────────────────────

int Grid::rows() const { return m_rows; }
int Grid::cols() const { return m_cols; }

Cell& Grid::at(int r, int c)             { return m_cells[r][c]; }
const Cell& Grid::at(int r, int c) const { return m_cells[r][c]; }

// ── EvalContext: formula evaluation & cross-sheet references ──────────────────

namespace {

// Recursion guard keyed by (sheet, row, col) so cycles are caught both within
// a sheet and across sheets (Sheet1!A1 → Sheet2!A1 → Sheet1!A1).
using EvalKey = std::tuple<std::string, int, int>;
thread_local std::set<EvalKey> s_eval_guard;

// Interpret a raw cell string: evaluate formulas through `ctx`, otherwise
// coerce to a number or fall back to text. `sheet` identifies the owning sheet
// for cycle detection.
Value eval_raw(const std::string& raw, const std::string& sheet,
               int row, int col, const EvalContext& ctx) {
    if (raw.empty()) return Value::empty();
    EvalKey key{sheet, row, col};
    if (s_eval_guard.count(key)) return Value::error(FormulaError::REF); // circular
    if (raw[0] == '=') {
        s_eval_guard.insert(key);
        Value result = Evaluator::evaluate_formula(raw, ctx);
        s_eval_guard.erase(key);
        return result;
    }
    try { return Value::number(std::stod(raw)); } catch (...) {}
    return Value::string(raw);
}

bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    return true;
}

// Read-only EvalContext over an inactive sheet's snapshot. Same-sheet refs
// resolve against the snapshot; qualified refs funnel back through the Grid so
// the active sheet stays authoritative and one cycle guard covers everything.
class SheetView : public EvalContext {
public:
    SheetView(const Sheet& s, const Grid& grid) : m_sheet(s), m_grid(grid) {}
    int rows() const override { return static_cast<int>(m_sheet.cells.size()); }
    int cols() const override { return rows() ? static_cast<int>(m_sheet.cells[0].size()) : 0; }
    Value cell_value(int r, int c) const override {
        if (r < 0 || r >= rows() || c < 0 || c >= cols())
            return Value::error(FormulaError::REF);
        return eval_raw(m_sheet.cells[r][c].value(), m_sheet.name, r, c, *this);
    }
    Value cell_value_in(const std::string& sheet, int r, int c) const override {
        return m_grid.cell_value_in(sheet, r, c);
    }
private:
    const Sheet& m_sheet;
    const Grid&  m_grid;
};

}  // namespace

Value Grid::cell_value(int row, int col) const {
    if (row < 0 || row >= m_rows || col < 0 || col >= m_cols)
        return Value::error(FormulaError::REF);
    return eval_raw(m_cells[row][col].value(), m_sheet_name, row, col, *this);
}

Value Grid::cell_value_in(const std::string& sheet, int row, int col) const {
    // The live grid holds the authoritative copy of the active sheet.
    if (iequals(sheet, m_sheet_name))
        return cell_value(row, col);
    const Sheet* s = m_sheet_lookup ? m_sheet_lookup(sheet) : nullptr;
    if (!s) return Value::error(FormulaError::REF);
    return SheetView(*s, *this).cell_value(row, col);
}

void Grid::set_sheet_lookup(std::function<const Sheet*(const std::string&)> fn) {
    m_sheet_lookup = std::move(fn);
}

std::string Grid::cell_display(int r, int c) const {
    const std::string& raw = m_cells[r][c].value();
    if (!raw.empty() && raw[0] == '=')
        return cell_value(r, c).to_display();
    return raw;
}
