#include "config_dialog.hpp"
#include "dialog_shell.hpp"

#include "titlebar.hpp"
#include "config/config.hpp"

#include <filesystem>
#include <fstream>
#include <toml++/toml.hpp>

using namespace ftxui;

namespace {
std::string kstr(const std::vector<char>& v) {
    std::string r;
    for (char c : v) { if (!r.empty()) r += ' '; r += c; }
    return r;
}
toml::array make_key_arr(const std::string& s) {
    toml::array arr;
    for (char c : s) if (c != ' ') arr.push_back(std::string(1, c));
    return arr;
}
}

ConfigDialog::ConfigDialog(TitleBar& tb, const Config& cfg, std::function<void()> on_close)
    : m_tb(tb), m_cfg(cfg), m_on_close(std::move(on_close)),
      m_tab_names{"Colors", "Keys", "Grid"} {
    refresh_from_cfg();

    in_cursor_bg       = Input(&cb_cursor_bg,       "");
    in_cursor_fg       = Input(&cb_cursor_fg,       "");
    in_selection_bg    = Input(&cb_selection_bg,    "");
    in_selection_fg    = Input(&cb_selection_fg,    "");
    in_header          = Input(&cb_header,          "");
    in_row_number      = Input(&cb_row_number,      "");
    in_dimmed          = Input(&cb_dimmed,          "");
    in_insert_badge_bg = Input(&cb_insert_badge_bg, "");
    in_insert_badge_fg = Input(&cb_insert_badge_fg, "");
    in_normal_badge_bg = Input(&cb_normal_badge_bg, "");
    in_normal_badge_fg = Input(&cb_normal_badge_fg, "");
    in_titlebar_bg     = Input(&cb_titlebar_bg,     "");
    in_titlebar_fg     = Input(&cb_titlebar_fg,     "");
    in_formula_fg      = Input(&cb_formula_fg,      "");

    in_nav_up      = Input(&kb_nav_up,      "");
    in_nav_down    = Input(&kb_nav_down,    "");
    in_nav_left    = Input(&kb_nav_left,    "");
    in_nav_right   = Input(&kb_nav_right,   "");
    in_insert_mode = Input(&kb_insert_mode, "");
    in_delete_cell = Input(&kb_delete_cell, "");
    in_undo        = Input(&kb_undo,        "");
    in_insert_row  = Input(&kb_insert_row,  "");
    in_delete_row  = Input(&kb_delete_row,  "");
    in_insert_col  = Input(&kb_insert_col,  "");
    in_delete_col  = Input(&kb_delete_col,  "");
    in_rename_col  = Input(&kb_rename_col,  "");
    in_cmd_mode    = Input(&kb_cmd_mode,    "");

    in_cell_width  = Input(&gb_cell_width,  "");

    m_tab_toggle = Menu(&m_tab_names, &m_tab, MenuOption::Horizontal());

    auto colors_tab = Container::Vertical({
        in_cursor_bg, in_cursor_fg, in_selection_bg, in_selection_fg,
        in_header, in_row_number, in_dimmed,
        in_insert_badge_bg, in_insert_badge_fg,
        in_normal_badge_bg, in_normal_badge_fg,
        in_titlebar_bg, in_titlebar_fg, in_formula_fg,
    });
    auto keys_tab = Container::Vertical({
        in_nav_up, in_nav_down, in_nav_left, in_nav_right,
        in_insert_mode, in_delete_cell, in_undo,
        in_insert_row, in_delete_row, in_insert_col, in_delete_col,
        in_rename_col, in_cmd_mode,
    });
    auto grid_tab = Container::Vertical({ in_cell_width });

    m_content = Container::Tab({ colors_tab, keys_tab, grid_tab }, &m_tab);

    m_save_btn = Button("  Save  ", [this]{ save_to_file(); },
                        make_dialog_btn_style(m_cfg));

    m_container = Container::Vertical({ m_tab_toggle, m_content, m_save_btn });
}

void ConfigDialog::refresh_from_cfg() {
    auto cn = [](Color c) { return Config::color_to_name(c); };
    cb_cursor_bg       = cn(m_cfg.colors.cursor_bg);
    cb_cursor_fg       = cn(m_cfg.colors.cursor_fg);
    cb_selection_bg    = cn(m_cfg.colors.selection_bg);
    cb_selection_fg    = cn(m_cfg.colors.selection_fg);
    cb_header          = cn(m_cfg.colors.header);
    cb_row_number      = cn(m_cfg.colors.row_number);
    cb_dimmed          = cn(m_cfg.colors.dimmed);
    cb_insert_badge_bg = cn(m_cfg.colors.insert_badge_bg);
    cb_insert_badge_fg = cn(m_cfg.colors.insert_badge_fg);
    cb_normal_badge_bg = cn(m_cfg.colors.normal_badge_bg);
    cb_normal_badge_fg = cn(m_cfg.colors.normal_badge_fg);
    cb_titlebar_bg     = cn(m_cfg.colors.titlebar_bg);
    cb_titlebar_fg     = cn(m_cfg.colors.titlebar_fg);
    cb_formula_fg      = cn(m_cfg.colors.formula_fg);
    kb_nav_up      = kstr(m_cfg.keys.nav_up);
    kb_nav_down    = kstr(m_cfg.keys.nav_down);
    kb_nav_left    = kstr(m_cfg.keys.nav_left);
    kb_nav_right   = kstr(m_cfg.keys.nav_right);
    kb_insert_mode = kstr(m_cfg.keys.insert_mode);
    kb_delete_cell = kstr(m_cfg.keys.delete_cell);
    kb_undo        = kstr(m_cfg.keys.undo);
    kb_insert_row  = kstr(m_cfg.keys.insert_row);
    kb_delete_row  = kstr(m_cfg.keys.delete_row);
    kb_insert_col  = kstr(m_cfg.keys.insert_col);
    kb_delete_col  = kstr(m_cfg.keys.delete_col);
    kb_rename_col  = kstr(m_cfg.keys.rename_col);
    kb_cmd_mode    = kstr(m_cfg.keys.cmd_mode);
    gb_cell_width  = std::to_string(m_cfg.grid.cell_width);
    m_tab          = 0;
    m_status.clear();
}

void ConfigDialog::save_to_file() {
    toml::table tbl;
    toml::table ct;
    ct.insert("cursor_bg",       cb_cursor_bg);
    ct.insert("cursor_fg",       cb_cursor_fg);
    ct.insert("selection_bg",    cb_selection_bg);
    ct.insert("selection_fg",    cb_selection_fg);
    ct.insert("header",          cb_header);
    ct.insert("row_number",      cb_row_number);
    ct.insert("dimmed",          cb_dimmed);
    ct.insert("insert_badge_bg", cb_insert_badge_bg);
    ct.insert("insert_badge_fg", cb_insert_badge_fg);
    ct.insert("normal_badge_bg", cb_normal_badge_bg);
    ct.insert("normal_badge_fg", cb_normal_badge_fg);
    ct.insert("titlebar_bg",     cb_titlebar_bg);
    ct.insert("titlebar_fg",     cb_titlebar_fg);
    ct.insert("formula_fg",      cb_formula_fg);
    tbl.insert("colors", std::move(ct));

    toml::table kt;
    kt.insert("nav_up",      make_key_arr(kb_nav_up));
    kt.insert("nav_down",    make_key_arr(kb_nav_down));
    kt.insert("nav_left",    make_key_arr(kb_nav_left));
    kt.insert("nav_right",   make_key_arr(kb_nav_right));
    kt.insert("insert_mode", make_key_arr(kb_insert_mode));
    kt.insert("delete_cell", make_key_arr(kb_delete_cell));
    kt.insert("undo",        make_key_arr(kb_undo));
    kt.insert("insert_row",  make_key_arr(kb_insert_row));
    kt.insert("delete_row",  make_key_arr(kb_delete_row));
    kt.insert("insert_col",  make_key_arr(kb_insert_col));
    kt.insert("delete_col",  make_key_arr(kb_delete_col));
    kt.insert("rename_col",  make_key_arr(kb_rename_col));
    kt.insert("cmd_mode",    make_key_arr(kb_cmd_mode));
    tbl.insert("keys", std::move(kt));

    toml::table gt;
    try { gt.insert("cell_width", std::stoi(gb_cell_width)); } catch (...) {}
    tbl.insert("grid", std::move(gt));

    auto path = Config::config_file_path();
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    out << tbl;
    m_status = "Saved to " + path.string() + "  —  restart to apply";
}

Component ConfigDialog::component() {
    auto renderer = Renderer(m_container, [this] {
        const int lw = 20, vw = 20;
        auto crow = [&](const char* label, Component inp) -> Element {
            return hbox({
                text("  "),
                text(label) | size(WIDTH, EQUAL, lw) | color(m_cfg.colors.dimmed),
                inp->Render() | size(WIDTH, EQUAL, vw),
            });
        };
        const char* color_hint =
            "black  red  green  yellow  blue  magenta  cyan  white\n"
            "gray_dark  gray_light  *_light variants (e.g. green_light)";
        const char* keys_hint = "Space-separated chars  (e.g.  i a)";
        Element page;
        switch (m_tab) {
        default:
        case 0:
            page = vbox({
                crow("cursor_bg",       in_cursor_bg),
                crow("cursor_fg",       in_cursor_fg),
                crow("selection_bg",    in_selection_bg),
                crow("selection_fg",    in_selection_fg),
                crow("header",          in_header),
                crow("row_number",      in_row_number),
                crow("dimmed",          in_dimmed),
                crow("insert_badge_bg", in_insert_badge_bg),
                crow("insert_badge_fg", in_insert_badge_fg),
                crow("normal_badge_bg", in_normal_badge_bg),
                crow("normal_badge_fg", in_normal_badge_fg),
                crow("titlebar_bg",     in_titlebar_bg),
                crow("titlebar_fg",     in_titlebar_fg),
                crow("formula_fg",      in_formula_fg),
                text(""),
                hbox({ text("  "), paragraph(color_hint) | color(m_cfg.colors.dimmed) }),
            });
            break;
        case 1:
            page = vbox({
                crow("nav_up",      in_nav_up),
                crow("nav_down",    in_nav_down),
                crow("nav_left",    in_nav_left),
                crow("nav_right",   in_nav_right),
                crow("insert_mode", in_insert_mode),
                crow("delete_cell", in_delete_cell),
                crow("undo",        in_undo),
                crow("insert_row",  in_insert_row),
                crow("delete_row",  in_delete_row),
                crow("insert_col",  in_insert_col),
                crow("delete_col",  in_delete_col),
                crow("rename_col",  in_rename_col),
                crow("cmd_mode",    in_cmd_mode),
                text(""),
                hbox({ text("  "), text(keys_hint) | color(m_cfg.colors.dimmed) }),
            });
            break;
        case 2:
            page = vbox({ crow("cell_width", in_cell_width) });
            break;
        }
        auto inner = window(
            hbox({ text(" "), text("Configuration") | bold, text(" ") }),
            vbox({
                m_tab_toggle->Render(),
                separator(),
                page | size(WIDTH, EQUAL, lw + vw + 4),
                separator(),
                hbox({ filler(), m_save_btn->Render(), filler() }),
            })
        ) | center;
        auto bottom = flexbox({
            m_status.empty()
                ? text("")
                : hbox({ text(" "),
                         text(m_status) | color(m_cfg.colors.header),
                         text("  ") }),
            hbox({ text(" "),
                   text("Ctrl+W") | bold | color(m_cfg.colors.header),
                   text("  save  ") | color(m_cfg.colors.dimmed) }),
            hbox({ text("← →") | bold | color(m_cfg.colors.header),
                   text("  switch tab  ") | color(m_cfg.colors.dimmed) }),
            hbox({ text("Tab") | bold | color(m_cfg.colors.header),
                   text("  next field  ") | color(m_cfg.colors.dimmed) }),
            hbox({ text("Esc") | bold | color(m_cfg.colors.header),
                   text("  close  ") | color(m_cfg.colors.dimmed) }),
        }, FlexboxConfig()
            .Set(FlexboxConfig::Wrap::Wrap)
            .Set(FlexboxConfig::JustifyContent::FlexStart)
            .Set(FlexboxConfig::AlignItems::FlexStart)
            .Set(FlexboxConfig::AlignContent::FlexStart));
        return render_dialog_shell(m_tb, inner, bottom);
    });
    return CatchEvent(renderer, [this](Event e) {
        if (e == Event::Escape)             { m_on_close(); return true; }
        if (e == Event::Special("\x17"))    { save_to_file(); return true; } // Ctrl+W
        return false;
    });
}
