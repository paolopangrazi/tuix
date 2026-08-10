#include "tips_dialog.hpp"
#include "dialog_shell.hpp"
#include "tips_catalog.hpp"

#include "config/config.hpp"
#include "util/flexbox.hpp"

#include <chrono>
#include <random>

using namespace ftxui;

namespace {
// A different tip on every launch: the catalog is short enough that a plain
// uniform draw beats persisting a rotation counter in the config file.
int random_tip_index() {
    const int n = (int)tuix::tips().size();
    if (n <= 1) return 0;
    static std::mt19937 rng{
        (std::mt19937::result_type)std::chrono::steady_clock::now().time_since_epoch().count()};
    return std::uniform_int_distribution<int>(0, n - 1)(rng);
}
}  // namespace

TipsDialog::TipsDialog(const Config& cfg, std::function<void()> on_close)
    : m_cfg(cfg), m_on_close(std::move(on_close)),
      m_index(random_tip_index()),
      m_show_at_startup(cfg.tips.show_at_startup),
      m_saved_preference(cfg.tips.show_at_startup) {
    auto style = make_dialog_btn_style(m_cfg);
    m_prev      = Button(" ◀ ", [this] { step(-1); }, style);
    m_next      = Button(" ▶ ", [this] { step(+1); }, style);
    m_close_btn = Button(" Got it ", [this] { close(); }, style);

    CheckboxOption copt;
    copt.label   = " Show these tips at startup";
    copt.checked = &m_show_at_startup;
    copt.transform = [this](const EntryState& s) {
        auto e = hbox({ text(s.state ? "[✓]" : "[ ]") | bold, text(s.label) });
        e = e | color(s.state ? m_cfg.colors.header : m_cfg.colors.dimmed);
        return s.focused
            ? e | bgcolor(m_cfg.colors.cursor_bg) | color(m_cfg.colors.cursor_fg)
            : e;
    };
    m_checkbox = Checkbox(copt);

    m_container = Container::Horizontal({ m_prev, m_next, m_checkbox, m_close_btn });
}

void TipsDialog::reset() {
    m_index = random_tip_index();
    m_status.clear();
}

void TipsDialog::step(int d) {
    const int n = (int)tuix::tips().size();
    if (n > 0) m_index = ((m_index + d) % n + n) % n;
}

void TipsDialog::close() {
    // Only touch config.toml when the user actually flipped the checkbox.
    if (m_show_at_startup != m_saved_preference) {
        if (Config::save_show_tips(m_show_at_startup)) {
            m_saved_preference = m_show_at_startup;
        } else {
            m_status = "Could not save the preference to config.toml";
            return;                       // keep the dialog open so it's seen
        }
    }
    m_on_close();
}

Component TipsDialog::component() {
    auto renderer = Renderer(m_container, [this] {
        constexpr int k_width = 62;
        const auto& all = tuix::tips();
        const tuix::Tip& tip = all[(size_t)m_index];

        Elements card = {
            text(tip.title) | bold | color(m_cfg.colors.header),
        };
        if (tip.keys && *tip.keys) {
            card.push_back(text(""));
            card.push_back(hbox({ text(" "), text(tip.keys) | bold,
                                  text(" ") }) | color(m_cfg.colors.accent));
        }
        card.push_back(text(""));
        card.push_back(paragraph(tip.body) | color(m_cfg.colors.dimmed));

        auto counter = text("Tip " + std::to_string(m_index + 1) + " / "
                            + std::to_string(all.size()))
                     | color(m_cfg.colors.dimmed);

        auto inner = window(
            hbox({ text(" "), text("Did you know?") | bold, text(" ") }),
            vbox({
                hbox({ m_prev->Render(), filler(), counter, filler(),
                       m_next->Render() }),
                separator(),
                // Fixed height so stepping through tips doesn't make the box
                // jump around under the cursor.
                vbox(std::move(card)) | size(HEIGHT, GREATER_THAN, 7),
                separator(),
                hbox({ m_checkbox->Render(), filler(), m_close_btn->Render() }),
            })
        ) | size(WIDTH, EQUAL, k_width) | center;

        Elements hints;
        if (!m_status.empty())
            hints.push_back(hbox({ text(" "), text(m_status) | color(m_cfg.colors.header),
                                   text("  ") }));
        hints.push_back(hbox({ text(" "),
                               text("← →") | bold | color(m_cfg.colors.header),
                               text("  previous / next tip  ") | color(m_cfg.colors.dimmed) }));
        hints.push_back(hbox({ text("Tab") | bold | color(m_cfg.colors.header),
                               text("  next field  ") | color(m_cfg.colors.dimmed) }));
        hints.push_back(hbox({ text("Space") | bold | color(m_cfg.colors.header),
                               text("  toggle checkbox  ") | color(m_cfg.colors.dimmed) }));
        hints.push_back(hbox({ text("Esc") | bold | color(m_cfg.colors.header),
                               text("  close  ") | color(m_cfg.colors.dimmed) }));

        return render_dialog_shell(inner, flexbox(std::move(hints), tuix::flex_wrap_left()));
    });

    // Arrows step tips rather than moving focus between the buttons (Tab does
    // that) — matching the help dialog, where ← → switch tabs. Space toggles
    // the checkbox from anywhere in the dialog: caught here it never reaches
    // the Checkbox itself, so the state flips exactly once either way.
    return add_escape_to_close(renderer, [this] { close(); }, [this](Event e) {
        if (e == Event::ArrowLeft  || e == Event::Character('h')) { step(-1); return true; }
        if (e == Event::ArrowRight || e == Event::Character('l')) { step(+1); return true; }
        if (e == Event::Character(' ')) {
            m_show_at_startup = !m_show_at_startup;
            return true;
        }
        return false;
    });
}
