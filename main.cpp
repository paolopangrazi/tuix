#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "titlebar.hpp"
#include "body.hpp"
#include "statusbar.hpp"
#include "loader/csv_loader.hpp"
#include "writer/csv_writer.hpp"
#include "config/config.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <toml++/toml.hpp>

static std::string format_size(uintmax_t bytes) {
    if (bytes < 1024)        return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
    return std::to_string(bytes / (1024 * 1024)) + " MB";
}

static std::string delimiter_name(char d) {
    switch (d) {
        case ',':  return "comma";
        case ';':  return "semicolon";
        case '\t': return "tab";
        case '|':  return "pipe";
        default:   return std::string(1, d);
    }
}

static std::string file_info(const std::string& path, const CsvData& data, const Body& body) {
    std::string name = std::filesystem::path(path).filename().string();
    std::string size;
    try { size = format_size(std::filesystem::file_size(path)); } catch (...) { size = "?"; }
    return name
        + "  |  " + size
        + "  |  " + std::to_string(body.grid().cols()) + " cols"
        + "  |  " + std::to_string(body.grid().rows()) + " rows"
        + "  |  " + delimiter_name(data.delimiter);
}

int main(int argc, char* argv[]) {
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();
    int  tab    = 0;

    const Config cfg = Config::load();
    Body body(50, 26, cfg);

    std::string file_info_msg;
    std::string current_path;       // empty if no file is associated
    char        current_delim = ',';
    StatusBar   confirm_status("Select Yes to quit, No to return");

    if (argc > 1) {
        try {
            auto data = CsvLoader::load(argv[1]);
            body.grid().load(data);
            file_info_msg = file_info(argv[1], data, body);
            current_path  = argv[1];
            current_delim = data.delimiter;
        } catch (const std::exception&) {}
    }

    std::function<void()> on_open;
    std::function<void()> on_save;
    std::function<void()> on_save_as;

    TitleBar titlebar(
        [&] { body.grid().undo(); },
        [&] { return body.grid().can_undo(); },
        [&] { body.grid().redo(); },
        [&] { return body.grid().can_redo(); },
        [&] { on_open(); },
        [&] { on_save(); },
        [&] { on_save_as(); },
        [&] { tab = 1; },
        cfg
    );

    auto load_path = [&](const std::string& path) {
        try {
            auto data = CsvLoader::load(path);
            body.grid().load(data);
            file_info_msg = file_info(path, data, body);
            current_path  = path;
            current_delim = data.delimiter;
        } catch (const std::exception&) {}
    };

    on_open = [&] {};

    auto do_write = [&](const std::string& path) {
        try {
            auto data = body.grid().to_csv_data(current_delim);
            CsvWriter::save(path, data);
            current_path  = path;
            file_info_msg = file_info(path, data, body);
        } catch (const std::exception&) {}
    };

    auto body_comp = body.make_component();
    auto go_main   = [&] { tab = 0; body_comp->TakeFocus(); };

    std::string save_as_buf;

    InputOption save_as_opt;
    save_as_opt.on_enter = [&] {
        if (!save_as_buf.empty()) {
            std::filesystem::path dir = current_path.empty()
                ? std::filesystem::current_path()
                : std::filesystem::path(current_path).parent_path();
            do_write((dir / save_as_buf).string());
        }
        go_main();
    };
    auto save_as_input = Input(&save_as_buf, "filename.csv", save_as_opt);

    on_save_as = [&] {
        save_as_buf = current_path.empty()
            ? "untitled.csv"
            : std::filesystem::path(current_path).filename().string();
        tab = 4;
    };

    on_save = [&] {
        if (current_path.empty()) return;
        tab = 3;
    };

    auto btn_style = ButtonOption();
    btn_style.transform = [cfg](const EntryState& s) {
        auto e = text(s.label);
        return s.focused ? e | bgcolor(cfg.colors.cursor_bg) | color(cfg.colors.cursor_fg) : e;
    };

    auto btn_yes = Button("  Yes  ", [&] { screen.ExitLoopClosure()(); }, btn_style);
    auto btn_no  = Button("  No   ", [&] { go_main(); },                    btn_style);

    auto btn_save_yes = Button("  Yes  ", [&] { do_write(current_path); go_main(); }, btn_style);
    auto btn_save_no  = Button("  No   ", [&] { go_main(); },                         btn_style);

    bool        cmd_mode = false;
    std::string cmd_buf;

    auto main_ui = Renderer(
        Container::Vertical({ body_comp, titlebar.component() }),
        [&] {
            auto status = [&]() -> Element {
                if (cmd_mode)
                    return hbox({ text(" "), text(cmd_buf) | bold, text("_") | bold, filler() });
                auto m = body.grid().mode();
                bool insert = (m == Grid::Mode::INSERT);
                auto mode_color = insert ? cfg.colors.insert_badge_bg : cfg.colors.normal_badge_bg;
                const std::string hint = body.grid().context_hint();
                auto flex_left = FlexboxConfig()
                    .Set(FlexboxConfig::Wrap::Wrap)
                    .Set(FlexboxConfig::JustifyContent::FlexStart)
                    .Set(FlexboxConfig::AlignItems::FlexStart)
                    .Set(FlexboxConfig::AlignContent::FlexStart);
                Elements lines;
                if (!file_info_msg.empty()) {
                    lines.push_back(hbox({
                        text(" FILE  ") | bold | color(cfg.colors.dimmed),
                        paragraph(file_info_msg) | color(cfg.colors.dimmed),
                    }));
                }
                auto switch_key = insert
                    ? hbox({ text("  "), text("Esc") | bold | color(cfg.colors.normal_badge_bg),
                             text("  →  NORMAL") | color(cfg.colors.dimmed) })
                    : hbox({ text("  "), text("i") | bold | color(cfg.colors.insert_badge_bg),
                             text(" / "),
                             text("a") | bold | color(cfg.colors.insert_badge_bg),
                             text("  →  INSERT") | color(cfg.colors.dimmed) });
                lines.push_back(flexbox({
                    hbox({ text(" "), text(insert ? "INSERT" : "NORMAL") | bold | color(mode_color) }),
                    std::move(switch_key),
                }, flex_left));
                if (!hint.empty()) {
                    lines.push_back(hbox({
                        text(" "), paragraph(hint) | color(cfg.colors.dimmed),
                    }));
                }
                lines.push_back(flexbox({
                    hbox({ text(" F1") | bold | color(cfg.colors.header),
                           text(": Help    ") | color(cfg.colors.dimmed) }),
                    hbox({ text("F2") | bold | color(cfg.colors.header),
                           text(": Config") | color(cfg.colors.dimmed) }),
                }, flex_left));
                return vbox(std::move(lines));
            }();

            return window(
                titlebar.render_logo(),
                vbox({
                    titlebar.render_buttons(),
                    separatorLight(),
                    body_comp->Render() | flex,
                    separatorLight(),
                    status,
                })
            );
        }
    );

    auto confirm_dialog = Renderer(
        Container::Horizontal({ btn_yes, btn_no }),
        [&] {
            return window(
                titlebar.render_logo(),
                vbox({
                    titlebar.render_buttons(),
                    separatorLight(),
                    filler(),
                    window(
                        text(" Confirm ") | bold,
                        vbox({
                            text("  Are you sure you want to exit?  ") | bold | center,
                            separatorLight(),
                            hbox({ filler(), btn_yes->Render(), text("    "), btn_no->Render(), filler() }),
                        })
                    ) | size(WIDTH, LESS_THAN, 44) | center,
                    filler(),
                    separatorLight(),
                    confirm_status.render(),
                })
            );
        }
    );

    // ── Save confirmation dialog ──────────────────────────────────────────────
    auto save_confirm_dialog = Renderer(
        Container::Horizontal({ btn_save_yes, btn_save_no }),
        [&] {
            std::string fname = std::filesystem::path(current_path).filename().string();
            return window(
                titlebar.render_logo(),
                vbox({
                    titlebar.render_buttons(),
                    separatorLight(),
                    filler(),
                    window(
                        text(" Confirm ") | bold,
                        vbox({
                            hbox({ text("  Overwrite "), text(fname) | bold, text("?  ") }) | center,
                            separatorLight(),
                            hbox({ filler(), btn_save_yes->Render(), text("    "), btn_save_no->Render(), filler() }),
                        })
                    ) | size(WIDTH, LESS_THAN, 60) | center,
                    filler(),
                    separatorLight(),
                    hbox({ text("  "), text("Esc") | bold | color(cfg.colors.header),
                           text("  cancel") | color(cfg.colors.dimmed), filler() }),
                })
            );
        }
    );

    // ── Save As dialog ───────────────────────────────────────────────────────
    auto save_as_dialog = CatchEvent(
        Renderer(save_as_input, [&] {
            std::string dir = current_path.empty()
                ? std::filesystem::current_path().string()
                : std::filesystem::path(current_path).parent_path().string();
            return window(
                titlebar.render_logo(),
                vbox({
                    titlebar.render_buttons(),
                    separatorLight(),
                    filler(),
                    window(
                        text(" Save As ") | bold,
                        vbox({
                            hbox({ text("  "), save_as_input->Render() | flex }),
                            separatorLight(),
                            hbox({ text("  "),
                                   text("Enter") | bold | color(cfg.colors.header),
                                   text("  save    "),
                                   text("Esc") | bold | color(cfg.colors.header),
                                   text("  cancel") | color(cfg.colors.dimmed),
                                   filler() }),
                        })
                    ) | size(WIDTH, LESS_THAN, 60) | center,
                    filler(),
                    separatorLight(),
                    hbox({ text("  dir: ") | color(cfg.colors.dimmed),
                           text(dir) | color(cfg.colors.dimmed), filler() }),
                })
            );
        }),
        [&](Event e) {
            if (e == Event::Escape) { go_main(); return true; }
            return false;
        }
    );

    // ── Help dialog ───────────────────────────────────────────────────────────
    int help_tab = 0;

    auto krow = [&](const char* keys, const char* desc) -> Element {
        return hbox({
            text("  "),
            text(keys) | bold | color(cfg.colors.header) | size(WIDTH, EQUAL, 26),
            text("  "),
            text(desc) | color(cfg.colors.dimmed),
        });
    };

    std::vector<std::string> help_tab_names = {
        "Navigation", "Editing", "Selection", "Col Header",
        "Row Index", "History", "Formulas", "File", "App",
    };

    auto help_tabs = Menu(&help_tab_names, &help_tab, MenuOption::Horizontal());

    auto help_content = Container::Tab({
        Renderer([&] { return vbox({
            krow("h/←  j/↓  k/↑  l/→",    "Move cursor"),
            krow("PgUp / PgDn",             "Scroll one page up / down"),
            krow("Home / End  /  0  /  $",  "Jump to first / last column"),
            krow("gg  /  G",                "Jump to first / last row"),
            krow("↑  from first row",       "Enter column header"),
        }); }),
        Renderer([&] { return vbox({
            krow("i  /  a",                 "Enter INSERT mode"),
            krow("Esc",                     "Exit INSERT → NORMAL"),
            krow("o  /  O",                 "Insert row below / above & edit"),
            krow("Enter",                   "Confirm & move down"),
            krow("Tab  /  Shift+Tab",       "Confirm & move right / left"),
            krow("Arrows  (INSERT)",        "Confirm & move in that direction"),
            krow("Del",                     "Clear cell buffer"),
            krow("Backspace  (INSERT)",     "Delete last typed character"),
            krow("Backspace / x  (NORMAL)", "Delete cell value"),
        }); }),
        Renderer([&] { return vbox({
            krow("Shift+Arrows  (NORMAL)",  "Start / extend selection"),
            krow("Esc  (NORMAL)",           "Clear selection"),
        }); }),
        Renderer([&] { return vbox({
            krow("i  /  a",                "Rename column"),
            krow("+",                      "Insert column to the right"),
            krow("-  /  x  /  Backspace",  "Delete column"),
            krow("↓  /  Enter",            "Exit header back to grid"),
        }); }),
        Renderer([&] { return vbox({
            krow("+",                      "Insert row below"),
            krow("-  /  x",               "Delete row"),
        }); }),
        Renderer([&] { return vbox({
            krow("u",                      "Undo last change"),
            krow("Ctrl+R",                "Redo last change"),
        }); }),
        Renderer([&] { return vbox({
            krow("=",                      "Open formula autocomplete popup"),
            krow("↑ / ↓  (popup)",         "Navigate formula list"),
            krow("Tab / Enter  (popup)",   "Complete with selected formula"),
        }); }),
        Renderer([&] { return vbox({
            krow("Ctrl+S",                "Save (warns before overwriting)"),
            krow("Save As  (titlebar)",    "Choose a path; prompts on overwrite"),
            krow("Open  (titlebar)",       "Open a CSV file"),
        }); }),
        Renderer([&] { return vbox({
            krow(":w",                    "Save current file"),
            krow(":w filename",           "Save as (relative or absolute path)"),
            krow(":wq",                   "Save and quit"),
            krow(":q  /  :q!",            "Quit via command mode"),
            krow("Ctrl+E",                "Toggle quit confirmation dialog"),
            krow("F1",                    "This help screen"),
        krow("F2",                    "Configuration editor"),
        }); }),
    }, &help_tab);

    auto help_inner = Container::Vertical({ help_tabs, help_content });

    auto help_dialog = Renderer(help_inner, [&] {
        return window(
            titlebar.render_logo(),
            vbox({
                hbox({ titlebar.render_buttons(), filler() }),
                separatorLight(),
                filler(),
                window(
                    hbox({ text(" "), text("Keybindings") | bold, text(" ") }),
                    vbox({
                        help_tabs->Render(),
                        separator(),
                        help_content->Render() | size(WIDTH, EQUAL, 70) | size(HEIGHT, GREATER_THAN, 12),
                    })
                ) | center,
                filler(),
                separatorLight(),
                flexbox({
                    hbox({ text(" "),
                           text("F1") | bold | color(cfg.colors.header),
                           text(" / "),
                           text("Esc") | bold | color(cfg.colors.header),
                           text("  close  ") | color(cfg.colors.dimmed) }),
                    hbox({ text("← →") | bold | color(cfg.colors.header),
                           text("  switch tab  ") | color(cfg.colors.dimmed) }),
                }, FlexboxConfig()
                    .Set(FlexboxConfig::Wrap::Wrap)
                    .Set(FlexboxConfig::JustifyContent::FlexStart)
                    .Set(FlexboxConfig::AlignItems::FlexStart)
                    .Set(FlexboxConfig::AlignContent::FlexStart)),
            })
        );
    });

    // ── Config dialog (F2) ───────────────────────────────────────────────────
    // String buffers — colors
    std::string cb_cursor_bg, cb_cursor_fg, cb_selection_bg, cb_selection_fg,
                cb_header, cb_row_number, cb_dimmed,
                cb_insert_badge_bg, cb_insert_badge_fg,
                cb_normal_badge_bg, cb_normal_badge_fg,
                cb_titlebar_bg, cb_titlebar_fg, cb_formula_fg;
    // String buffers — keys (space-separated chars)
    std::string kb_nav_up, kb_nav_down, kb_nav_left, kb_nav_right,
                kb_insert_mode, kb_delete_cell, kb_undo,
                kb_insert_row, kb_delete_row, kb_insert_col, kb_delete_col,
                kb_rename_col, kb_cmd_mode;
    // String buffer — grid
    std::string gb_cell_width;
    std::string cfg_status;

    auto kstr = [](const std::vector<char>& v) -> std::string {
        std::string r; for (char c : v) { if (!r.empty()) r += ' '; r += c; } return r;
    };
    auto init_cfg_bufs = [&] {
        auto cn = [](ftxui::Color c) { return Config::color_to_name(c); };
        cb_cursor_bg       = cn(cfg.colors.cursor_bg);
        cb_cursor_fg       = cn(cfg.colors.cursor_fg);
        cb_selection_bg    = cn(cfg.colors.selection_bg);
        cb_selection_fg    = cn(cfg.colors.selection_fg);
        cb_header          = cn(cfg.colors.header);
        cb_row_number      = cn(cfg.colors.row_number);
        cb_dimmed          = cn(cfg.colors.dimmed);
        cb_insert_badge_bg = cn(cfg.colors.insert_badge_bg);
        cb_insert_badge_fg = cn(cfg.colors.insert_badge_fg);
        cb_normal_badge_bg = cn(cfg.colors.normal_badge_bg);
        cb_normal_badge_fg = cn(cfg.colors.normal_badge_fg);
        cb_titlebar_bg     = cn(cfg.colors.titlebar_bg);
        cb_titlebar_fg     = cn(cfg.colors.titlebar_fg);
        cb_formula_fg      = cn(cfg.colors.formula_fg);
        kb_nav_up      = kstr(cfg.keys.nav_up);
        kb_nav_down    = kstr(cfg.keys.nav_down);
        kb_nav_left    = kstr(cfg.keys.nav_left);
        kb_nav_right   = kstr(cfg.keys.nav_right);
        kb_insert_mode = kstr(cfg.keys.insert_mode);
        kb_delete_cell = kstr(cfg.keys.delete_cell);
        kb_undo        = kstr(cfg.keys.undo);
        kb_insert_row  = kstr(cfg.keys.insert_row);
        kb_delete_row  = kstr(cfg.keys.delete_row);
        kb_insert_col  = kstr(cfg.keys.insert_col);
        kb_delete_col  = kstr(cfg.keys.delete_col);
        kb_rename_col  = kstr(cfg.keys.rename_col);
        kb_cmd_mode    = kstr(cfg.keys.cmd_mode);
        gb_cell_width  = std::to_string(cfg.grid.cell_width);
        cfg_status = "";
    };
    init_cfg_bufs();

    auto do_save_cfg = [&] {
        auto make_key_arr = [](const std::string& s) {
            toml::array arr;
            for (char c : s) if (c != ' ') arr.push_back(std::string(1, c));
            return arr;
        };
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
        cfg_status = "Saved to " + path.string() + "  —  restart to apply";
    };

    // Input components — colors
    auto in_cursor_bg       = Input(&cb_cursor_bg,       "");
    auto in_cursor_fg       = Input(&cb_cursor_fg,       "");
    auto in_selection_bg    = Input(&cb_selection_bg,    "");
    auto in_selection_fg    = Input(&cb_selection_fg,    "");
    auto in_header          = Input(&cb_header,          "");
    auto in_row_number      = Input(&cb_row_number,      "");
    auto in_dimmed          = Input(&cb_dimmed,          "");
    auto in_insert_badge_bg = Input(&cb_insert_badge_bg, "");
    auto in_insert_badge_fg = Input(&cb_insert_badge_fg, "");
    auto in_normal_badge_bg = Input(&cb_normal_badge_bg, "");
    auto in_normal_badge_fg = Input(&cb_normal_badge_fg, "");
    auto in_titlebar_bg     = Input(&cb_titlebar_bg,     "");
    auto in_titlebar_fg     = Input(&cb_titlebar_fg,     "");
    auto in_formula_fg      = Input(&cb_formula_fg,      "");
    // Input components — keys
    auto in_nav_up          = Input(&kb_nav_up,      "");
    auto in_nav_down        = Input(&kb_nav_down,    "");
    auto in_nav_left        = Input(&kb_nav_left,    "");
    auto in_nav_right       = Input(&kb_nav_right,   "");
    auto in_insert_mode     = Input(&kb_insert_mode, "");
    auto in_delete_cell     = Input(&kb_delete_cell, "");
    auto in_undo            = Input(&kb_undo,        "");
    auto in_insert_row      = Input(&kb_insert_row,  "");
    auto in_delete_row      = Input(&kb_delete_row,  "");
    auto in_insert_col      = Input(&kb_insert_col,  "");
    auto in_delete_col      = Input(&kb_delete_col,  "");
    auto in_rename_col      = Input(&kb_rename_col,  "");
    auto in_cmd_mode        = Input(&kb_cmd_mode,    "");
    // Input component — grid
    auto in_cell_width      = Input(&gb_cell_width,  "");

    int cfg_tab = 0;
    std::vector<std::string> cfg_tab_names = {"Colors", "Keys", "Grid"};
    auto cfg_tab_toggle = Menu(&cfg_tab_names, &cfg_tab, MenuOption::Horizontal());

    auto cfg_colors_tab = Container::Vertical({
        in_cursor_bg, in_cursor_fg, in_selection_bg, in_selection_fg,
        in_header, in_row_number, in_dimmed,
        in_insert_badge_bg, in_insert_badge_fg,
        in_normal_badge_bg, in_normal_badge_fg,
        in_titlebar_bg, in_titlebar_fg, in_formula_fg,
    });
    auto cfg_keys_tab = Container::Vertical({
        in_nav_up, in_nav_down, in_nav_left, in_nav_right,
        in_insert_mode, in_delete_cell, in_undo,
        in_insert_row, in_delete_row, in_insert_col, in_delete_col,
        in_rename_col, in_cmd_mode,
    });
    auto cfg_grid_tab = Container::Vertical({ in_cell_width });

    auto cfg_content = Container::Tab({
        cfg_colors_tab, cfg_keys_tab, cfg_grid_tab,
    }, &cfg_tab);

    auto cfg_save_btn = Button("  Save  ", [&] { do_save_cfg(); }, btn_style);
    auto cfg_inner = Container::Vertical({ cfg_tab_toggle, cfg_content, cfg_save_btn });

    auto cfg_dialog = Renderer(cfg_inner, [&] {
        const int lw = 20, vw = 20;
        auto crow = [&](const char* label, Component inp) -> Element {
            return hbox({
                text("  "),
                text(label) | size(WIDTH, EQUAL, lw) | color(cfg.colors.dimmed),
                inp->Render() | size(WIDTH, EQUAL, vw),
            });
        };
        const char* color_hint =
            "black  red  green  yellow  blue  magenta  cyan  white\n"
            "gray_dark  gray_light  *_light variants (e.g. green_light)";
        const char* keys_hint = "Space-separated chars  (e.g.  i a)";
        Element page;
        switch (cfg_tab) {
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
                hbox({ text("  "), paragraph(color_hint) | color(cfg.colors.dimmed) }),
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
                hbox({ text("  "), text(keys_hint) | color(cfg.colors.dimmed) }),
            });
            break;
        case 2:
            page = vbox({
                crow("cell_width", in_cell_width),
            });
            break;
        }
        return window(
            titlebar.render_logo(),
            vbox({
                hbox({ titlebar.render_buttons(), filler() }),
                separatorLight(),
                filler(),
                window(
                    hbox({ text(" "), text("Configuration") | bold, text(" ") }),
                    vbox({
                        cfg_tab_toggle->Render(),
                        separator(),
                        page | size(WIDTH, EQUAL, lw + vw + 4),
                        separator(),
                        hbox({ filler(), cfg_save_btn->Render(), filler() }),
                    })
                ) | center,
                filler(),
                separatorLight(),
                flexbox({
                    cfg_status.empty()
                        ? text("")
                        : hbox({ text(" "),
                                 text(cfg_status) | color(cfg.colors.header),
                                 text("  ") }),
                    hbox({ text(" "),
                           text("Ctrl+W") | bold | color(cfg.colors.header),
                           text("  save  ") | color(cfg.colors.dimmed) }),
                    hbox({ text("← →") | bold | color(cfg.colors.header),
                           text("  switch tab  ") | color(cfg.colors.dimmed) }),
                    hbox({ text("Tab") | bold | color(cfg.colors.header),
                           text("  next field  ") | color(cfg.colors.dimmed) }),
                    hbox({ text("Esc") | bold | color(cfg.colors.header),
                           text("  close  ") | color(cfg.colors.dimmed) }),
                }, FlexboxConfig()
                    .Set(FlexboxConfig::Wrap::Wrap)
                    .Set(FlexboxConfig::JustifyContent::FlexStart)
                    .Set(FlexboxConfig::AlignItems::FlexStart)
                    .Set(FlexboxConfig::AlignContent::FlexStart)),
            })
        );
    });

    auto root = CatchEvent(
        Container::Tab({ main_ui, confirm_dialog, help_dialog, save_confirm_dialog, save_as_dialog, cfg_dialog }, &tab),
        [&](Event e) {
            if (e == Event::F1)                  { if (tab == 2) { go_main(); } else { help_tab = 0; tab = 2; } return true; }
            if (e == Event::F2)                  { if (tab == 5) { go_main(); } else { init_cfg_bufs(); cfg_tab = 0; tab = 5; } return true; }
            if (e == Event::Special("\x05"))     { tab = (tab == 0) ? 1 : 0; return true; }
            if (e == Event::Escape && tab == 1)  { go_main(); return true; }
            if (e == Event::Escape && tab == 2)  { go_main(); return true; }
            if (e == Event::Escape && tab == 3)  { go_main(); return true; }
            if (e == Event::Escape && tab == 5)  { go_main(); return true; }
            if ((e == Event::Special("\x13") || e == Event::Special("\x17")) && tab == 5) { do_save_cfg(); return true; }

            if (cmd_mode) {
                if (e == Event::Escape)    { cmd_mode = false; cmd_buf.clear(); return true; }
                if (e == Event::Return)    {
                    bool quit      = (cmd_buf == ":q" || cmd_buf == ":q!");
                    bool save      = (cmd_buf == ":w");
                    bool save_quit = (cmd_buf == ":wq");
                    bool save_as   = (cmd_buf.size() > 3 && cmd_buf.substr(0, 3) == ":w ");
                    std::string save_as_path = save_as ? cmd_buf.substr(3) : "";
                    cmd_mode = false; cmd_buf.clear();
                    if (quit)      tab = 1;
                    if (save)      on_save();
                    if (save_quit) { if (!current_path.empty()) do_write(current_path); screen.ExitLoopClosure()(); }
                    if (save_as && !save_as_path.empty()) {
                        std::filesystem::path p = save_as_path;
                        if (p.is_relative()) {
                            std::filesystem::path base = current_path.empty()
                                ? std::filesystem::current_path()
                                : std::filesystem::path(current_path).parent_path();
                            p = base / p;
                        }
                        do_write(p.string());
                    }
                    return true;
                }
                if (e == Event::Backspace) {
                    if (cmd_buf.size() > 1) cmd_buf.pop_back();
                    else                   { cmd_mode = false; cmd_buf.clear(); }
                    return true;
                }
                if (e.is_character()) { cmd_buf += e.character(); return true; }
                return true;
            }

            if (tab == 0 && body.grid().mode() == Grid::Mode::NORMAL
                         && cfg.key_is(e, cfg.keys.cmd_mode)) {
                cmd_mode = true;
                cmd_buf  = ":";
                return true;
            }

            return false;
        }
    );

    screen.SetCursor(Screen::Cursor{});  // reset stale BarBlinking shape left by removed Input
    screen.Loop(root);
}
