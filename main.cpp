#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "titlebar.hpp"
#include "body.hpp"
#include "session.hpp"
#include "status_area.hpp"
#include "suggestion_bar.hpp"
#include "file_bar.hpp"
#include "tab_bar.hpp"
#include "cmd_mode.hpp"
#include "config/config.hpp"

#include "confirm_dialog.hpp"
#include "path_input_dialog.hpp"
#include "help_dialog.hpp"
#include "config_dialog.hpp"
#include "sheet_actions_dialog.hpp"

#include <filesystem>

// Tab indices into the root Container::Tab below — order must match it.
enum Tab { Main, ExitConfirm, Help, SaveConfirm, SaveAs, ConfigEditor, Open, RenameSheet,
           SheetActions, DeleteSheetConfirm };

int main(int argc, char* argv[]) {
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();
    int  tab    = Main;

    const Config cfg = Config::load();
    Body    body(50, 26, cfg);
    Session session(body);

    body.grid().set_calc_ready_cb([&] { screen.PostEvent(ftxui::Event::Custom); });

    if (argc > 1) session.load(argv[1]);
    session.init_empty_if_needed();

    auto body_comp = body.make_component();
    auto go_main   = [&] { tab = 0; body_comp->TakeFocus(); };

    // ── Dialogs (tabs 1..6) ──────────────────────────────────────────────────
    ConfirmDialog exit_dialog(cfg, 44,
        [] { return text("  Are you sure you want to exit?  ") | bold | center; },
        [&] { screen.ExitLoopClosure()(); },
        [&] { go_main(); });

    ConfirmDialog save_confirm(cfg, 60,
        [&] {
            std::string fname = std::filesystem::path(session.current_path()).filename().string();
            return hbox({ text("  Overwrite "), text(fname) | bold, text("?  ") }) | center;
        },
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

    PathInputDialog rename_sheet_dialog(cfg,
        "Rename Sheet", "rename", "sheet name",
        [&] { return std::string{}; },
        [&](const std::string& name) { session.rename_active(name); go_main(); },
        [&] { go_main(); });

    auto active_sheet_name = [&] {
        return session.workbook().at(session.workbook().active_index()).name;
    };

    SheetActionsDialog sheet_actions_dialog(cfg,
        active_sheet_name,
        [&] { return session.workbook().size() > 1; },
        [&] {
            rename_sheet_dialog.set_buffer(active_sheet_name());
            tab = RenameSheet;
        },
        [&] { tab = DeleteSheetConfirm; },
        [&] { go_main(); });

    ConfirmDialog delete_sheet_confirm(cfg, 56,
        [&] {
            return vbox({
                hbox({ text("  Delete sheet "),
                       text("'" + active_sheet_name() + "'") | bold,
                       text("?  ") }) | center,
                text("  This cannot be undone.  ")
                    | color(cfg.colors.dimmed) | center,
            });
        },
        [&] { session.delete_active(); go_main(); },
        [&] { go_main(); });

    TabBar tab_bar(cfg,
        [&] { return session.workbook().active_index(); },
        [&] { return session.workbook().size(); },
        [&](int i) { return session.workbook().at(i).name; },
        [&](int i) { session.switch_to(i); },
        [&] { session.add_sheet(); },
        [&](int) { tab = SheetActions; });

    // ── Titlebar actions route straight to the dialogs above ─────────────────
    TitleBar titlebar(
        [&] { body.grid().undo(); },
        [&] { return body.grid().can_undo(); },
        [&] { body.grid().redo(); },
        [&] { return body.grid().can_redo(); },
        [&] { open_dialog.clear_buffer(); tab = Open; },
        [&] { if (session.has_path()) tab = SaveConfirm; },
        [&] {
            save_as_dialog.set_buffer(session.has_path()
                ? std::filesystem::path(session.current_path()).filename().string()
                : "untitled.csv");
            tab = SaveAs;
        },
        [&] { tab = ExitConfirm; },
        cfg
    );

    // ── `:` command line ─────────────────────────────────────────────────────
    CmdMode cmd_mode({
        /* quit      */ [&] { tab = ExitConfirm; },
        /* save      */ [&] { if (session.has_path()) tab = SaveConfirm; },
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
        Container::Vertical({ body_comp, titlebar.component(), tab_bar.component() }),
        [&] {
            Elements rows;
            rows.push_back(body_comp->Render() | flex);
            if (session.is_xlsx_workflow()) {
                rows.push_back(separatorLight());
                rows.push_back(tab_bar.render());
            }
            auto sugg = body.grid().range_suggestions();
            if (sugg.empty()) sugg = body.grid().cell_suggestions();
            if (!sugg.empty()) {
                rows.push_back(separatorLight());
                rows.push_back(render_suggestion_bar(cfg, sugg));
            }
            if (!session.file_info().empty()) {
                rows.push_back(separatorLight());
                rows.push_back(render_file_bar(cfg, session.file_info()));
            }
            rows.push_back(separatorLight());
            rows.push_back(render_status_area(cfg, cmd_mode.is_active(), cmd_mode.buffer(),
                                              body.grid().mode(), body.grid().context_hint()));
            return vbox(std::move(rows));
        });

    auto inner_tabs = Container::Tab({
        main_inner,
        exit_dialog.component(),
        help_dialog.component(),
        save_confirm.component(),
        save_as_dialog.component(),
        cfg_dialog.component(),
        open_dialog.component(),
        rename_sheet_dialog.component(),
        sheet_actions_dialog.component(),
        delete_sheet_confirm.component(),
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
            if (tab == Help) go_main();
            else { help_dialog.reset_tab(); tab = Help; }
            return true;
        }
        if (e == Event::F12) {
            if (tab == ConfigEditor) go_main();
            else { cfg_dialog.refresh_from_cfg(); tab = ConfigEditor; }
            return true;
        }
        if (e == Event::Special("\x05")) {           // Ctrl+E → toggle exit confirm
            tab = (tab == Main) ? ExitConfirm : Main;
            return true;
        }
        if (tab == Main && session.is_xlsx_workflow()
                        && body.grid().mode() == Grid::Mode::NORMAL) {
            // Ctrl+PageDown / Ctrl+PageUp cycle sheets; Ctrl+T adds a new sheet.
            if (e == Event::Special("\x1B[6;5~")) {
                int n = session.workbook().size();
                if (n > 1) session.switch_to((session.workbook().active_index() + 1) % n);
                return true;
            }
            if (e == Event::Special("\x1B[5;5~")) {
                int n = session.workbook().size();
                if (n > 1) session.switch_to((session.workbook().active_index() - 1 + n) % n);
                return true;
            }
            if (e == Event::Special("\x14")) {        // Ctrl+T
                session.add_sheet();
                return true;
            }
        }
        if (cmd_mode.handle(e)) return true;
        if (tab == Main && body.grid().mode() == Grid::Mode::NORMAL
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
