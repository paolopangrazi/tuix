#include "sheet_actions_dialog.hpp"
#include "dialog_shell.hpp"

#include "config/config.hpp"

using namespace ftxui;

SheetActionsDialog::SheetActionsDialog(const Config& cfg,
                                       std::function<std::string()> get_sheet_name,
                                       std::function<bool()>        can_delete,
                                       std::function<void()>        on_rename,
                                       std::function<void()>        on_delete,
                                       std::function<void()>        on_close)
    : m_cfg(cfg),
      m_get_sheet_name(std::move(get_sheet_name)),
      m_can_delete(std::move(can_delete)),
      m_on_close(std::move(on_close)) {
    auto style = make_dialog_btn_style(m_cfg);
    m_rename = Button(" Rename ", std::move(on_rename), style);
    m_delete = Button(" Delete ", std::move(on_delete), style);
    m_cancel = Button(" Cancel ", [this]{ m_on_close(); }, style);
    m_container = Container::Horizontal({ m_rename, m_delete, m_cancel });
}

Component SheetActionsDialog::component() {
    auto renderer = Renderer(m_container, [this] {
        const bool del_ok = m_can_delete();
        Elements buttons;
        buttons.push_back(filler());
        buttons.push_back(m_rename->Render());
        buttons.push_back(text("  "));
        if (del_ok) {
            buttons.push_back(m_delete->Render());
            buttons.push_back(text("  "));
        }
        buttons.push_back(m_cancel->Render());
        buttons.push_back(filler());

        auto title = hbox({
            text("  Sheet "),
            text("'" + m_get_sheet_name() + "'") | bold,
            text("  "),
        }) | center;

        Elements body_rows;
        body_rows.push_back(title);
        if (!del_ok) {
            body_rows.push_back(text("  (last sheet — cannot delete)  ")
                                | color(m_cfg.colors.dimmed) | center);
        }
        body_rows.push_back(separatorLight());
        body_rows.push_back(hbox(std::move(buttons)));

        auto inner = window(
            text(" Sheet actions ") | bold,
            vbox(std::move(body_rows))
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
