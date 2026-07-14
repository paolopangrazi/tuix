#include "chart_panel.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include <ftxui/dom/canvas.hpp>

#include "config/config.hpp"
#include "formulas/value.hpp"

using namespace ftxui;

namespace charts {

std::vector<int> histogram_bins(const std::vector<double>& values, int bins) {
    if (bins < 1) bins = 1;
    std::vector<int> counts(bins, 0);
    if (values.empty()) return counts;

    const double lo = *std::min_element(values.begin(), values.end());
    const double hi = *std::max_element(values.begin(), values.end());
    if (hi <= lo) { counts[0] = static_cast<int>(values.size()); return counts; }

    for (double v : values) {
        int idx = static_cast<int>((v - lo) / (hi - lo) * bins);
        counts[std::clamp(idx, 0, bins - 1)]++;
    }
    return counts;
}

}  // namespace charts

namespace {

std::string num(double v) { return Value::number(v).to_display(); }

// Map a data value to a canvas y (0 = top) given the drawn value window.
int map_y(double v, double lo, double hi, int h) {
    if (hi <= lo) return h / 2;
    double t = (v - lo) / (hi - lo);          // 0 at lo (bottom) … 1 at hi (top)
    int y = static_cast<int>((1.0 - t) * (h - 1) + 0.5);
    return std::clamp(y, 0, h - 1);
}

// Vertical bars from a zero baseline — shared by Bar and Histogram.
void draw_bars(Canvas& cv, const std::vector<double>& vals, const Color& color) {
    const int n = static_cast<int>(vals.size());
    if (n == 0) return;
    const int W = cv.width(), H = cv.height();

    double dmin = *std::min_element(vals.begin(), vals.end());
    double dmax = *std::max_element(vals.begin(), vals.end());
    const double lo = std::min(0.0, dmin);    // always include the zero baseline
    const double hi = std::max(0.0, dmax);
    const int y_base = map_y(0.0, lo, hi, H);

    for (int i = 0; i < n; ++i) {
        const int x0 = i * W / n;
        const int x1 = std::max(x0 + 1, (i + 1) * W / n - 1);   // 1-dot gap
        const int yv = map_y(vals[i], lo, hi, H);
        const int top = std::min(yv, y_base), bot = std::max(yv, y_base);
        for (int x = x0; x < x1 && x < W; ++x)
            for (int y = top; y <= bot; ++y)
                cv.DrawPoint(x, y, true, color);
    }
}

void draw_line(Canvas& cv, const std::vector<double>& vals, const Color& color) {
    const int n = static_cast<int>(vals.size());
    const int W = cv.width(), H = cv.height();
    const double lo = *std::min_element(vals.begin(), vals.end());
    const double hi = *std::max_element(vals.begin(), vals.end());

    auto px = [&](int i) { return n > 1 ? i * (W - 1) / (n - 1) : W / 2; };
    if (n == 1) { cv.DrawPoint(W / 2, map_y(vals[0], lo, hi, H), true, color); return; }
    for (int i = 0; i + 1 < n; ++i)
        cv.DrawPointLine(px(i),     map_y(vals[i],     lo, hi, H),
                         px(i + 1), map_y(vals[i + 1], lo, hi, H), color);
}

Element header(const Config& cfg, const Grid::ChartData& d) {
    const char* type = d.type == Grid::ChartType::Bar       ? "▊ Bar"
                     : d.type == Grid::ChartType::Line       ? "╱ Line"
                     :                                         "▟ Histogram";
    Elements chips = {
        text(std::string(" ") + type + " ") | bold | color(cfg.colors.header),
        text(d.label + "  ") | bold | color(cfg.colors.formula_fg),
        text("n=" + std::to_string(d.values.size()) + "  ") | color(cfg.colors.dimmed),
    };
    if (!d.values.empty()) {
        const double mn = *std::min_element(d.values.begin(), d.values.end());
        const double mx = *std::max_element(d.values.begin(), d.values.end());
        chips.push_back(text("min " + num(mn) + "  max " + num(mx) + "  ")
                        | color(cfg.colors.dimmed));
    }
    chips.push_back(text("[c cycle]") | color(cfg.colors.dimmed));
    return hbox(std::move(chips));
}

}  // anonymous namespace

Element render_chart_panel(const Config& cfg, const Grid::ChartData& d,
                           int width, int height) {
    width  = std::max(width, 12);
    height = std::max(height, 4);

    if (d.values.empty())
        return vbox({
            header(cfg, d),
            text("  no numeric values to chart") | color(cfg.colors.dimmed),
        });

    Canvas cv(width * 2, height * 4);         // braille: 2×4 dots per cell
    const Color col = cfg.colors.formula_fg;

    switch (d.type) {
        case Grid::ChartType::Bar:
            draw_bars(cv, d.values, col);
            break;
        case Grid::ChartType::Line:
            draw_line(cv, d.values, col);
            break;
        case Grid::ChartType::Histogram: {
            const int bins = std::clamp(
                static_cast<int>(std::ceil(std::sqrt((double)d.values.size()))),
                1, width);
            const auto counts = charts::histogram_bins(d.values, bins);
            std::vector<double> heights(counts.begin(), counts.end());
            draw_bars(cv, heights, col);
            break;
        }
    }

    return vbox({ header(cfg, d), canvas(std::move(cv)) });
}
