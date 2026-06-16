#include "help_dialog.hpp"
#include "dialog_shell.hpp"

#include "config/config.hpp"

using namespace ftxui;

HelpDialog::HelpDialog(const Config& cfg, std::function<void()> on_close)
    : m_cfg(cfg), m_on_close(std::move(on_close)),
      m_tab_names{
          "Navigation", "Editing", "Selection", "Search", "Col Header",
          "Row Header", "History", "Formulas", "Sheets", "Mouse", "File", "App",
      } {
    auto krow = [&](const char* keys, const char* desc) -> Element {
        return hbox({
            text("  "),
            text(keys) | bold | color(m_cfg.colors.header) | size(WIDTH, EQUAL, 26),
            text("  "),
            text(desc) | color(m_cfg.colors.dimmed),
        });
    };

    m_tabs_menu = Menu(&m_tab_names, &m_tab, MenuOption::Horizontal());

    m_content = Container::Tab({
        Renderer([krow] { return vbox({
            krow("h/←  j/↓  k/↑  l/→",      "Move cursor"),
            krow("PgUp / PgDn",              "Scroll one page up / down"),
            krow("Home / End  /  0  /  $",   "Jump to first / last column"),
            krow("gg  /  G",                 "Jump to first / last row"),
            krow("↑  from first row",        "Enter column header"),
        }); }),
        Renderer([krow] { return vbox({
            krow("i  /  a  /  F2",           "Enter INSERT mode"),
            krow("Esc",                      "Exit INSERT → NORMAL"),
            krow("o  /  O",                  "Insert row below / above & edit"),
            krow("Enter",                    "Confirm & move down"),
            krow("Tab  /  Shift+Tab",        "Confirm & move right / left"),
            krow("Arrows  (INSERT)",         "Confirm & move in that direction"),
            krow("Del",                      "Clear cell buffer"),
            krow("Backspace  (INSERT)",      "Delete last typed character"),
            krow("Backspace / x  (NORMAL)",  "Delete cell value"),
        }); }),
        Renderer([krow] { return vbox({
            krow("Shift+Arrows  (NORMAL)",   "Start / extend selection"),
            krow("Esc  (NORMAL)",            "Clear selection"),
            krow("y",                        "Yank cell / selection"),
            krow("p",                        "Paste yanked cell(s)"),
        }); }),
        Renderer([krow] { return vbox({
            krow("/",                        "Start incremental search"),
            krow("n  /  N",                  "Jump to next / previous match"),
            krow("Enter  (search)",          "Confirm search, keep highlight"),
            krow("Esc  (search)",            "Cancel search, restore cursor"),
        }); }),
        Renderer([krow] { return vbox({
            krow("i  /  a",                  "Rename column"),
            krow("+",                        "Insert column to the right"),
            krow("-  /  x  /  Backspace",    "Delete column"),
            krow("↓  /  Enter",              "Exit header back to grid"),
        }); }),
        Renderer([krow] { return vbox({
            krow("+",                        "Insert row below"),
            krow("-  /  x  /  Backspace",    "Delete row"),
        }); }),
        Renderer([krow] { return vbox({
            krow("u",                        "Undo last change"),
            krow("Ctrl+R",                   "Redo last change"),
        }); }),
        Renderer([krow] { return vbox({
            krow("=",                        "Open formula autocomplete popup"),
            krow("↑ / ↓  (popup)",           "Navigate formula list"),
            krow("Tab / Enter  (popup)",     "Complete with selected formula"),
        }); }),
        Renderer([krow] { return vbox({
            krow("Ctrl+PgDn / Ctrl+PgUp",    "Cycle to next / previous sheet"),
            krow("Ctrl+T",                   "Add a new sheet"),
            krow("Click tab",                "Switch to that sheet"),
            krow("Click active tab",         "Rename / delete sheet"),
            krow("Click  +",                 "Add a new sheet"),
        }); }),
        Renderer([krow] { return vbox({
            krow("Click cell",               "Move cursor to that cell"),
            krow("Click column header",      "Select that column header"),
            krow("Wheel up / down",          "Scroll three rows"),
            krow("Click / drag scrollbar",   "Scroll to that position"),
            krow("Click  +  /  -",           "Insert / delete row or column"),
        }); }),
        Renderer([krow] { return vbox({
            krow("Save  (titlebar)",         "Save current file (prompts on overwrite)"),
            krow("Save As  (titlebar)",      "Choose a path; prompts on overwrite"),
            krow("Open  (titlebar)",         "Open a CSV file"),
        }); }),
        Renderer([krow] { return vbox({
            krow(":w",                       "Save current file"),
            krow(":w filename",              "Save as (relative or absolute path)"),
            krow(":e filename",              "Open a CSV file"),
            krow(":B12  (bare A1 ref)",      "Jump to that cell"),
            krow(":s/old/new/",              "Find & replace across all cells"),
            krow(":wq",                      "Save and quit"),
            krow(":q  /  :q!",               "Quit via command mode"),
            krow("Ctrl+E",                   "Toggle quit confirmation dialog"),
            krow("F1",                       "This help screen"),
            krow("F12",                      "Configuration editor"),
        }); }),
    }, &m_tab);

    m_container = Container::Vertical({ m_tabs_menu, m_content });
}

void HelpDialog::reset_tab() { m_tab = 0; }

Component HelpDialog::component() {
    auto renderer = Renderer(m_container, [this] {
        auto inner = window(
            hbox({ text(" "), text("Keybindings") | bold, text(" ") }),
            vbox({
                m_tabs_menu->Render(),
                separator(),
                m_content->Render() | size(WIDTH, EQUAL, 70) | size(HEIGHT, GREATER_THAN, 12),
            })
        ) | center;
        auto bottom = flexbox({
            hbox({ text(" "),
                   text("F1")  | bold | color(m_cfg.colors.header),
                   text(" / "),
                   text("Esc") | bold | color(m_cfg.colors.header),
                   text("  close  ") | color(m_cfg.colors.dimmed) }),
            hbox({ text("← →") | bold | color(m_cfg.colors.header),
                   text("  switch tab  ") | color(m_cfg.colors.dimmed) }),
        }, FlexboxConfig()
            .Set(FlexboxConfig::Wrap::Wrap)
            .Set(FlexboxConfig::JustifyContent::FlexStart)
            .Set(FlexboxConfig::AlignItems::FlexStart)
            .Set(FlexboxConfig::AlignContent::FlexStart));
        return render_dialog_shell(inner, bottom);
    });
    return CatchEvent(renderer, [this](Event e) {
        if (e == Event::Escape) { m_on_close(); return true; }
        return false;
    });
}
