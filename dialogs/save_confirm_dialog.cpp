#include "save_confirm_dialog.hpp"
#include "dialog_shell.hpp"

#include "titlebar.hpp"
#include "config/config.hpp"

#include <filesystem>

using namespace ftxui;

SaveConfirmDialog::SaveConfirmDialog(TitleBar& tb, const Config& cfg,
                                     std::function<std::string()> get_path,
                                     std::function<void()> on_yes,
                                     std::function<void()> on_close)
    : m_tb(tb), m_cfg(cfg),
      m_get_path(std::move(get_path)),
      m_on_close(std::move(on_close)) {
    auto style = make_dialog_btn_style(m_cfg);
    m_yes = Button("  Yes  ", std::move(on_yes),    style);
    m_no  = Button("  No   ", [this]{ m_on_close(); }, style);
    m_container = Container::Horizontal({ m_yes, m_no });
}

Component SaveConfirmDialog::component() {
    auto renderer = Renderer(m_container, [this] {
        std::string fname = std::filesystem::path(m_get_path()).filename().string();
        auto inner = window(
            text(" Confirm ") | bold,
            vbox({
                hbox({ text("  Overwrite "), text(fname) | bold, text("?  ") }) | center,
                separatorLight(),
                hbox({ filler(), m_yes->Render(), text("    "), m_no->Render(), filler() }),
            })
        ) | size(WIDTH, LESS_THAN, 60) | center;
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
