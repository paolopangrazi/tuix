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
#include <string>

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
                Elements lines;
                if (!file_info_msg.empty()) {
                    lines.push_back(hbox({
                        text("▎") | bold | color(cfg.colors.dimmed),
                        text(" FILE  ") | bold | color(cfg.colors.dimmed),
                        text(file_info_msg) | color(cfg.colors.dimmed),
                        filler(),
                    }));
                }
                auto switch_key = insert
                    ? hbox({ text("  "), text("Esc") | bold | color(cfg.colors.normal_badge_bg),
                             text("  →  NORMAL") | color(cfg.colors.dimmed), text("    ") })
                    : hbox({ text("  "), text("i") | bold | color(cfg.colors.insert_badge_bg),
                             text(" / "),
                             text("a") | bold | color(cfg.colors.insert_badge_bg),
                             text("  →  INSERT") | color(cfg.colors.dimmed), text("    ") });
                lines.push_back(hbox({
                    text("▎") | bold | color(mode_color),
                    text(" "),
                    text(insert ? "INSERT" : "NORMAL") | bold | color(mode_color),
                    std::move(switch_key),
                    text(hint) | color(cfg.colors.dimmed),
                    filler(),
                    text("F1") | bold | color(cfg.colors.header),
                    text(": Help  ") | color(cfg.colors.dimmed),
                }));
                return vbox(std::move(lines));
            }();

            return window(
                titlebar.render_logo(),
                vbox({
                    hbox({ titlebar.render_buttons(), filler() }),
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
                    hbox({ titlebar.render_buttons(), filler() }),
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
                    hbox({ titlebar.render_buttons(), filler() }),
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
                    hbox({ titlebar.render_buttons(), filler() }),
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
                hbox({ text(" "), text("F1") | bold | color(cfg.colors.header),
                       text(" / "), text("Esc") | bold | color(cfg.colors.header),
                       text("  close this window  ") | color(cfg.colors.dimmed),
                       text("← →") | bold | color(cfg.colors.header),
                       text("  switch tab") | color(cfg.colors.dimmed), filler() }),
            })
        );
    });

    auto root = CatchEvent(
        Container::Tab({ main_ui, confirm_dialog, help_dialog, save_confirm_dialog, save_as_dialog }, &tab),
        [&](Event e) {
            if (e == Event::F1)                  { if (tab == 2) { go_main(); } else { help_tab = 0; tab = 2; } return true; }
            if (e == Event::Special("\x05"))     { tab = (tab == 0) ? 1 : 0; return true; }
            if (e == Event::Escape && tab == 1)  { go_main(); return true; }
            if (e == Event::Escape && tab == 2)  { go_main(); return true; }
            if (e == Event::Escape && tab == 3)  { go_main(); return true; }

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
