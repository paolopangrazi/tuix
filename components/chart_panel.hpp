#pragma once
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "grid.hpp"

struct Config;

// Renders the Unicode chart panel (bar / line / histogram) for the current
// selection or column. Drawn with FTXUI's braille Canvas, sized to `width` and
// `height` in terminal cells. Caller renders this only when data.active.
ftxui::Element render_chart_panel(const Config& cfg, const Grid::ChartData& data,
                                  int width, int height);

namespace charts {

// Bin `values` into `bins` equal-width buckets across [min, max], returning the
// count per bucket. A degenerate range (all equal) puts everything in bin 0.
// Pure and testable — the visual drawing lives in the .cpp.
std::vector<int> histogram_bins(const std::vector<double>& values, int bins);

}  // namespace charts
