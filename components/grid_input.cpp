// grid_input.cpp — the FTXUI component, event dispatch, the per-mode keyboard
// handlers, and mouse hit-testing / drag handling. (Part of the Grid class; see
// grid.hpp.)
#include "grid.hpp"

#include <algorithm>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "formula_catalog.hpp"

using namespace ftxui;

namespace {

const Event ShiftArrowUp    = Event::Special("\x1B[1;2A");
const Event ShiftArrowDown  = Event::Special("\x1B[1;2B");
const Event ShiftArrowRight = Event::Special("\x1B[1;2C");
const Event ShiftArrowLeft  = Event::Special("\x1B[1;2D");

}  // namespace

// ── Component & dispatch ─────────────────────────────────────────────────────

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
        if (m_searching && !e.is_mouse()) return handle_search(e);
        if (m_editing && !e.is_mouse()) return handle_cell_editing(e);
        if (handle_normal_nav(e)) return true;
        return handle_mouse(e);
    });
}

// ── Keyboard handlers ────────────────────────────────────────────────────────

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
                m_edit_cursor = (int)m_edit_buf.size();
                m_ac_sel = 0;
                return true;
            }
        }
    }

    if (e == Event::Escape)     { commit_edit(); m_insert_sticky = false; return true; }
    if (e == Event::Delete)     { m_edit_buf.clear(); m_edit_cursor = 0; return true; }
    if (e == Event::Return)     { commit_and_step( 1,  0); return true; }
    if (e == Event::Tab)        { commit_and_step( 0,  1); return true; }
    if (e == Event::TabReverse) { commit_and_step( 0, -1); return true; }
    if (e == Event::ArrowDown)  { commit_and_step( 1,  0); return true; }
    if (e == Event::ArrowLeft)  {
        if (m_edit_typed) { if (m_edit_cursor > 0) --m_edit_cursor; return true; }
        commit_and_step(0, -1); return true;
    }
    if (e == Event::ArrowRight) {
        if (m_edit_typed) { if (m_edit_cursor < (int)m_edit_buf.size()) ++m_edit_cursor; return true; }
        commit_and_step(0,  1); return true;
    }
    if (e == Event::ArrowUp) {
        if (m_cursor_row < 0) return true;       // already in the header, keep editing
        commit_and_step(-1, 0); return true;     // row 0 → header, else up one row
    }
    if (e == Event::Backspace)  {
        m_ac_sel = 0;
        if (m_edit_cursor > 0) {
            m_edit_buf.erase(m_edit_cursor - 1, 1);
            --m_edit_cursor;
        }
        return true;
    }
    if (e.is_character()) {
        m_ac_sel = 0;
        m_edit_typed = true;
        m_edit_buf.insert(m_edit_cursor, e.character());
        m_edit_cursor += (int)e.character().size();
        return true;
    }
    return true;  // consume all other keyboard events while editing
}

bool Grid::handle_search(Event e) {
    if (e == Event::Escape) { cancel_search(); return true; }
    if (e == Event::Return) { commit_search(); return true; }
    if (e == Event::Backspace) {
        if (m_search_buf.empty()) { cancel_search(); return true; }
        m_search_buf.pop_back();
        update_search();
        return true;
    }
    if (e.is_character()) {
        m_search_buf += e.character();
        update_search();
        return true;
    }
    return true;  // swallow other keys while the search prompt is open
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
    if (e == Event::Escape && !m_search_query.empty()) {  // clear match highlight
        m_search_query.clear(); m_search_hits.clear(); return true;
    }
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
    // Manual column resize: works whenever a column is under the cursor (header
    // or data cell). Checked before the insert/edit keys so `<`/`>` never leak
    // into editing or the data-cell command block below.
    if (m_cursor_col >= 0) {
        if (m_cfg.key_is(e, m_cfg.keys.col_widen))  { resize_col(m_cursor_col,  1); return true; }
        if (m_cfg.key_is(e, m_cfg.keys.col_narrow)) { resize_col(m_cursor_col, -1); return true; }
    }
    if (m_cursor_row >= 0) {   // grow/shrink the current row's height
        if (m_cfg.key_is(e, m_cfg.keys.row_taller))  { resize_row(m_cursor_row,  1); return true; }
        if (m_cfg.key_is(e, m_cfg.keys.row_shorter)) { resize_row(m_cursor_row, -1); return true; }
    }
    if (m_cfg.key_is(e, m_cfg.keys.heatmap)) { toggle_heatmap(); return true; }
    if (m_cfg.key_is(e, m_cfg.keys.insert_mode) || e == Event::F2) { start_edit(false); return true; }  // i/a/F2: edit (cell or header name)
    if (m_cursor_row < 0) {  // column header: action box inserts / deletes columns
        if (m_cfg.key_is(e, m_cfg.keys.insert_col)) { insert_col(m_cursor_col);     return true; }
        if (m_cfg.key_is(e, m_cfg.keys.delete_col)) { try_delete_col(m_cursor_col); return true; }
        if (m_cfg.key_is(e, m_cfg.keys.sort_col)) {   // toggle ascending ↔ descending
            const bool desc = (m_sort_col == m_cursor_col) ? !m_sort_desc : false;
            sort_by({{m_cursor_col, desc}});
            return true;
        }
    }
    if (m_cursor_col < 0) {  // row index: action box inserts / deletes rows
        if (m_cfg.key_is(e, m_cfg.keys.insert_row)) { insert_row(m_cursor_row); m_cursor_col = -1; return true; }
        if (m_cfg.key_is(e, m_cfg.keys.delete_row)) { try_delete_row(m_cursor_row); return true; }
    }
    if (e == Event::Backspace || m_cfg.key_is(e, m_cfg.keys.delete_cell)) {
        if (m_cursor_row < 0)      { try_delete_col(m_cursor_col); }
        else if (m_cursor_col < 0) { try_delete_row(m_cursor_row); }
        else                       { clear_cell(m_cursor_row, m_cursor_col); }
        return true;
    }
    if (m_cfg.key_is(e, m_cfg.keys.undo)) { undo(); return true; }
    if (e == Event::Special("\x12"))      { redo(); return true; }  // Ctrl+R

    // `/` opens search; n/N step through matches (vim-style).
    if (!m_insert_sticky && e == Event::Character('/')) { start_search();  return true; }
    if (!m_insert_sticky && e == Event::Character('n')) { search_step( 1); return true; }
    if (!m_insert_sticky && e == Event::Character('N')) { search_step(-1); return true; }

    // Single-key NORMAL-mode commands, only on a data cell (vim-style).
    if (!m_insert_sticky && m_cursor_row >= 0 && m_cursor_col >= 0 && e.is_character()) {
        const std::string ch = e.character();
        // 'g' is the lone multi-key prefix (gg → top); any other key clears it.
        if (ch == "g") {
            if (m_pending_g) { m_pending_g = false; m_has_selection = false; m_cursor_row = 0; adjust_viewport(); }
            else             { m_pending_g = true; }
            return true;
        }
        m_pending_g = false;
        if (ch == "G") { m_has_selection = false; m_cursor_row = m_rows - 1; adjust_viewport(); return true; }
        if (ch == "0") { m_has_selection = false; m_cursor_col = 0;          adjust_viewport(); return true; }
        if (ch == "$") { m_has_selection = false; m_cursor_col = m_cols - 1; adjust_viewport(); return true; }
        if (ch == "o") { insert_row(m_cursor_row);     start_edit(false);    return true; }
        if (ch == "O") { insert_row(m_cursor_row - 1); start_edit(false);    return true; }
        if (ch == "y") { yank_selection(); return true; }
        if (ch == "p" && !m_yank_data.empty()) { paste_yanked(); return true; }
        return true;  // swallow any other char while on a data cell
    }
    m_pending_g = false;

    if (!e.is_mouse()) return true;       // consume all other keyboard events in NORMAL
    return false;                         // mouse falls through to handle_mouse
}

// ── Mouse ────────────────────────────────────────────────────────────────────

void Grid::scroll_to_mouse_y(int my) {
    const int bar_h   = m_box.y_max - m_box.y_min + 1;
    const int ry      = my - m_box.y_min;
    const int max_off = std::max(0, m_rows - vis_rows());
    m_offset_row = std::clamp(ry * max_off / std::max(1, bar_h - 1), 0, max_off);
    m_cursor_row = std::clamp(m_cursor_row,
                              m_offset_row,
                              std::min(m_rows - 1, m_offset_row + vis_rows() - 1));
}

int Grid::col_at_x(int rx) const {
    int x = rx - (ActionBox::k_width + k_rownum_w);   // skip the row-number gutter
    for (int c = m_offset_col; c < m_cols; ++c) {
        x -= 1 + m_col_widths[c];                     // separator + cell
        if (x < 0) return c;
    }
    return -1;
}

int Grid::row_at_y(int ry) const {
    if (ry < 2) return -1;                            // 0 = header, 1 = heavy separator
    int line = 2;
    for (int r = m_offset_row; r < m_rows; ++r) {
        if (ry >= line && ry < line + m_row_heights[r]) return r;
        line += m_row_heights[r] + 1;                 // content lines + separator
        if (line > ry) break;
    }
    return -1;
}

bool Grid::select_at_mouse(int mx, int my) {
    if (mx <= m_box.x_min || mx >= m_box.x_max) return false;
    if (my <= m_box.y_min || my >= m_box.y_max) return false;

    const int rx = mx - m_box.x_min - 1;
    const int ry = my - m_box.y_min - 1;

    if (ry == 0 && rx >= ActionBox::k_width + k_rownum_w) {   // header band → header row (-1)
        const int c = col_at_x(rx);
        if (c < 0) return false;
        commit_edit();
        m_cursor_row = -1;
        m_cursor_col = c;
        adjust_viewport();
        return true;
    }

    const int r = row_at_y(ry);
    const int c = col_at_x(rx);
    if (r < 0 || r >= m_rows || c < 0 || c >= m_cols) return false;

    commit_edit();
    m_cursor_row = r;
    m_cursor_col = c;
    adjust_viewport();
    return true;
}

void Grid::drag_select_to(int mx, int my) {
    m_status_msg.clear();   // let the selection hint show (move() does this for keys)
    const int rx = mx - m_box.x_min - 1;
    const int ry = my - m_box.y_min - 1;
    const int last_row = std::min(m_rows - 1, m_offset_row + vis_rows() - 1);
    const int last_col = std::min(m_cols - 1, m_offset_col + vis_cols() - 1);

    // Column: col_at_x folds each separator into the adjacent cell, so it only
    // returns -1 in the gutter or past the last column — clamp those.
    int c = col_at_x(rx);
    if (c < 0) c = (rx < ActionBox::k_width + k_rownum_w) ? m_offset_col : last_col;

    // Row: snap to the nearest row, treating the separator line *below* a row as
    // belonging to that row. (row_at_y returns -1 on separators, which made the
    // drag jump to the bottom row and flicker as the cursor crossed them.)
    int r;
    if (ry < 2) {
        r = m_offset_row;
    } else {
        r = last_row;
        for (int rr = m_offset_row, line = 2; rr < m_rows; ++rr) {
            if (ry <= line + m_row_heights[rr]) { r = rr; break; }  // content or its separator
            line += m_row_heights[rr] + 1;
        }
    }

    m_cursor_row = r;
    m_cursor_col = c;
    m_has_selection = (r != m_sel_row || c != m_sel_col);   // a single cell ⇒ no range
    adjust_viewport();
}

bool Grid::handle_mouse(Event e) {
    if (!e.is_mouse()) return false;

    const int mx = e.mouse().x, my = e.mouse().y;
    m_header_hover = -1;   // cleared each event; re-set below for plain hovers

    // A held-Left drag step. With "any-event" tracking the initial press is a
    // Left+Pressed; each subsequent move while the button is down is a Left+Moved
    // (FTXUI ≥ v6). Either continues a drag; a Released (or anything else) ends it.
    const bool drag_held =
        e.mouse().button == Mouse::Left && e.mouse().motion != Mouse::Released;

    if (m_resize_col >= 0) {                    // a column-width drag in progress
        if (drag_held) {
            set_col_width(m_resize_col, m_resize_start_w + (mx - m_resize_start_x));
            m_resize_hover = m_resize_col;
            return true;
        }
        m_resize_col = m_resize_hover = -1;   // release (or anything else) ends the drag
        return true;
    }
    if (m_resize_row >= 0) {                   // same, for a row-height drag
        if (drag_held) {
            set_row_height(m_resize_row, m_resize_start_h + (my - m_resize_start_y));
            m_resize_hrow = m_resize_row;
            return true;
        }
        m_resize_row = m_resize_hrow = -1;
        return true;
    }
    if (m_mouse_selecting) {                    // painting a multi-cell selection
        if (drag_held) {
            drag_select_to(mx, my);
            return true;
        }
        m_mouse_selecting = false;             // button released → finish
        return true;
    }

    // Update ActionBox hover states
    for (int r = 0; r < m_rows; ++r)
        m_action_boxes[r].set_hovered(m_action_boxes[r].contains_cell(mx, my));
    for (int c = 0; c < m_cols; ++c)
        m_col_action_boxes[c].set_hovered(m_col_action_boxes[c].contains_cell(mx, my));

    // Highlight the grabbable column boundary while hovering the header band, and
    // the grabbable row boundary while hovering the left row-number gutter.
    const int rx = mx - m_box.x_min - 1;
    const int ry = my - m_box.y_min - 1;
    const int k_gutter = ActionBox::k_width + k_rownum_w;
    m_resize_hover = (ry == 0) ? border_hit(rx) : -1;
    m_resize_hrow  = (rx >= 0 && rx < k_gutter) ? row_border_hit(ry) : -1;
    // Header sort affordance: a column under the mouse (but not over its resize
    // border, which owns its own ⇔ handle) shows a clickable ▲ hint.
    m_header_hover = (ry == 0 && rx >= k_gutter && m_resize_hover < 0) ? col_at_x(rx) : -1;

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
        // Grab a column boundary to start a resize drag (checked after the action
        // boxes so their +/- keep priority over the surrounding separator).
        if (m_resize_hover >= 0) {
            m_resize_col     = m_resize_hover;
            m_resize_start_x = mx;
            m_resize_start_w = m_col_widths[m_resize_hover];
            return true;
        }
        // Grab a row boundary in the gutter to start a height drag (after the
        // row +/- action boxes above, so those keep priority).
        if (m_resize_hrow >= 0) {
            m_resize_row     = m_resize_hrow;
            m_resize_start_y = my;
            m_resize_start_h = m_row_heights[m_resize_hrow];
            return true;
        }
        if (!select_at_mouse(mx, my)) return false;
        // Landing on a data cell anchors a drag-selection; dragging to another
        // cell turns it into a range. A plain click (press+release, no move)
        // just leaves the cursor here with no selection.
        if (m_cursor_row >= 0 && m_cursor_col >= 0) {
            m_sel_row = m_cursor_row;
            m_sel_col = m_cursor_col;
            m_has_selection = false;
            m_mouse_selecting = true;
        } else if (m_cursor_row < 0 && m_cursor_col >= 0) {
            // Click on a column header (not a +/- box or resize border) sorts it,
            // toggling ascending ↔ descending — same as the `s` key.
            const bool desc = (m_sort_col == m_cursor_col) ? !m_sort_desc : false;
            sort_by({{m_cursor_col, desc}});
        }
        return true;
    }
    return false;
}
