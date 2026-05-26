#include "titlebar.hpp"

using namespace ftxui;

TitleBar::TitleBar(
    std::function<void()> on_undo,
    std::function<bool()> can_undo,
    std::function<void()> on_redo,
    std::function<bool()> can_redo,
    std::function<void()> on_open,
    std::function<void()> on_save,
    std::function<void()> on_save_as,
    std::function<void()> on_exit,
    const Config& cfg
)
    : m_cfg(cfg)
    , m_can_undo(std::move(can_undo))
    , m_can_redo(std::move(can_redo))
{
    auto plain_style = ButtonOption();
    plain_style.transform = [cfg](const EntryState& s) {
        auto e = text(s.label);
        return s.focused ? e | bold | color(cfg.colors.cursor_bg) : e | color(cfg.colors.dimmed);
    };

    auto hist_style = [&](std::function<bool()> enabled) {
        auto opt = ButtonOption();
        opt.transform = [enabled, cfg](const EntryState& s) {
            auto e = text(s.label);
            if (!enabled()) return e | color(cfg.colors.dimmed) | dim;
            return s.focused ? e | bold | color(cfg.colors.cursor_bg) : e | color(cfg.colors.dimmed);
        };
        return opt;
    };

    m_btn_undo    = Button("[ ↩ Undo ]",  std::move(on_undo),    hist_style(m_can_undo));
    m_btn_redo    = Button("[ ↪ Redo ]",  std::move(on_redo),    hist_style(m_can_redo));
    m_btn_open    = Button("[ Open ]",    std::move(on_open),    plain_style);
    m_btn_save    = Button("[ Save ]",    std::move(on_save),    plain_style);
    m_btn_save_as = Button("[ Save As ]", std::move(on_save_as), plain_style);
    m_btn_exit    = Button("[ Exit ]",    std::move(on_exit),    plain_style);
    m_btns        = Container::Horizontal({
        m_btn_undo, m_btn_redo, m_btn_open, m_btn_save, m_btn_save_as, m_btn_exit
    });
}

Element TitleBar::render_logo() const {
    return hbox({
        text(" "),
        text("▌") | color(m_cfg.colors.titlebar_bg),
        text("TUIX") | bold | color(m_cfg.colors.titlebar_bg),
        text("▐") | color(m_cfg.colors.titlebar_bg),
        text(" = Tui Excel ") | color(m_cfg.colors.dimmed),
    });
}

Element TitleBar::render_buttons() const {
    return hbox({
        m_btn_undo->Render(),
        text("  "),
        m_btn_redo->Render(),
        text("    "),
        m_btn_open->Render(),
        text("  "),
        m_btn_save->Render(),
        text("  "),
        m_btn_save_as->Render(),
        text("    "),
        m_btn_exit->Render(),
        text(" "),
    });
}

Component TitleBar::component() { return m_btns; }
