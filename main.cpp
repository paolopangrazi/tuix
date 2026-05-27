#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "titlebar.hpp"
#include "body.hpp"
#include "session.hpp"
#include "status_area.hpp"
#include "config/config.hpp"

#include "dialog_shell.hpp"
#include "exit_dialog.hpp"
#include "save_confirm_dialog.hpp"
#include "path_input_dialog.hpp"
#include "help_dialog.hpp"
#include "config_dialog.hpp"

#include <filesystem>
#include <string>

int main(int argc, char* argv[]) {
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();
    int  tab    = 0;

    const Config cfg = Config::load();
    Body    body(50, 26, cfg);
    Session session(body);

    if (argc > 1) session.load(argv[1]);

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
        [&] { return session.current_path(); },
        [&] { session.write(session.current_path()); go_main(); },
        [&] { go_main(); });

    PathInputDialog save_as_dialog(titlebar, cfg,
        "Save As", "save", "filename.csv",
        [&] { return session.dir_of_current(); },
        [&](const std::string& name) {
            session.write((std::filesystem::path(session.dir_of_current()) / name).string());
        },
        [&] { go_main(); });

    PathInputDialog open_dialog(titlebar, cfg,
        "Open", "open", "path/to/file.csv",
        [&] { return session.dir_of_current(); },
        [&](const std::string& s) { session.load(session.resolve(s)); },
        [&] { go_main(); });

    HelpDialog   help_dialog(titlebar, cfg, [&] { go_main(); });
    ConfigDialog cfg_dialog (titlebar, cfg, [&] { go_main(); });

    on_open    = [&] { open_dialog.clear_buffer(); tab = 6; };
    on_save_as = [&] {
        save_as_dialog.set_buffer(session.has_path()
            ? std::filesystem::path(session.current_path()).filename().string()
            : "untitled.csv");
        tab = 4;
    };
    on_save = [&] { if (session.has_path()) tab = 3; };

    // ── Main view ────────────────────────────────────────────────────────────
    bool        cmd_mode = false;
    std::string cmd_buf;

    auto main_ui = Renderer(
        Container::Vertical({ body_comp, titlebar.component() }),
        [&] {
            auto status = render_status_area(
                cfg, cmd_mode, cmd_buf,
                body.grid().mode(), body.grid().context_hint(), session.file_info());
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
                        if (session.has_path()) session.write(session.current_path());
                        screen.ExitLoopClosure()();
                    }
                    if (save_as && !sp.empty()) session.write(session.resolve(sp));
                    if (edit    && !ep.empty()) session.load (session.resolve(ep));
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
