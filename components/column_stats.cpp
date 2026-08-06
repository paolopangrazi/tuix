#include "column_stats.hpp"

#include "config/config.hpp"
#include "formulas/value.hpp"
#include "util/flexbox.hpp"

using namespace ftxui;

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
        stat("sum: ", Value::display_number(stats.sum));
        stat("mean: ", Value::display_number(stats.mean));
        stat("med: ",  Value::display_number(stats.median));
        stat("min: ",  Value::display_number(stats.min));
        stat("max: ",  Value::display_number(stats.max));
    }
    stat("nulls: ", std::to_string(stats.nulls));

    return flexbox(std::move(chips), tuix::flex_wrap_left());
}
