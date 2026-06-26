// grid_render.cpp — all FTXUI element building: the formula bar, the grid body,
// the scrollbar, the context hint, and the formula autocomplete popup. (Part of
// the Grid class; see grid.hpp.)
#include "grid.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <utility>

#include <ftxui/dom/elements.hpp>

#include "formula_catalog.hpp"
#include "heatmap.hpp"
#include "util/text_width.hpp"

using namespace ftxui;

std::string Grid::context_hint() const {
    if (m_searching)
        return "type to search  |  Enter: confirm  |  Esc: cancel  |  then n/N to step";
    if (m_pending_delete_row >= 0 || m_pending_delete_col >= 0)
        return "y: confirm delete  |  n/Esc: cancel";
    if (m_editing) {
        if (m_cursor_row < 0)
            return "Enter: confirm & ↓  |  Tab/→: confirm & next col  |  Esc: confirm & NORMAL";
        if (should_show_ac(m_edit_buf) && !ac_matches().empty())
            return "↑↓: navigate  |  Tab/Enter: complete  |  type to filter";
        return "Enter: confirm & ↓  |  Tab: confirm & →  |  Arrows: confirm & move  |  Del: clear";
    }
    if (!m_status_msg.empty())
        return m_status_msg;
    if (m_cursor_row < 0)
        return "hjkl/arrows: nav  |  +: insert col  |  -/x: delete col  |  i/a/F2: rename  |  <>: resize  |  ↓: into grid";
    if (m_cursor_col < 0)
        return "hjkl/arrows: nav  |  +: insert row  |  -/x: delete row  |  }{: resize  |  u/Ctrl+R: undo/redo  |  →: enter row";
    if (m_has_selection && m_cursor_row >= 0 && m_cursor_col >= 0)
        return "selection  |  y: copy → clipboard  |  H: heatmap  |  Shift+arrows: extend  |  Esc: clear";
    return "hjkl: nav  |  i/a/F2: edit  |  o/O: new row  |  x: delete  |  y/p: yank/paste  |  Shift+arrows: select  |  gg/G: top/bottom  |  /n N: search  |  u/Ctrl+R: undo/redo  |  :: cmd";
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
    if (m_searching) {
        const std::string count = m_search_hits.empty()
            ? "no matches"
            : std::to_string(m_search_hits.size()) + (m_search_hits.size() == 1 ? " match" : " matches");
        return window(
            hbox({ text(" "), text("search") | bold | color(m_cfg.colors.header), text(" ") }),
            hbox({ text(" /"), text(m_search_buf), text("_"), filler(),
                   text(count + " ") | color(m_cfg.colors.dimmed) })
        );
    }
    std::string label = cursor_label();
    std::string content;
    if (m_cursor_col < 0)       content = "";                          // row-index gutter
    else if (m_editing)         content = m_edit_buf.substr(0, m_edit_cursor) + "_" + m_edit_buf.substr(m_edit_cursor);
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

    // The separator drawn before column c is column (c-1)'s right edge. When the
    // mouse hovers that boundary, mark it on the header row with a "⇔" grab
    // handle so the user sees the column can be dragged to resize. Body rows keep
    // a plain separator so the indicator stays a single, legible glyph.
    auto col_sep = [&](int c, bool header) -> Element {
        if (header && m_resize_hover >= 0 && c - 1 == m_resize_hover)
            return text("⇔") | color(m_cfg.colors.cursor_bg) | bold | size(WIDTH, EQUAL, 1);
        return separator();
    };

    // Horizontal separator below row r. When the mouse hovers that row's bottom
    // border, show a "⇕" grab handle in the gutter (a vbox keeps the trailing
    // separator horizontal inside the surrounding hbox).
    auto row_sep = [&](int r) -> Element {
        if (m_resize_hrow == r)
            return hbox({ text("⇕") | color(m_cfg.colors.cursor_bg) | bold | size(WIDTH, EQUAL, 1),
                          vbox({ separator() }) | flex });
        return separator();
    };

    // Heatmap background+foreground for a numeric cell inside the active region.
    auto heat_at = [&](int r, int c) -> std::optional<std::pair<Color, Color>> {
        if (!m_heat_active) return std::nullopt;
        if (r < m_heat_r0 || r > m_heat_r1 || c < m_heat_c0 || c > m_heat_c1) return std::nullopt;
        double v;
        if (!cell_value(r, c).to_number(v)) return std::nullopt;
        const double t = (m_heat_max > m_heat_min) ? (v - m_heat_min) / (m_heat_max - m_heat_min) : 0.5;
        int R, G, B;
        heat_rgb(t, R, G, B);
        const Color fg = (0.299 * R + 0.587 * G + 0.114 * B) > 140 ? Color::Black : Color::White;
        return std::make_pair(Color::RGB(R, G, B), fg);
    };

    Elements header;
    header.push_back(text(std::string(ActionBox::k_width + k_rownum_w, ' ')) | bold | color(m_cfg.colors.header));
    for (int c = m_offset_col; c < c_end; ++c) {
        header.push_back(col_sep(c, /*header=*/true));
        const bool hsel    = (m_cursor_row < 0 && c == m_cursor_col);
        // width = name + " " + action box + trailing " " (gap before the column's
        // right separator, so the +/- box never crowds the resize ⇔ handle).
        const int  name_w  = m_col_widths[c] - ActionBox::k_width - 2;
        std::string sort_mark;
        if (!(hsel && m_editing)) {
            if (c == m_sort_col)            sort_mark = m_sort_desc ? " ▼" : " ▲";
            else if (c == m_header_hover)   sort_mark = " ▲";   // hover hint: click to sort
        }
        std::string name = (hsel && m_editing)
            ? m_edit_buf.substr(0, m_edit_cursor) + "_" + m_edit_buf.substr(m_edit_cursor)
            : m_col_names[c];
        const int name_budget = std::max(0, name_w - (sort_mark.empty() ? 0 : 2));
        if ((int)name.size() > name_budget) name = name.substr(0, name_budget);
        name += sort_mark;
        auto name_e = text(name) | size(WIDTH, EQUAL, name_w) | bold;
        name_e = hsel ? name_e | bgcolor(m_cfg.colors.cursor_bg) | color(m_cfg.colors.cursor_fg)
                      : name_e | color(m_cfg.colors.header);
        header.push_back(
            hbox({ std::move(name_e), text(" "), m_col_action_boxes[c].render(hsel), text(" ") })
            | reflect(m_col_action_boxes[c].cell_box())
        );
    }


    header.push_back(filler());

    Elements elems;
    elems.push_back(hbox(std::move(header)));
    elems.push_back(separatorHeavy());

    for (int r = m_offset_row; r < r_end; ++r) {
        Elements row;
        const int rh = m_row_heights[r];

        // First cell: index + action box, stretched to the row's height.
        if (r == m_pending_delete_row)
            row.push_back(vbox({ text("del?") | bold | color(Color::Red), filler() })
                          | size(WIDTH, EQUAL, k_rownum_w + ActionBox::k_width) | size(HEIGHT, EQUAL, rh));
        else {
            const bool idx_active = (m_cursor_col < 0 && r == m_cursor_row);
            Element gutter = hbox({
                m_action_boxes[r].render(idx_active),
                text(std::to_string(r + 1)) | align_right | size(WIDTH, EQUAL, k_rownum_w) | color(m_cfg.colors.row_number),
            }) | reflect(m_action_boxes[r].cell_box());
            row.push_back(vbox({ std::move(gutter), filler() }) | size(HEIGHT, EQUAL, rh));
        }

        for (int c = m_offset_col; c < c_end; ++c) {
            row.push_back(col_sep(c, /*header=*/false));
            const bool is_cursor  = (r == m_cursor_row && c == m_cursor_col);
            const bool is_yanked  = m_yank_row >= 0 && !m_yank_data.empty() &&
                r >= m_yank_row && r < m_yank_row + (int)m_yank_data.size() &&
                c >= m_yank_col && c < m_yank_col + (int)m_yank_data[0].size();
            const std::string& raw = m_cells[r][c].value();
            const bool is_formula = !raw.empty() && raw[0] == '=';
            std::string val = (is_cursor && m_editing) ? m_edit_buf : cell_display(r, c);
            val = tuix::truncate_to_width(val, m_col_widths[c]);
            // Stretch the cell to the row's height; content sits on the top line
            // and the highlight (if any) fills the whole box.
            auto e = vbox({ text(val), filler() })
                     | size(WIDTH, EQUAL, m_col_widths[c]) | size(HEIGHT, EQUAL, rh);
            if (is_cursor)
                e = e | bgcolor(m_cfg.colors.cursor_bg) | color(m_cfg.colors.cursor_fg);
            else if (auto h = heat_at(r, c))
                e = e | bgcolor(h->first) | color(h->second);
            else if (is_yanked)
                e = e | color(Color::Yellow);
            else if (in_selection(r, c))
                e = e | bgcolor(m_cfg.colors.selection_bg) | color(m_cfg.colors.selection_fg);
            else if (!m_search_hits.empty() &&
                     std::binary_search(m_search_hits.begin(), m_search_hits.end(), std::make_pair(r, c)))
                e = e | bgcolor(Color::Cyan) | color(Color::Black);
            else if (is_formula)
                e = e | color(m_cfg.colors.formula_fg);
            row.push_back(e);
        }
        row.push_back(filler());
        elems.push_back(hbox(std::move(row)));
        elems.push_back(row_sep(r));
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
