#pragma once
#include <functional>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

struct Config;

// Common dialog body: filler · inner · filler · separator · bottom bar.
// The outer chrome (title logo + buttons row) is added at the root, not here.
ftxui::Element render_dialog_shell(
    ftxui::Element inner,
    ftxui::Element bottom_bar
);

ftxui::ButtonOption make_dialog_btn_style(const Config& cfg);

// Wraps `renderer` so Escape always calls `on_close()`; every modal dialog
// wants this. `extra`, if given, handles any further keys (checked when the
// event isn't Escape) — e.g. config_dialog's Ctrl+W save.
ftxui::Component add_escape_to_close(
    ftxui::Component renderer,
    std::function<void()> on_close,
    std::function<bool(ftxui::Event)> extra = nullptr
);

// The "Esc  cancel" key-hint fragment shared by the simple modals (confirm,
// path input, sheet actions). Two elements: the bold key label, then the
// dimmed "  cancel" text — callers embed them in whatever bottom-bar layout
// they need.
ftxui::Elements escape_cancel_hint(const Config& cfg);

// Full convenience bottom bar for dialogs whose only hint is "Esc  cancel":
// two leading spaces, the hint, then a filler.
ftxui::Element escape_cancel_bar(const Config& cfg);
