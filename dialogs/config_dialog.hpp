#pragma once
#include <functional>
#include <string>
#include <vector>
#include <ftxui/component/component.hpp>

struct Config;

class ConfigDialog {
public:
    ConfigDialog(const Config& cfg,
                 std::function<void()> on_close);

    void refresh_from_cfg();   // re-read cfg values into buffers (call on open)
    void save_to_file();       // write current buffers to ~/.config/tuix/config.toml
    ftxui::Component component();

private:
    const Config& m_cfg;
    std::function<void()> m_on_close;

    // ── buffers ─────────────────────────────────────────────────────────────
    std::string cb_cursor_bg, cb_cursor_fg, cb_selection_bg, cb_selection_fg,
                cb_header, cb_row_number, cb_dimmed,
                cb_insert_badge_bg, cb_insert_badge_fg,
                cb_normal_badge_bg, cb_normal_badge_fg,
                cb_titlebar_bg, cb_titlebar_fg, cb_formula_fg;
    std::string kb_nav_up, kb_nav_down, kb_nav_left, kb_nav_right,
                kb_insert_mode, kb_delete_cell, kb_undo,
                kb_insert_row, kb_delete_row, kb_insert_col, kb_delete_col,
                kb_rename_col, kb_cmd_mode;
    std::string gb_cell_width;
    std::string gb_start_mode;
    std::string m_status;

    int m_tab = 0;
    std::vector<std::string> m_tab_names;

    // ── components ─────────────────────────────────────────────────────────
    ftxui::Component in_cursor_bg, in_cursor_fg, in_selection_bg, in_selection_fg,
                     in_header, in_row_number, in_dimmed,
                     in_insert_badge_bg, in_insert_badge_fg,
                     in_normal_badge_bg, in_normal_badge_fg,
                     in_titlebar_bg, in_titlebar_fg, in_formula_fg;
    ftxui::Component in_nav_up, in_nav_down, in_nav_left, in_nav_right,
                     in_insert_mode, in_delete_cell, in_undo,
                     in_insert_row, in_delete_row, in_insert_col, in_delete_col,
                     in_rename_col, in_cmd_mode;
    ftxui::Component in_cell_width;
    ftxui::Component in_start_mode;
    ftxui::Component m_tab_toggle, m_content, m_save_btn, m_container;
};
