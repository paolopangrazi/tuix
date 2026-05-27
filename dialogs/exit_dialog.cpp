#include "exit_dialog.hpp"
#include "dialog_shell.hpp"

#include "titlebar.hpp"
#include "config/config.hpp"

using namespace ftxui;

ExitDialog::ExitDialog(TitleBar& tb, const Config& cfg,
                       std::function<void()> on_yes,
                       std::function<void()> on_close)
    : m_tb(tb), m_cfg(cfg), m_on_close(std::move(on_close)) {
    auto style = make_dialog_btn_style(m_cfg);
    m_yes = Button("  Yes  ", std::move(on_yes),    style);
    m_no  = Button("  No   ", [this]{ m_on_close(); }, style);
    m_container = Container::Horizontal({ m_yes, m_no });
}

Component ExitDialog::component() {
    auto renderer = Renderer(m_container, [this] {
        auto inner = window(
            text(" Confirm ") | bold,
            vbox({
                text("  Are you sure you want to exit?  ") | bold | center,
                separatorLight(),
                hbox({ filler(), m_yes->Render(), text("    "), m_no->Render(), filler() }),
            })
        ) | size(WIDTH, LESS_THAN, 44) | center;
        auto bottom = hbox({
            text("  "), text("Esc") | bold | color(m_cfg.colors.header),
            text("  cancel") | color(m_cfg.colors.dimmed), filler(),
        });
        return render_dialog_shell(m_tb, inner, bottom);
    });
    return CatchEvent(renderer, [this](Event e) {
        if (e == Event::Escape) { m_on_close(); return true; }
        return false;
    });
}
