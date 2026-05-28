#include "grid.hpp"

#include <algorithm>
#include <set>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "formulas/evaluator.hpp"

using namespace ftxui;

namespace {

const Event ShiftArrowUp    = Event::Special("\x1B[1;2A");
const Event ShiftArrowDown  = Event::Special("\x1B[1;2B");
const Event ShiftArrowRight = Event::Special("\x1B[1;2C");
const Event ShiftArrowLeft  = Event::Special("\x1B[1;2D");

struct FormulaInfo { const char* name; const char* sig; const char* desc; };
const FormulaInfo k_formulas[] = {
    { "ABS",         "ABS(number)",                "Absolute value (removes sign)"          },
    { "AVERAGE",     "AVERAGE(range)",             "Arithmetic mean of numeric cells"       },
    { "CONCATENATE", "CONCATENATE(text1, text2…)", "Join two or more text strings"          },
    { "COUNT",       "COUNT(range)",               "Count cells that contain numbers"       },
    { "COUNTA",      "COUNTA(range)",              "Count non-empty cells"                  },
    { "IF",          "IF(cond, true, false)",       "Return one of two values based on test" },
    { "IFERROR",     "IFERROR(expr, fallback)",     "Return fallback if expr errors"         },
    { "INT",         "INT(number)",                "Truncate to integer toward zero"        },
    { "LEN",         "LEN(text)",                  "Number of characters in text"           },
    { "LOWER",       "LOWER(text)",                "Convert text to lower case"             },
    { "MAX",         "MAX(range)",                 "Largest numeric value in range"         },
    { "MIN",         "MIN(range)",                 "Smallest numeric value in range"        },
    { "MOD",         "MOD(number, divisor)",        "Remainder after division"               },
    { "ROUND",       "ROUND(number, digits)",       "Round to given decimal places"          },
    { "SQRT",        "SQRT(number)",               "Square root"                            },
    { "SUM",         "SUM(range)",                 "Sum all numeric values in range"        },
    { "TRIM",        "TRIM(text)",                 "Remove leading/trailing whitespace"     },
    { "UPPER",       "UPPER(text)",                "Convert text to upper case"             },
};
constexpr int k_formulas_count = (int)(sizeof(k_formulas) / sizeof(k_formulas[0]));

std::string ac_prefix(const std::string& buf) {
    int i = (int)buf.size();
    while (i > 0 && std::isalpha((unsigned char)buf[i - 1])) --i;
    return buf.substr(i);
}

bool should_show_ac(const std::string& buf) {
    if (buf.empty() || buf[0] != '=') return false;
    return buf.size() == 1 || std::isalpha((unsigned char)buf.back());
}

}  // namespace

Grid::Grid(int rows, int cols, const Config& cfg)
    : k_cell_w(cfg.grid.cell_width),
      m_rows(rows), m_cols(cols),
      m_cfg(cfg),
      m_cells(rows, std::vector<Cell>(cols)),
      m_col_names(cols),
      m_col_widths(cols, k_cell_w),
      m_action_boxes(rows),
      m_col_action_boxes(cols) {
    for (int c = 0; c < cols; ++c)
        m_col_names[c] = col_letter(c);
    start_edit(false);
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
    for (int c = 0; c < m_cols; ++c)
        m_col_widths[c] = compute_col_width(c);

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

void Grid::add_row() {
    m_cells.emplace_back(m_cols);
    m_action_boxes.emplace_back();
    ++m_rows;
    m_cursor_row = m_rows - 1;
    m_cursor_col = 0;
    m_editing    = false;
    adjust_viewport();
    launch_build();
}

void Grid::insert_row(int r) {
    m_cells.insert(m_cells.begin() + r + 1, std::vector<Cell>(m_cols));
    m_action_boxes.insert(m_action_boxes.begin() + r + 1, ActionBox{});
    ++m_rows;
    m_cursor_row = r + 1;
    m_cursor_col = 0;
    m_editing    = false;
    adjust_viewport();
    launch_build();
}

void Grid::add_col() {
    for (auto& row : m_cells)
        row.emplace_back();
    m_col_names.push_back(col_letter(m_cols));
    m_col_widths.push_back(k_cell_w);
    m_col_action_boxes.emplace_back();
    ++m_cols;
    m_cursor_col = m_cols - 1;
    m_cursor_row = -1;   // land on the new column's header
    m_editing    = false;
    adjust_viewport();
    launch_build();
}

void Grid::insert_col(int c) {
    const std::string new_name = unique_col_name(col_letter(m_cols), m_cols);
    for (auto& row : m_cells)
        row.insert(row.begin() + c + 1, Cell{});
    m_col_names.insert(m_col_names.begin() + c + 1, new_name);
    m_col_widths.insert(m_col_widths.begin() + c + 1, k_cell_w);
    m_col_action_boxes.insert(m_col_action_boxes.begin() + c + 1, ActionBox{});
    ++m_cols;
    m_cursor_col = c + 1;
    m_cursor_row = -1;   // land on the new column's header
    m_editing    = false;
    adjust_viewport();
    launch_build();
}

void Grid::delete_col(int c) {
    if (m_cols <= 1 || c < 0 || c >= m_cols) return;
    for (auto& row : m_cells)
        row.erase(row.begin() + c);
    m_col_names.erase(m_col_names.begin() + c);
    m_col_widths.erase(m_col_widths.begin() + c);
    m_col_action_boxes.erase(m_col_action_boxes.begin() + c);
    --m_cols;
    m_cursor_col = std::min(m_cursor_col, m_cols - 1);
    m_editing    = false;
    adjust_viewport();
    launch_build();
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
    --m_rows;
    m_cursor_row = std::min(m_cursor_row, m_rows - 1);
    m_editing    = false;
    adjust_viewport();
    launch_build();
}

int Grid::rows() const { return m_rows; }
int Grid::cols() const { return m_cols; }

Cell& Grid::at(int r, int c)             { return m_cells[r][c]; }
const Cell& Grid::at(int r, int c) const { return m_cells[r][c]; }

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

static thread_local std::set<std::pair<int,int>> s_eval_guard;

Value Grid::cell_value(int row, int col) const {
    if (row < 0 || row >= m_rows || col < 0 || col >= m_cols)
        return Value::error(FormulaError::REF);
    const std::string& raw = m_cells[row][col].value();
    if (raw.empty()) return Value::empty();
    auto key = std::make_pair(row, col);
    if (s_eval_guard.count(key)) return Value::error(FormulaError::REF); // circular
    if (raw[0] == '=') {
        s_eval_guard.insert(key);
        Value result = Evaluator::evaluate_formula(raw, *this);
        s_eval_guard.erase(key);
        return result;
    }
    try { return Value::number(std::stod(raw)); } catch (...) {}
    return Value::string(raw);
}

std::string Grid::cell_display(int r, int c) const {
    const std::string& raw = m_cells[r][c].value();
    if (!raw.empty() && raw[0] == '=')
        return cell_value(r, c).to_display();
    return raw;
}

std::string Grid::context_hint() const {
    if (m_pending_delete_row >= 0 || m_pending_delete_col >= 0)
        return "y: confirm delete  |  n/Esc: cancel";
    if (m_editing) {
        if (m_cursor_row < 0)
            return "Enter: confirm & ↓  |  Tab/→: confirm & next col  |  Esc: confirm & NORMAL";
        if (should_show_ac(m_edit_buf) && !ac_matches().empty())
            return "↑↓: navigate  |  Tab/Enter: complete  |  type to filter";
        return "Enter: confirm & ↓  |  Tab: confirm & →  |  Arrows: confirm & move  |  Del: clear";
    }
    if (m_cursor_row < 0)
        return "hjkl/arrows: nav  |  +: insert col  |  -/x: delete col  |  i/a: rename  |  ↓: into grid";
    if (m_cursor_col < 0)
        return "hjkl/arrows: nav  |  +: insert row  |  -/x: delete row  |  u/Ctrl+R: undo/redo  |  →: enter row";
    return "hjkl: nav  |  i/a: edit  |  o/O: new row  |  x: delete  |  y/p: yank/paste  |  Shift+arrows: select  |  gg/G: top/bottom  |  u/Ctrl+R: undo/redo  |  :: cmd";
}

std::vector<Grid::Suggestion> Grid::cell_suggestions() const {
    if (m_cursor_row < 0 || m_cursor_col < 0) return {};
    if (m_has_selection) return {};
    if (!m_calc_cache.ready()) return {};
    auto entries = m_calc_cache.get(m_cursor_row, m_cursor_col);
    std::vector<Suggestion> out;
    out.reserve(entries.size());
    for (auto& e : entries) out.push_back({e.name, e.value});
    return out;
}

std::vector<Grid::Suggestion> Grid::range_suggestions() const {
    if (!m_has_selection || m_cursor_row < 0 || m_cursor_col < 0) return {};

    const int r0 = std::min(m_cursor_row, m_sel_row);
    const int r1 = std::max(m_cursor_row, m_sel_row);
    const int c0 = std::min(m_cursor_col, m_sel_col);
    const int c1 = std::max(m_cursor_col, m_sel_col);

    if (r0 == r1 && c0 == c1) return {};  // single cell — cell_suggestions handles it

    const std::string range = col_letter(c0) + std::to_string(r0 + 1)
                            + ":"
                            + col_letter(c1) + std::to_string(r1 + 1);

    std::vector<Suggestion> out;
    auto add = [&](const char* name, const std::string& formula) {
        Value v = Evaluator::evaluate_formula(formula, *this);
        if (!v.is_error()) out.push_back({name, v.to_display()});
    };

    add("SUM",    "=SUM("     + range + ")");
    add("AVG",    "=AVERAGE(" + range + ")");
    add("COUNT",  "=COUNT("   + range + ")");
    add("COUNTA", "=COUNTA("  + range + ")");
    add("MIN",    "=MIN("     + range + ")");
    add("MAX",    "=MAX("     + range + ")");

    return out;
}

void Grid::move(int dr, int dc) {
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
    m_edit_orig = value_at(m_cursor_row, m_cursor_col);
    m_edit_buf  = clear ? "" : m_edit_orig;
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
        m_col_widths[m_cursor_col] = compute_col_width(m_cursor_col);
        if (m_cursor_row >= 0)
            m_calc_cache.rebuild_cell(m_cursor_row, m_cursor_col, after);
    }
    m_editing = false;
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

bool Grid::can_undo() const noexcept { return !m_undo_stack.empty(); }
bool Grid::can_redo() const noexcept { return !m_redo_stack.empty(); }

void Grid::undo() {
    if (m_undo_stack.empty()) return;
    m_editing = false;
    auto e = std::move(m_undo_stack.back());
    m_undo_stack.pop_back();
    set_value_at(e.row, e.col, e.before);
    m_col_widths[e.col] = compute_col_width(e.col);
    if (e.row >= 0) m_calc_cache.rebuild_cell(e.row, e.col, e.before);
    m_cursor_row = e.row;
    m_cursor_col = e.col;
    m_redo_stack.push_back(std::move(e));
    adjust_viewport();
}

void Grid::redo() {
    if (m_redo_stack.empty()) return;
    m_editing = false;
    auto e = std::move(m_redo_stack.back());
    m_redo_stack.pop_back();
    set_value_at(e.row, e.col, e.after);
    m_col_widths[e.col] = compute_col_width(e.col);
    if (e.row >= 0) m_calc_cache.rebuild_cell(e.row, e.col, e.after);
    m_cursor_row = e.row;
    m_cursor_col = e.col;
    m_undo_stack.push_back(std::move(e));
    adjust_viewport();
}

void Grid::scroll_to_mouse_y(int my) {
    const int bar_h   = m_box.y_max - m_box.y_min + 1;
    const int ry      = my - m_box.y_min;
    const int max_off = std::max(0, m_rows - vis_rows());
    m_offset_row = std::clamp(ry * max_off / std::max(1, bar_h - 1), 0, max_off);
    m_cursor_row = std::clamp(m_cursor_row,
                              m_offset_row,
                              std::min(m_rows - 1, m_offset_row + vis_rows() - 1));
}

bool Grid::select_at_mouse(int mx, int my) {
    if (mx <= m_box.x_min || mx >= m_box.x_max) return false;
    if (my <= m_box.y_min || my >= m_box.y_max) return false;

    const int rx = mx - m_box.x_min - 1;
    const int ry = my - m_box.y_min - 1;

    const int k_gutter = ActionBox::k_width + k_rownum_w;

    auto col_at = [&](int x) -> int {
        x -= k_gutter;
        for (int c = m_offset_col; c < m_cols; ++c) {
            x -= 1 + m_col_widths[c]; // consume separator + cell
            if (x < 0) return c;
        }
        return -1;
    };

    if (ry == 0 && rx >= k_gutter) {        // header band → header row (-1)
        const int c = col_at(rx);
        if (c < 0) return false;
        commit_edit();
        m_cursor_row = -1;
        m_cursor_col = c;
        adjust_viewport();
        return true;
    }
    if (ry < 2) return false;

    const int r = m_offset_row + (ry - 2) / 2;
    const int c = col_at(rx);

    if (r >= m_rows || c < 0 || c >= m_cols) return false;

    commit_edit();
    m_cursor_row = r;
    m_cursor_col = c;
    adjust_viewport();
    return true;
}

Element Grid::formula_bar() const {
    if (m_pending_delete_row >= 0) {
        return window(
            hbox({ text(" "), text("delete row") | bold | color(Color::Red), text(" ") }),
            hbox({
                text(" Delete row " + std::to_string(m_pending_delete_row + 1) + "?") | bold,
                text("   [Y] confirm   [N] cancel") | color(m_cfg.colors.dimmed),
                filler(),
            })
        );
    }
    if (m_pending_delete_col >= 0) {
        return window(
            hbox({ text(" "), text("delete col") | bold | color(Color::Red), text(" ") }),
            hbox({
                text(" Delete column \"" + m_col_names[m_pending_delete_col] + "\"?") | bold,
                text("   [Y] confirm   [N] cancel") | color(m_cfg.colors.dimmed),
                filler(),
            })
        );
    }
    std::string label = cursor_label();
    std::string content;
    if (m_cursor_col < 0)       content = "";                          // row-index gutter
    else if (m_editing)         content = m_edit_buf + "_";
    else if (m_cursor_row < 0)  content = m_col_names[m_cursor_col];   // column header
    else                        content = at(m_cursor_row, m_cursor_col).display();
    return window(
        hbox({ text(" "), text(label) | bold | color(m_cfg.colors.header), text(" ") }),
        hbox({ text(" "), text(content), filler() })
    );
}

Element Grid::render() const {
    const int vr    = vis_rows();
    const int vc    = vis_cols();
    const int r_end = std::min(m_rows, m_offset_row + vr);
    const int c_end = std::min(m_cols, m_offset_col + vc);

    Elements header;
    header.push_back(text(std::string(ActionBox::k_width + k_rownum_w, ' ')) | bold | color(m_cfg.colors.header));
    for (int c = m_offset_col; c < c_end; ++c) {
        header.push_back(separator());
        const bool hsel    = (m_cursor_row < 0 && c == m_cursor_col);
        const int  name_w  = m_col_widths[c] - ActionBox::k_width - 1;
        std::string name = (hsel && m_editing) ? m_edit_buf : m_col_names[c];
        if ((int)name.size() > name_w) name = name.substr(0, name_w);
        auto name_e = text(name) | center | size(WIDTH, EQUAL, name_w) | bold;
        name_e = hsel ? name_e | bgcolor(m_cfg.colors.cursor_bg) | color(m_cfg.colors.cursor_fg)
                      : name_e | color(m_cfg.colors.header);
        header.push_back(
            hbox({ std::move(name_e), text(" "), m_col_action_boxes[c].render(hsel) })
            | reflect(m_col_action_boxes[c].cell_box())
        );
    }


    header.push_back(filler());

    Elements elems;
    elems.push_back(hbox(std::move(header)));
    elems.push_back(separatorHeavy());

    for (int r = m_offset_row; r < r_end; ++r) {
        Elements row;

        // First cell: index + action box
        if (r == m_pending_delete_row)
            row.push_back(text("del?") | bold | color(Color::Red) | size(WIDTH, EQUAL, k_rownum_w + ActionBox::k_width));
        else {
            const bool idx_active = (m_cursor_col < 0 && r == m_cursor_row);
            row.push_back(hbox({
                m_action_boxes[r].render(idx_active),
                text(std::to_string(r + 1)) | align_right | size(WIDTH, EQUAL, k_rownum_w) | color(m_cfg.colors.row_number),
            }) | reflect(m_action_boxes[r].cell_box()));
        }

        for (int c = m_offset_col; c < c_end; ++c) {
            row.push_back(separator());
            const bool is_cursor  = (r == m_cursor_row && c == m_cursor_col);
            const bool is_yanked  = m_yank_row >= 0 && !m_yank_data.empty() &&
                r >= m_yank_row && r < m_yank_row + (int)m_yank_data.size() &&
                c >= m_yank_col && c < m_yank_col + (int)m_yank_data[0].size();
            const std::string& raw = m_cells[r][c].value();
            const bool is_formula = !raw.empty() && raw[0] == '=';
            std::string val = (is_cursor && m_editing) ? m_edit_buf : cell_display(r, c);
            if ((int)val.size() > m_col_widths[c]) val = val.substr(0, m_col_widths[c]);
            auto e = text(val) | size(WIDTH, EQUAL, m_col_widths[c]);
            if (is_cursor)
                e = e | bgcolor(m_cfg.colors.cursor_bg) | color(m_cfg.colors.cursor_fg);
            else if (is_yanked)
                e = e | color(Color::Yellow);
            else if (in_selection(r, c))
                e = e | bgcolor(m_cfg.colors.selection_bg) | color(m_cfg.colors.selection_fg);
            else if (is_formula)
                e = e | color(m_cfg.colors.formula_fg);
            row.push_back(e);
        }
        row.push_back(filler());
        elems.push_back(hbox(std::move(row)));
        elems.push_back(separator());
    }

    return vbox(std::move(elems)) | borderRounded | reflect(m_box);
}

Element Grid::vscrollbar() const {
    const int bar_h   = m_box.y_max - m_box.y_min + 1;
    const int visible = vis_rows();

    if (bar_h <= 0) return text(" ") | size(WIDTH, EQUAL, 1);

    Elements bar;

    if (m_rows <= visible) {
        for (int i = 0; i < bar_h; ++i)
            bar.push_back(text("░") | color(m_cfg.colors.dimmed));
    } else {
        const int thumb_h   = std::max(1, bar_h * visible / m_rows);
        const int max_top   = bar_h - thumb_h;
        const int thumb_top = (m_offset_row * max_top) / (m_rows - visible);

        for (int i = 0; i < bar_h; ++i) {
            const bool in_thumb = (i >= thumb_top && i < thumb_top + thumb_h);
            bar.push_back(
                text(in_thumb ? "▓" : "░") |
                color(in_thumb ? m_cfg.colors.row_number : m_cfg.colors.dimmed)
            );
        }
    }

    return vbox(std::move(bar)) | size(WIDTH, EQUAL, 1);
}

std::vector<int> Grid::ac_matches() const {
    const std::string pfx = ac_prefix(m_edit_buf);
    std::string upper = pfx;
    for (auto& c : upper) c = std::toupper((unsigned char)c);
    std::vector<int> out;
    for (int i = 0; i < k_formulas_count; ++i)
        if (std::string(k_formulas[i].name).substr(0, upper.size()) == upper)
            out.push_back(i);
    return out;
}

Element Grid::render_ac_popup() const {
    const auto matches = ac_matches();
    const int sel = std::clamp(m_ac_sel, 0, (int)matches.size() - 1);
    Elements items;
    for (int i = 0; i < (int)matches.size(); ++i) {
        const bool s = (i == sel);
        const auto& f = k_formulas[matches[i]];
        auto sig_e  = text(f.sig)  | size(WIDTH, EQUAL, 30);
        auto desc_e = text(f.desc);
        auto e = hbox({ text(" "), std::move(sig_e), text("  "), std::move(desc_e), text(" "), filler() });
        if (s)
            e = e | bgcolor(m_cfg.colors.cursor_bg) | color(m_cfg.colors.cursor_fg);
        else
            e = e | color(m_cfg.colors.dimmed);
        items.push_back(std::move(e));
    }
    return window(
        hbox({ text(" "), text("formulas") | bold | color(m_cfg.colors.header), text(" ") }),
        vbox(std::move(items))
    );
}

Component Grid::make_component() {
    auto renderer = Renderer([this] {
        Elements elems;
        elems.push_back(formula_bar());
        if (m_editing && should_show_ac(m_edit_buf) && !ac_matches().empty())
            elems.push_back(render_ac_popup());
        elems.push_back(hbox({ render() | flex, vscrollbar() }) | flex);
        return vbox(std::move(elems));
    });
    return CatchEvent(renderer, [this](Event e) -> bool {
        if (m_pending_delete_row >= 0 || m_pending_delete_col >= 0)
            return handle_pending_delete(e);
        if (m_editing && !e.is_mouse()) return handle_cell_editing(e);
        if (handle_normal_nav(e)) return true;
        return handle_mouse(e);
    });
}

// ── Event handlers ────────────────────────────────────────────────────────────

bool Grid::handle_pending_delete(Event e) {
    const bool is_row = (m_pending_delete_row >= 0);
    int& pending = is_row ? m_pending_delete_row : m_pending_delete_col;
    if (e.is_mouse()) {
        if (e.mouse().button == Mouse::Left && e.mouse().motion == Mouse::Pressed)
            pending = -1;
        return false;
    }
    const bool confirm = (e == Event::Character('y') || e == Event::Character('Y') || e == Event::Return);
    if (confirm) {
        if (is_row) delete_row(pending);
        else        delete_col(pending);
    }
    pending = -1;
    return true;
}

bool Grid::handle_cell_editing(Event e) {
    // Autocomplete popup navigation (cells only — headers don't take formulas)
    if (m_cursor_row >= 0 && should_show_ac(m_edit_buf)) {
        const auto matches = ac_matches();
        if (!matches.empty()) {
            const int n = (int)matches.size();
            if (e == Event::ArrowUp)   { m_ac_sel = std::max(0, m_ac_sel - 1); return true; }
            if (e == Event::ArrowDown) { m_ac_sel = std::min(n - 1, m_ac_sel + 1); return true; }
            if (e == Event::Tab || e == Event::Return) {
                const int idx = std::clamp(m_ac_sel, 0, n - 1);
                const std::string pfx = ac_prefix(m_edit_buf);
                m_edit_buf = m_edit_buf.substr(0, m_edit_buf.size() - pfx.size())
                             + k_formulas[matches[idx]].name + "(";
                m_ac_sel = 0;
                return true;
            }
        }
    }

    if (e == Event::Escape)     { commit_edit(); m_insert_sticky = false; return true; }
    if (e == Event::Delete)     { m_edit_buf.clear(); return true; }
    if (e == Event::Return)     { commit_and_step( 1,  0); return true; }
    if (e == Event::Tab)        { commit_and_step( 0,  1); return true; }
    if (e == Event::TabReverse) { commit_and_step( 0, -1); return true; }
    if (e == Event::ArrowDown)  { commit_and_step( 1,  0); return true; }
    if (e == Event::ArrowLeft)  { commit_and_step( 0, -1); return true; }
    if (e == Event::ArrowRight) { commit_and_step( 0,  1); return true; }
    if (e == Event::ArrowUp) {
        if (m_cursor_row < 0) return true;       // already in the header, keep editing
        commit_and_step(-1, 0); return true;     // row 0 → header, else up one row
    }
    if (e == Event::Backspace)  { m_ac_sel = 0; if (!m_edit_buf.empty()) m_edit_buf.pop_back(); return true; }
    if (e.is_character())       { m_ac_sel = 0; m_edit_buf += e.character(); return true; }
    return true;  // consume all other keyboard events while editing
}

bool Grid::handle_normal_nav(Event e) {
    if (m_cursor_row >= 0 && m_cursor_col >= 0) {   // selection only over data cells
        if (e == ShiftArrowUp)    { extend_selection(-1,  0); return true; }
        if (e == ShiftArrowDown)  { extend_selection( 1,  0); return true; }
        if (e == ShiftArrowLeft)  { extend_selection( 0, -1); return true; }
        if (e == ShiftArrowRight) { extend_selection( 0,  1); return true; }
    }

    auto nav_move = [&](int dr, int dc) {
        m_has_selection = false;
        move(dr, dc);
        if (m_insert_sticky && m_cursor_col >= 0) start_edit(false);  // header or cell
    };

    if (e == Event::Escape && m_insert_sticky) { m_insert_sticky = false; return true; }
    if (e == Event::Escape && m_has_selection)  { m_has_selection = false; return true; }
    if (e == Event::ArrowUp || m_cfg.key_is(e, m_cfg.keys.nav_up)) {
        nav_move(-1, 0); return true;   // row 0 → header (-1); sticky auto-edits the name
    }
    if (e == Event::ArrowDown  || m_cfg.key_is(e, m_cfg.keys.nav_down))  { nav_move( 1,  0); return true; }
    if (e == Event::ArrowLeft  || m_cfg.key_is(e, m_cfg.keys.nav_left))  { nav_move( 0, -1); return true; }
    if (e == Event::ArrowRight || m_cfg.key_is(e, m_cfg.keys.nav_right)) { nav_move( 0,  1); return true; }
    if (e == Event::PageUp)     { m_has_selection = false; page_up();   return true; }
    if (e == Event::PageDown)   { m_has_selection = false; page_down(); return true; }
    if (e == Event::Home)       { m_has_selection = false; move_home(); return true; }
    if (e == Event::End)        { m_has_selection = false; move_end();  return true; }
    if (e == Event::Return)     { m_has_selection = false; move(1, 0); return true; }
    if (e == Event::Tab)        { m_has_selection = false; move(0, 1); return true; }
    if (e == Event::TabReverse) { move(0,-1); return true; }
    if (m_cfg.key_is(e, m_cfg.keys.insert_mode)) { start_edit(false); return true; }  // i/a: edit (cell or header name)
    if (m_cursor_row < 0) {  // column header: action box inserts / deletes columns
        if (m_cfg.key_is(e, m_cfg.keys.insert_col)) { insert_col(m_cursor_col);     return true; }
        if (m_cfg.key_is(e, m_cfg.keys.delete_col)) { try_delete_col(m_cursor_col); return true; }
    }
    if (m_cursor_col < 0) {  // row index: action box inserts / deletes rows
        if (m_cfg.key_is(e, m_cfg.keys.insert_row)) { insert_row(m_cursor_row); m_cursor_col = -1; return true; }
        if (m_cfg.key_is(e, m_cfg.keys.delete_row)) { try_delete_row(m_cursor_row); return true; }
    }
    if (e == Event::Backspace || m_cfg.key_is(e, m_cfg.keys.delete_cell)) {
        if (m_cursor_row < 0)      { try_delete_col(m_cursor_col); }
        else if (m_cursor_col < 0) { try_delete_row(m_cursor_row); }
        else {
            auto& cell = at(m_cursor_row, m_cursor_col);
            if (!cell.value().empty()) {
                m_undo_stack.push_back({m_cursor_row, m_cursor_col, cell.value(), ""});
                m_redo_stack.clear();
                cell.set_value("");
                m_col_widths[m_cursor_col] = compute_col_width(m_cursor_col);
            }
        }
        return true;
    }
    if (m_cfg.key_is(e, m_cfg.keys.undo)) { undo(); return true; }
    if (e == Event::Special("\x12"))      { redo(); return true; }  // Ctrl+R

    if (!m_insert_sticky && m_cursor_row >= 0 && m_cursor_col >= 0 && e.is_character()) {
        const auto ch = e.character();
        if (ch == "g") {
            if (m_pending_g) {
                m_pending_g = false;
                m_has_selection = false;
                m_cursor_row = 0;
                adjust_viewport();
            } else {
                m_pending_g = true;
            }
            return true;
        }
        m_pending_g = false;
        if (ch == "G") {
            m_has_selection = false;
            m_cursor_row = m_rows - 1;
            adjust_viewport();
            return true;
        }
        if (ch == "0") {
            m_has_selection = false;
            m_cursor_col = 0;
            adjust_viewport();
            return true;
        }
        if (ch == "$") {
            m_has_selection = false;
            m_cursor_col = m_cols - 1;
            adjust_viewport();
            return true;
        }
        if (ch == "o") {
            insert_row(m_cursor_row);
            start_edit(false);
            return true;
        }
        if (ch == "O") {
            insert_row(m_cursor_row - 1);
            start_edit(false);
            return true;
        }
    } else {
        m_pending_g = false;
    }

    if (!m_insert_sticky && m_cursor_row >= 0 && m_cursor_col >= 0 && e.is_character() && e.character() == "y") {
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
        return true;
    }
    if (!m_insert_sticky && m_cursor_row >= 0 && m_cursor_col >= 0 && e.is_character() && e.character() == "p"
            && !m_yank_data.empty()) {
        const int need_rows = m_cursor_row + (int)m_yank_data.size();
        const int need_cols = m_cursor_col + (int)m_yank_data[0].size();
        while (m_rows < need_rows) {
            m_cells.emplace_back(m_cols);
            m_action_boxes.emplace_back();
            ++m_rows;
        }
        while (m_cols < need_cols) {
            for (auto& row : m_cells) row.emplace_back();
            m_col_names.push_back(col_letter(m_cols));
            m_col_widths.push_back(k_cell_w);
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
        return true;
    }
    if (!e.is_mouse()) return true;       // consume all other keyboard events in NORMAL
    return false;                         // mouse falls through to handle_mouse
}

bool Grid::handle_mouse(Event e) {
    if (!e.is_mouse()) return false;

    const int mx = e.mouse().x, my = e.mouse().y;

    // Update ActionBox hover states
    for (int r = 0; r < m_rows; ++r)
        m_action_boxes[r].set_hovered(m_action_boxes[r].contains_cell(mx, my));
    for (int c = 0; c < m_cols; ++c)
        m_col_action_boxes[c].set_hovered(m_col_action_boxes[c].contains_cell(mx, my));

    if (e.mouse().button == Mouse::WheelUp)   { move(-3, 0); return true; }
    if (e.mouse().button == Mouse::WheelDown) { move( 3, 0); return true; }

    const int  sb_x  = m_box.x_max + 1;
    const bool on_sb = (mx == sb_x && my >= m_box.y_min && my <= m_box.y_max);

    if (e.mouse().button == Mouse::Left && e.mouse().motion == Mouse::Pressed) {
        if (on_sb) { scroll_to_mouse_y(my); return true; }
        for (int r = 0; r < m_rows; ++r) {
            if (m_action_boxes[r].contains_plus (mx, my)) { insert_row(r);     return true; }
            if (m_action_boxes[r].contains_minus(mx, my)) { try_delete_row(r); return true; }
        }
        for (int c = 0; c < m_cols; ++c) {
            if (m_col_action_boxes[c].contains_plus (mx, my)) { insert_col(c);     return true; }
            if (m_col_action_boxes[c].contains_minus(mx, my)) { try_delete_col(c); return true; }
        }
        return select_at_mouse(mx, my);
    }
    return false;
}

int Grid::vis_rows() const {
    const int h = m_box.y_max - m_box.y_min + 1;
    if (h > 4) return std::max(1, (h - 4) / 2);
    return std::max(1, Terminal::Size().dimy / 3);
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
    const int vr = vis_rows(), vc = vis_cols();
    if (m_cursor_row >= 0) {   // header row (-1) is always pinned above the grid
        if (m_cursor_row < m_offset_row)         m_offset_row = m_cursor_row;
        if (m_cursor_row >= m_offset_row + vr)   m_offset_row = m_cursor_row - vr + 1;
    }
    if (m_cursor_col >= 0) {
        if (m_cursor_col < m_offset_col)         m_offset_col = m_cursor_col;
        if (m_cursor_col >= m_offset_col + vc)   m_offset_col = m_cursor_col - vc + 1;
    }
}

int Grid::compute_col_width(int c) const {
    int w = static_cast<int>(m_col_names[c].size()) + ActionBox::k_width + 1;
    for (int r = 0; r < m_rows; ++r)
        w = std::max(w, static_cast<int>(cell_display(r, c).size()));
    return std::max(w, ActionBox::k_width + 3);
}

std::string Grid::col_letter(int c) const {
    std::string s;
    do { s = char('A' + c % 26) + s; c = c / 26 - 1; } while (c >= 0);
    return s;
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
