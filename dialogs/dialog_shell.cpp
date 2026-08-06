#include "dialog_shell.hpp"

#include "config/config.hpp"

using namespace ftxui;

Element render_dialog_shell(Element inner, Element bottom) {
    return vbox({
        filler(),
        std::move(inner),
        filler(),
        separatorLight(),
        std::move(bottom),
    });
}

ButtonOption make_dialog_btn_style(const Config& cfg) {
    auto opt = ButtonOption();
    opt.transform = [cfg](const EntryState& s) {
        auto e = text(s.label);
        return s.focused
            ? e | bgcolor(cfg.colors.cursor_bg) | color(cfg.colors.cursor_fg)
            : e;
    };
    return opt;
}

Component add_escape_to_close(Component renderer, std::function<void()> on_close,
                               std::function<bool(Event)> extra) {
    return CatchEvent(renderer, [on_close = std::move(on_close), extra = std::move(extra)](Event e) {
        if (e == Event::Escape) { on_close(); return true; }
        return extra ? extra(e) : false;
    });
}

Elements escape_cancel_hint(const Config& cfg) {
    return { text("Esc") | bold | color(cfg.colors.header),
             text("  cancel") | color(cfg.colors.dimmed) };
}

Element escape_cancel_bar(const Config& cfg) {
    Elements row = { text("  ") };
    auto hint = escape_cancel_hint(cfg);
    row.insert(row.end(), hint.begin(), hint.end());
    row.push_back(filler());
    return hbox(std::move(row));
}
