#include "column_stats.hpp"

#include "config/config.hpp"
#include "formulas/value.hpp"

using namespace ftxui;

namespace {
// Format a number the same way cells/formulas do (trims trailing zeros, etc.).
std::string num(double v) { return Value::number(v).to_display(); }
}  // namespace

Element render_column_stats(const Config& cfg, const Grid::ColumnStats& stats) {
    Elements chips;

    auto stat = [&](const std::string& label, const std::string& value) {
        chips.push_back(hbox({
            text(label)  | color(cfg.colors.dimmed),
            text(value)  | bold | color(cfg.colors.header),
            text("    "),
        }));
    };

    chips.push_back(hbox({
        text(" Σ  ") | bold | color(cfg.colors.header),
        text(stats.name) | bold | color(cfg.colors.formula_fg),
        text("    "),
    }));

    stat("non-empty: ", std::to_string(stats.nonnull));
    if (stats.numeric > 0) {
        stat("sum: ", num(stats.sum));
        stat("mean: ", num(stats.mean));
        stat("med: ",  num(stats.median));
        stat("min: ",  num(stats.min));
        stat("max: ",  num(stats.max));
    }
    stat("nulls: ", std::to_string(stats.nulls));

    return flexbox(std::move(chips), FlexboxConfig()
        .Set(FlexboxConfig::Wrap::Wrap)
        .Set(FlexboxConfig::JustifyContent::FlexStart)
        .Set(FlexboxConfig::AlignItems::FlexStart)
        .Set(FlexboxConfig::AlignContent::FlexStart));
}
