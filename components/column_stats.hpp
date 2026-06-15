#pragma once
#include <ftxui/dom/elements.hpp>

#include "grid.hpp"

struct Config;

// Renders the one-line summary strip for the column under the cursor.
// Caller only renders this when stats.valid is true.
ftxui::Element render_column_stats(const Config& cfg, const Grid::ColumnStats& stats);
