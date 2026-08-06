#pragma once
#include <ftxui/dom/flexbox_config.hpp>

namespace tuix {

// The FlexboxConfig used everywhere a row of chips/hints should wrap and
// pack to the start (status bar hints, suggestion/column-stat chips,
// titlebar buttons, dialog key-hint rows).
inline ftxui::FlexboxConfig flex_wrap_left() {
    return ftxui::FlexboxConfig()
        .Set(ftxui::FlexboxConfig::Wrap::Wrap)
        .Set(ftxui::FlexboxConfig::JustifyContent::FlexStart)
        .Set(ftxui::FlexboxConfig::AlignItems::FlexStart)
        .Set(ftxui::FlexboxConfig::AlignContent::FlexStart);
}

}  // namespace tuix
