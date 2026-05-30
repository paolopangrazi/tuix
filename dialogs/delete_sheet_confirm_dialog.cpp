#include "delete_sheet_confirm_dialog.hpp"
#include "dialog_shell.hpp"

#include "config/config.hpp"

using namespace ftxui;

DeleteSheetConfirmDialog::DeleteSheetConfirmDialog(const Config& cfg,
                                                   std::function<std::string()> get_sheet_name,
                                                   std::function<void()> on_yes,
                                                   std::function<void()> on_close)
    : m_cfg(cfg),
      m_get_sheet_name(std::move(get_sheet_name)),
      m_on_close(std::move(on_close)) {
    auto style = make_dialog_btn_style(m_cfg);
    m_yes = Button("  Yes  ", std::move(on_yes),    style);
    m_no  = Button("  No   ", [this]{ m_on_close(); }, style);
    m_container = Container::Horizontal({ m_yes, m_no });
}

Component DeleteSheetConfirmDialog::component() {
    auto renderer = Renderer(m_container, [this] {
        auto inner = window(
            text(" Confirm ") | bold,
            vbox({
                hbox({ text("  Delete sheet "),
                       text("'" + m_get_sheet_name() + "'") | bold,
                       text("?  ") }) | center,
                text("  This cannot be undone.  ")
                    | color(m_cfg.colors.dimmed) | center,
                separatorLight(),
                hbox({ filler(), m_yes->Render(), text("    "), m_no->Render(), filler() }),
            })
        ) | size(WIDTH, LESS_THAN, 56) | center;
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
