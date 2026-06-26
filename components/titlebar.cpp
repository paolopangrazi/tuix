#include "titlebar.hpp"

#include <chrono>
#include <cmath>

#include <ftxui/component/animation.hpp>

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
    // Left-to-right accent→accent2 gradient wordmark (TrueColor; on the default
    // theme the accents are palette green/cyan, keeping the brand on-theme).
    LinearGradient logo;
    if (m_cfg.theme.animations) {
        // Opt-in: a slow "breathing" sweep — a bright accent2 stop drifts across
        // the wordmark. RequestAnimationFrame keeps frames coming only while this
        // is enabled, so the app stays idle-quiet by default.
        const float t = std::chrono::duration<float>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        const float phase = 0.10f + 0.80f * (0.5f + 0.5f * std::sin(t * 1.1f));
        logo = LinearGradient().Angle(90)
                   .Stop(m_cfg.colors.accent,  0.0f)
                   .Stop(m_cfg.colors.accent2, phase)
                   .Stop(m_cfg.colors.accent,  1.0f);
        animation::RequestAnimationFrame();
    } else {
        logo = LinearGradient(90, m_cfg.colors.accent, m_cfg.colors.accent2);
    }
    return hbox({
        text(" "),
        text("▌tuiX▐") | bold | color(logo),
        text(" = tui eXcel-lent spreadsheet editor ") | color(m_cfg.colors.dimmed),
    });
}

Element TitleBar::render_buttons() const {
    return flexbox({
        hbox({ m_btn_undo->Render(),    text("  ") }),
        hbox({ m_btn_redo->Render(),    text("    ") }),
        hbox({ m_btn_open->Render(),    text("  ") }),
        hbox({ m_btn_save->Render(),    text("  ") }),
        hbox({ m_btn_save_as->Render(), text("    ") }),
        m_btn_exit->Render(),
    }, FlexboxConfig()
        .Set(FlexboxConfig::Wrap::Wrap)
        .Set(FlexboxConfig::JustifyContent::FlexStart)
        .Set(FlexboxConfig::AlignItems::FlexStart)
        .Set(FlexboxConfig::AlignContent::FlexStart));
}

Component TitleBar::component() { return m_btns; }
