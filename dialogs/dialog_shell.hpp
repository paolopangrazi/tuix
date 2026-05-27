#pragma once
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>

class TitleBar;
struct Config;

ftxui::Element render_dialog_shell(
    const TitleBar& titlebar,
    ftxui::Element  inner,
    ftxui::Element  bottom_bar
);

ftxui::ButtonOption make_dialog_btn_style(const Config& cfg);
