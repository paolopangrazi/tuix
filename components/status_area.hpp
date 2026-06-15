#pragma once
#include <string>
#include <ftxui/dom/elements.hpp>
#include "grid.hpp"

struct Config;

// Renders the multi-line status panel at the bottom of the main grid view.
// Returns either the cmd-mode prompt (when cmd_mode is true) or the
// FILE / MODE / hint / F1-F12 stack.
ftxui::Element render_status_area(
    const Config& cfg,
    bool cmd_mode,
    const std::string& cmd_buf,
    Grid::Mode mode,
    const std::string& hint,
    int rows,
    int cols
);
