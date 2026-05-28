#include "path_input_dialog.hpp"
#include "dialog_shell.hpp"

#include "config/config.hpp"

using namespace ftxui;

PathInputDialog::PathInputDialog(const Config& cfg,
                                 std::string title, std::string action_label,
                                 std::string placeholder,
                                 std::function<std::string()> get_dir,
                                 std::function<void(const std::string&)> on_submit,
                                 std::function<void()> on_close)
    : m_cfg(cfg),
      m_title(std::move(title)), m_action_label(std::move(action_label)),
      m_get_dir(std::move(get_dir)),
      m_on_submit(std::move(on_submit)),
      m_on_close(std::move(on_close)) {
    InputOption opt;
    opt.on_enter = [this] {
        if (!m_buf.empty()) m_on_submit(m_buf);
        m_on_close();
    };
    m_input = Input(&m_buf, std::move(placeholder), opt);
}

void PathInputDialog::set_buffer(std::string s) { m_buf = std::move(s); }
void PathInputDialog::clear_buffer()             { m_buf.clear(); }

Component PathInputDialog::component() {
    auto renderer = Renderer(m_input, [this] {
        auto inner = window(
            hbox({ text(" "), text(m_title) | bold, text(" ") }),
            vbox({
                hbox({ text("  "), m_input->Render() | flex }),
                separatorLight(),
                hbox({ text("  "),
                       text("Enter") | bold | color(m_cfg.colors.header),
                       text("  " + m_action_label + "    "),
                       text("Esc")   | bold | color(m_cfg.colors.header),
                       text("  cancel") | color(m_cfg.colors.dimmed),
                       filler() }),
            })
        ) | size(WIDTH, LESS_THAN, 60) | center;
        auto bottom = hbox({
            text("  dir: ") | color(m_cfg.colors.dimmed),
            text(m_get_dir()) | color(m_cfg.colors.dimmed),
            filler(),
        });
        return render_dialog_shell(inner, bottom);
    });
    return CatchEvent(renderer, [this](Event e) {
        if (e == Event::Escape) { m_on_close(); return true; }
        return false;
    });
}
