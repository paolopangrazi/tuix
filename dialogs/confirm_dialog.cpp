#include "confirm_dialog.hpp"
#include "dialog_shell.hpp"

#include "config/config.hpp"

using namespace ftxui;

ConfirmDialog::ConfirmDialog(const Config& cfg,
                             int width,
                             std::function<ftxui::Element()> body,
                             std::function<void()> on_yes,
                             std::function<void()> on_close)
    : m_cfg(cfg),
      m_width(width),
      m_body(std::move(body)),
      m_on_close(std::move(on_close)) {
    auto style = make_dialog_btn_style(m_cfg);
    m_yes = Button("  Yes  ", std::move(on_yes),      style);
    m_no  = Button("  No   ", [this]{ m_on_close(); }, style);
    m_container = Container::Horizontal({ m_yes, m_no });
}

Component ConfirmDialog::component() {
    auto renderer = Renderer(m_container, [this] {
        auto inner = window(
            text(" Confirm ") | bold,
            vbox({
                m_body(),
                separatorLight(),
                hbox({ filler(), m_yes->Render(), text("    "), m_no->Render(), filler() }),
            })
        ) | size(WIDTH, LESS_THAN, m_width) | center;
        auto bottom = hbox({
            text("  "), text("Esc") | bold | color(m_cfg.colors.header),
            text("  cancel") | color(m_cfg.colors.dimmed), filler(),
        });
        return render_dialog_shell(inner, bottom);
    });
    return CatchEvent(renderer, [this](Event e) {
        if (e == Event::Escape) { m_on_close(); return true; }
        return false;
    });
}
