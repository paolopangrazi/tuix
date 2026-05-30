#include "status_area.hpp"

#include "config/config.hpp"

using namespace ftxui;

Element render_status_area(const Config& cfg,
                           bool cmd_mode,
                           const std::string& cmd_buf,
                           Grid::Mode mode,
                           const std::string& hint) {
    if (cmd_mode) {
        return vbox({
            hbox({ text(" "), text(cmd_buf) | bold, text("_") | bold, filler() }),
            hbox({ text(" "),
                   text("Enter")     | bold | color(cfg.colors.header),
                   text(": run  ")   | color(cfg.colors.dimmed),
                   text("Esc")       | bold | color(cfg.colors.header),
                   text(": cancel  ") | color(cfg.colors.dimmed),
                   text("Backspace") | bold | color(cfg.colors.header),
                   text(": erase")   | color(cfg.colors.dimmed),
                   filler() }),
        });
    }

    bool insert = (mode == Grid::Mode::INSERT);
    auto mode_color = insert ? cfg.colors.insert_badge_bg : cfg.colors.normal_badge_bg;

    auto flex_left = FlexboxConfig()
        .Set(FlexboxConfig::Wrap::Wrap)
        .Set(FlexboxConfig::JustifyContent::FlexStart)
        .Set(FlexboxConfig::AlignItems::FlexStart)
        .Set(FlexboxConfig::AlignContent::FlexStart);

    Elements lines;
    if (!hint.empty()) {
        lines.push_back(hbox({
            text(" "), paragraph(hint) | color(cfg.colors.dimmed),
        }));
    }

    auto switch_key = insert
        ? hbox({ text("  "), text("Esc") | bold | color(cfg.colors.normal_badge_bg),
                 text("  →  NORMAL") | color(cfg.colors.dimmed) })
        : hbox({ text("  "), text("i") | bold | color(cfg.colors.insert_badge_bg),
                 text(" / "),
                 text("a") | bold | color(cfg.colors.insert_badge_bg),
                 text(" / "),
                 text("F2") | bold | color(cfg.colors.insert_badge_bg),
                 text("  →  INSERT") | color(cfg.colors.dimmed) });

    lines.push_back(flexbox({
        hbox({ text(" "), text(insert ? "INSERT" : "NORMAL") | bold | color(mode_color) }),
        std::move(switch_key),
        hbox({ text("    F1") | bold | color(cfg.colors.header),
               text(": Help    ") | color(cfg.colors.dimmed) }),
        hbox({ text("F12") | bold | color(cfg.colors.header),
               text(": Config") | color(cfg.colors.dimmed) }),
    }, flex_left));

    return vbox(std::move(lines));
}
