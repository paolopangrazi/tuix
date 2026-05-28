#include "suggestion_bar.hpp"

#include "config/config.hpp"

using namespace ftxui;

Element render_suggestion_bar(const Config& cfg,
                             const std::vector<Grid::Suggestion>& suggestions) {
    Elements chips;
    chips.push_back(hbox({ text(" ƒ  ") | bold | color(cfg.colors.header) }));
    for (const auto& s : suggestions) {
        chips.push_back(hbox({
            text(s.name)  | bold | color(cfg.colors.header),
            text(" → ")   | color(cfg.colors.dimmed),
            text(s.value) | color(cfg.colors.formula_fg),
            text("    "),
        }));
    }
    return flexbox(std::move(chips), FlexboxConfig()
        .Set(FlexboxConfig::Wrap::Wrap)
        .Set(FlexboxConfig::JustifyContent::FlexStart)
        .Set(FlexboxConfig::AlignItems::FlexStart)
        .Set(FlexboxConfig::AlignContent::FlexStart));
}
