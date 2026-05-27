#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "titlebar.hpp"
#include "body.hpp"
#include "status_area.hpp"
#include "loader/csv_loader.hpp"
#include "writer/csv_writer.hpp"
#include "config/config.hpp"

#include "dialog_shell.hpp"
#include "exit_dialog.hpp"
#include "save_confirm_dialog.hpp"
#include "path_input_dialog.hpp"
#include "help_dialog.hpp"
#include "config_dialog.hpp"

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
    std::string current_path;
    char        current_delim = ',';

    if (argc > 1) {
        try {
            auto data = CsvLoader::load(argv[1]);
            body.grid().load(data);
            file_info_msg = file_info(argv[1], data, body);
            current_path  = argv[1];
            current_delim = data.delimiter;
        } catch (const std::exception&) {}
    }

    auto load_path = [&](const std::string& path) {
        try {
            auto data = CsvLoader::load(path);
            body.grid().load(data);
            file_info_msg = file_info(path, data, body);
            current_path  = path;
            current_delim = data.delimiter;
        } catch (const std::exception&) {}
    };

    auto do_write = [&](const std::string& path) {
        try {
            auto data = body.grid().to_csv_data(current_delim);
            CsvWriter::save(path, data);
            current_path  = path;
            file_info_msg = file_info(path, data, body);
        } catch (const std::exception&) {}
    };

    auto resolve = [&](const std::string& s) -> std::string {
        std::filesystem::path p = s;
        if (p.is_relative()) {
            std::filesystem::path base = current_path.empty()
                ? std::filesystem::current_path()
                : std::filesystem::path(current_path).parent_path();
            p = base / p;
        }
        return p.string();
    };

    auto dir_of_current = [&] {
        return current_path.empty()
            ? std::filesystem::current_path().string()
            : std::filesystem::path(current_path).parent_path().string();
    };

    auto body_comp = body.make_component();
    auto go_main   = [&] { tab = 0; body_comp->TakeFocus(); };

    std::function<void()> on_open, on_save, on_save_as;
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

    // ── Dialogs ──────────────────────────────────────────────────────────────
    ExitDialog exit_dialog(titlebar, cfg,
        [&] { screen.ExitLoopClosure()(); },
        [&] { go_main(); });

    SaveConfirmDialog save_confirm(titlebar, cfg,
        [&]{ return current_path; },
        [&] { do_write(current_path); go_main(); },
        [&] { go_main(); });

    PathInputDialog save_as_dialog(titlebar, cfg,
        "Save As", "save", "filename.csv",
        dir_of_current,
        [&](const std::string& name) {
            std::filesystem::path dir = current_path.empty()
                ? std::filesystem::current_path()
                : std::filesystem::path(current_path).parent_path();
            do_write((dir / name).string());
        },
        [&] { go_main(); });

    PathInputDialog open_dialog(titlebar, cfg,
        "Open", "open", "path/to/file.csv",
        dir_of_current,
        [&](const std::string& s) { load_path(resolve(s)); },
        [&] { go_main(); });

    HelpDialog   help_dialog(titlebar, cfg, [&] { go_main(); });
    ConfigDialog cfg_dialog (titlebar, cfg, [&] { go_main(); });

    on_open    = [&] { open_dialog.clear_buffer(); tab = 6; };
    on_save_as = [&] {
        save_as_dialog.set_buffer(current_path.empty()
            ? "untitled.csv"
            : std::filesystem::path(current_path).filename().string());
        tab = 4;
    };
    on_save = [&] { if (!current_path.empty()) tab = 3; };

    // ── Main view ────────────────────────────────────────────────────────────
    bool        cmd_mode = false;
    std::string cmd_buf;

    auto main_ui = Renderer(
        Container::Vertical({ body_comp, titlebar.component() }),
        [&] {
            auto status = render_status_area(
                cfg, cmd_mode, cmd_buf,
                body.grid().mode(), body.grid().context_hint(), file_info_msg);
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

    // ── Root: route F-keys, Ctrl+E and `:` cmd entry ─────────────────────────
    auto root = CatchEvent(
        Container::Tab({
            main_ui,
            exit_dialog.component(),
            help_dialog.component(),
            save_confirm.component(),
            save_as_dialog.component(),
            cfg_dialog.component(),
            open_dialog.component(),
        }, &tab),
        [&](Event e) {
            if (e == Event::F1) {
                if (tab == 2) go_main();
                else { help_dialog.reset_tab(); tab = 2; }
                return true;
            }
            if (e == Event::F2) {
                if (tab == 5) go_main();
                else { cfg_dialog.refresh_from_cfg(); tab = 5; }
                return true;
            }
            if (e == Event::Special("\x05")) {           // Ctrl+E → toggle exit confirm
                tab = (tab == 0) ? 1 : 0;
                return true;
            }

            if (cmd_mode) {
                if (e == Event::Escape) { cmd_mode = false; cmd_buf.clear(); return true; }
                if (e == Event::Return) {
                    bool quit      = (cmd_buf == ":q" || cmd_buf == ":q!");
                    bool save      = (cmd_buf == ":w");
                    bool save_quit = (cmd_buf == ":wq");
                    bool save_as   = (cmd_buf.size() > 3 && cmd_buf.substr(0, 3) == ":w ");
                    bool edit      = (cmd_buf.size() > 3 && cmd_buf.substr(0, 3) == ":e ");
                    std::string sp = save_as ? cmd_buf.substr(3) : "";
                    std::string ep = edit    ? cmd_buf.substr(3) : "";
                    cmd_mode = false; cmd_buf.clear();
                    if (quit)                          tab = 1;
                    if (save)                          on_save();
                    if (save_quit) {
                        if (!current_path.empty()) do_write(current_path);
                        screen.ExitLoopClosure()();
                    }
                    if (save_as && !sp.empty()) do_write (resolve(sp));
                    if (edit    && !ep.empty()) load_path(resolve(ep));
                    return true;
                }
                if (e == Event::Backspace) {
                    if (cmd_buf.size() > 1) cmd_buf.pop_back();
                    else                    { cmd_mode = false; cmd_buf.clear(); }
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

    screen.SetCursor(Screen::Cursor{});
    screen.Loop(root);
}
