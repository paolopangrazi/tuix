#pragma once
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>

struct Config;

// Common dialog body: filler · inner · filler · separator · bottom bar.
// The outer chrome (title logo + buttons row) is added at the root, not here.
ftxui::Element render_dialog_shell(
    ftxui::Element inner,
    ftxui::Element bottom_bar
);

ftxui::ButtonOption make_dialog_btn_style(const Config& cfg);
