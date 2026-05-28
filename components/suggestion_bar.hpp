#pragma once
#include <vector>
#include <ftxui/dom/elements.hpp>
#include "grid.hpp"

struct Config;

// Renders the formula-suggestion strip (ƒ NAME → value …) shown between the
// grid and the status bar. Caller only renders this when the list is non-empty.
ftxui::Element render_suggestion_bar(const Config& cfg,
                                     const std::vector<Grid::Suggestion>& suggestions);
