#include "dialog_shell.hpp"

#include "titlebar.hpp"
#include "config/config.hpp"

using namespace ftxui;

Element render_dialog_shell(const TitleBar& tb, Element inner, Element bottom) {
    return window(
        tb.render_logo(),
        vbox({
            tb.render_buttons(),
            separatorLight(),
            filler(),
            std::move(inner),
            filler(),
            separatorLight(),
            std::move(bottom),
        })
    );
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
