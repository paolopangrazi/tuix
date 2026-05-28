#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "titlebar.hpp"
#include "body.hpp"
#include "session.hpp"
#include "status_area.hpp"
#include "cmd_mode.hpp"
#include "config/config.hpp"

#include "exit_dialog.hpp"
#include "save_confirm_dialog.hpp"
#include "path_input_dialog.hpp"
#include "help_dialog.hpp"
#include "config_dialog.hpp"

#include <filesystem>

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

    // ── Dialogs (tabs 1..6) ──────────────────────────────────────────────────
    ExitDialog exit_dialog(cfg,
        [&] { screen.ExitLoopClosure()(); },
        [&] { go_main(); });

    SaveConfirmDialog save_confirm(cfg,
        [&] { return session.current_path(); },
        [&] { session.write(session.current_path()); go_main(); },
        [&] { go_main(); });

    PathInputDialog save_as_dialog(cfg,
        "Save As", "save", "filename.csv",
        [&] { return session.dir_of_current(); },
        [&](const std::string& name) {
            session.write((std::filesystem::path(session.dir_of_current()) / name).string());
        },
        [&] { go_main(); });

    PathInputDialog open_dialog(cfg,
        "Open", "open", "path/to/file.csv",
        [&] { return session.dir_of_current(); },
        [&](const std::string& s) { session.load(session.resolve(s)); },
        [&] { go_main(); });

    HelpDialog   help_dialog(cfg, [&] { go_main(); });
    ConfigDialog cfg_dialog (cfg, [&] { go_main(); });

    // ── Titlebar actions route straight to the dialogs above ─────────────────
    TitleBar titlebar(
        [&] { body.grid().undo(); },
        [&] { return body.grid().can_undo(); },
        [&] { body.grid().redo(); },
        [&] { return body.grid().can_redo(); },
        [&] { open_dialog.clear_buffer(); tab = 6; },
        [&] { if (session.has_path()) tab = 3; },
        [&] {
            save_as_dialog.set_buffer(session.has_path()
                ? std::filesystem::path(session.current_path()).filename().string()
                : "untitled.csv");
            tab = 4;
        },
        [&] { tab = 1; },
        cfg
    );

    // ── `:` command line ─────────────────────────────────────────────────────
    CmdMode cmd_mode({
        /* quit      */ [&] { tab = 1; },
        /* save      */ [&] { if (session.has_path()) tab = 3; },
        /* save_quit */ [&] {
            if (session.has_path()) session.write(session.current_path());
            screen.ExitLoopClosure()();
        },
        /* save_as   */ [&](const std::string& p) { session.write(session.resolve(p)); },
        /* edit      */ [&](const std::string& p) { session.load (session.resolve(p)); },
    });

    // ── Main view: grid + status (chrome is drawn at the root) ───────────────
    // The titlebar component lives in this Vertical (body_comp is child 0, so
    // the grid keeps keyboard focus); its buttons are *rendered* at the root.
    auto main_inner = Renderer(
        Container::Vertical({ body_comp, titlebar.component() }),
        [&] {
            return vbox({
                body_comp->Render() | flex,
                separatorLight(),
                render_status_area(cfg, cmd_mode.is_active(), cmd_mode.buffer(),
                                   body.grid().mode(), body.grid().context_hint(),
                                   session.file_info()),
            });
        });

    auto inner_tabs = Container::Tab({
        main_inner,
        exit_dialog.component(),
        help_dialog.component(),
        save_confirm.component(),
        save_as_dialog.component(),
        cfg_dialog.component(),
        open_dialog.component(),
    }, &tab);

    // ── Root chrome: titlebar logo + buttons wrap every tab ──────────────────
    auto root_chrome = Renderer(inner_tabs, [&] {
        return window(
            titlebar.render_logo(),
            vbox({
                titlebar.render_buttons(),
                separatorLight(),
                inner_tabs->Render() | flex,
            })
        );
    });

    auto root = CatchEvent(root_chrome, [&](Event e) {
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
        if (cmd_mode.handle(e)) return true;
        if (tab == 0 && body.grid().mode() == Grid::Mode::NORMAL
                     && cfg.key_is(e, cfg.keys.cmd_mode)) {
            cmd_mode.enter();
            return true;
        }
        return false;
    });

    screen.SetCursor(Screen::Cursor{});
    body_comp->TakeFocus();
    screen.Loop(root);
}
