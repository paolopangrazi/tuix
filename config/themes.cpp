#include "themes.hpp"

#include <ftxui/screen/color.hpp>

using ftxui::Color;

namespace {

// Tokyo Night (storm) — dark blue-violet, bright cyan/green accents.
Colors tokyo_night() {
    Colors c;
    c.grid_bg         = Color::RGB(0x1a, 0x1b, 0x26);
    c.cursor_bg       = Color::RGB(0x7a, 0xa2, 0xf7);  c.cursor_fg    = Color::RGB(0x1a, 0x1b, 0x26);
    c.selection_bg    = Color::RGB(0x28, 0x34, 0x57);  c.selection_fg = Color::RGB(0xc0, 0xca, 0xf5);
    c.header          = Color::RGB(0x7d, 0xcf, 0xff);
    c.row_number      = Color::RGB(0x56, 0x5f, 0x89);  c.dimmed       = Color::RGB(0x56, 0x5f, 0x89);
    c.formula_fg      = Color::RGB(0xbb, 0x9a, 0xf7);
    c.insert_badge_bg = Color::RGB(0x9e, 0xce, 0x6a);  c.insert_badge_fg = Color::RGB(0x1a, 0x1b, 0x26);
    c.normal_badge_bg = Color::RGB(0x7a, 0xa2, 0xf7);  c.normal_badge_fg = Color::RGB(0x1a, 0x1b, 0x26);
    c.titlebar_bg     = Color::RGB(0x7a, 0xa2, 0xf7);  c.titlebar_fg  = Color::RGB(0x1a, 0x1b, 0x26);
    c.accent          = Color::RGB(0x7a, 0xa2, 0xf7);  c.accent2      = Color::RGB(0x7d, 0xcf, 0xff);
    c.border          = Color::RGB(0x3b, 0x42, 0x61);  c.border_focus = Color::RGB(0x7a, 0xa2, 0xf7);
    c.zebra_bg        = Color::RGB(0x1f, 0x23, 0x35);  c.header_bg    = Color::RGB(0x24, 0x28, 0x3b);
    c.crosshair       = Color::RGB(0xe0, 0xaf, 0x68);
    c.search_bg       = Color::RGB(0x7d, 0xcf, 0xff);  c.search_fg    = Color::RGB(0x1a, 0x1b, 0x26);
    c.yank_fg         = Color::RGB(0xe0, 0xaf, 0x68);
    c.scrollbar_thumb = Color::RGB(0x7a, 0xa2, 0xf7);  c.scrollbar_track = Color::RGB(0x29, 0x2e, 0x42);
    return c;
}

// Catppuccin Mocha — soft warm pastels on dark.
Colors catppuccin() {
    Colors c;
    c.grid_bg         = Color::RGB(0x1e, 0x1e, 0x2e);
    c.cursor_bg       = Color::RGB(0x89, 0xb4, 0xfa);  c.cursor_fg    = Color::RGB(0x1e, 0x1e, 0x2e);
    c.selection_bg    = Color::RGB(0x31, 0x32, 0x44);  c.selection_fg = Color::RGB(0xcd, 0xd6, 0xf4);
    c.header          = Color::RGB(0x89, 0xdc, 0xeb);
    c.row_number      = Color::RGB(0x6c, 0x70, 0x86);  c.dimmed       = Color::RGB(0x6c, 0x70, 0x86);
    c.formula_fg      = Color::RGB(0xcb, 0xa6, 0xf7);
    c.insert_badge_bg = Color::RGB(0xa6, 0xe3, 0xa1);  c.insert_badge_fg = Color::RGB(0x1e, 0x1e, 0x2e);
    c.normal_badge_bg = Color::RGB(0x89, 0xb4, 0xfa);  c.normal_badge_fg = Color::RGB(0x1e, 0x1e, 0x2e);
    c.titlebar_bg     = Color::RGB(0x89, 0xb4, 0xfa);  c.titlebar_fg  = Color::RGB(0x1e, 0x1e, 0x2e);
    c.accent          = Color::RGB(0xcb, 0xa6, 0xf7);  c.accent2      = Color::RGB(0x89, 0xb4, 0xfa);
    c.border          = Color::RGB(0x45, 0x47, 0x5a);  c.border_focus = Color::RGB(0x89, 0xb4, 0xfa);
    c.zebra_bg        = Color::RGB(0x24, 0x24, 0x37);  c.header_bg    = Color::RGB(0x31, 0x32, 0x44);
    c.crosshair       = Color::RGB(0xf9, 0xe2, 0xaf);
    c.search_bg       = Color::RGB(0x89, 0xdc, 0xeb);  c.search_fg    = Color::RGB(0x1e, 0x1e, 0x2e);
    c.yank_fg         = Color::RGB(0xfa, 0xb3, 0x87);
    c.scrollbar_thumb = Color::RGB(0x89, 0xb4, 0xfa);  c.scrollbar_track = Color::RGB(0x31, 0x32, 0x44);
    return c;
}

// Nord — cool muted arctic blues/grays.
Colors nord() {
    Colors c;
    c.grid_bg         = Color::RGB(0x2e, 0x34, 0x40);
    c.cursor_bg       = Color::RGB(0x88, 0xc0, 0xd0);  c.cursor_fg    = Color::RGB(0x2e, 0x34, 0x40);
    c.selection_bg    = Color::RGB(0x43, 0x4c, 0x5e);  c.selection_fg = Color::RGB(0xec, 0xef, 0xf4);
    c.header          = Color::RGB(0x8f, 0xbc, 0xbb);
    c.row_number      = Color::RGB(0x4c, 0x56, 0x6a);  c.dimmed       = Color::RGB(0x4c, 0x56, 0x6a);
    c.formula_fg      = Color::RGB(0xb4, 0x8e, 0xad);
    c.insert_badge_bg = Color::RGB(0xa3, 0xbe, 0x8c);  c.insert_badge_fg = Color::RGB(0x2e, 0x34, 0x40);
    c.normal_badge_bg = Color::RGB(0x81, 0xa1, 0xc1);  c.normal_badge_fg = Color::RGB(0x2e, 0x34, 0x40);
    c.titlebar_bg     = Color::RGB(0x88, 0xc0, 0xd0);  c.titlebar_fg  = Color::RGB(0x2e, 0x34, 0x40);
    c.accent          = Color::RGB(0x88, 0xc0, 0xd0);  c.accent2      = Color::RGB(0x8f, 0xbc, 0xbb);
    c.border          = Color::RGB(0x43, 0x4c, 0x5e);  c.border_focus = Color::RGB(0x88, 0xc0, 0xd0);
    c.zebra_bg        = Color::RGB(0x35, 0x3b, 0x49);  c.header_bg    = Color::RGB(0x3b, 0x42, 0x52);
    c.crosshair       = Color::RGB(0xeb, 0xcb, 0x8b);
    c.search_bg       = Color::RGB(0x8f, 0xbc, 0xbb);  c.search_fg    = Color::RGB(0x2e, 0x34, 0x40);
    c.yank_fg         = Color::RGB(0xeb, 0xcb, 0x8b);
    c.scrollbar_thumb = Color::RGB(0x88, 0xc0, 0xd0);  c.scrollbar_track = Color::RGB(0x3b, 0x42, 0x52);
    return c;
}

// Gruvbox (dark) — warm retro earth tones.
Colors gruvbox() {
    Colors c;
    c.grid_bg         = Color::RGB(0x28, 0x28, 0x28);
    c.cursor_bg       = Color::RGB(0xfa, 0xbd, 0x2f);  c.cursor_fg    = Color::RGB(0x28, 0x28, 0x28);
    c.selection_bg    = Color::RGB(0x50, 0x49, 0x45);  c.selection_fg = Color::RGB(0xeb, 0xdb, 0xb2);
    c.header          = Color::RGB(0xb8, 0xbb, 0x26);
    c.row_number      = Color::RGB(0x92, 0x83, 0x74);  c.dimmed       = Color::RGB(0x92, 0x83, 0x74);
    c.formula_fg      = Color::RGB(0x8e, 0xc0, 0x7c);
    c.insert_badge_bg = Color::RGB(0xb8, 0xbb, 0x26);  c.insert_badge_fg = Color::RGB(0x28, 0x28, 0x28);
    c.normal_badge_bg = Color::RGB(0x83, 0xa5, 0x98);  c.normal_badge_fg = Color::RGB(0x28, 0x28, 0x28);
    c.titlebar_bg     = Color::RGB(0xfa, 0xbd, 0x2f);  c.titlebar_fg  = Color::RGB(0x28, 0x28, 0x28);
    c.accent          = Color::RGB(0xfa, 0xbd, 0x2f);  c.accent2      = Color::RGB(0xfe, 0x80, 0x19);
    c.border          = Color::RGB(0x50, 0x49, 0x45);  c.border_focus = Color::RGB(0xfa, 0xbd, 0x2f);
    c.zebra_bg        = Color::RGB(0x32, 0x30, 0x2f);  c.header_bg    = Color::RGB(0x3c, 0x38, 0x36);
    c.crosshair       = Color::RGB(0xfb, 0x49, 0x34);
    c.search_bg       = Color::RGB(0xb8, 0xbb, 0x26);  c.search_fg    = Color::RGB(0x28, 0x28, 0x28);
    c.yank_fg         = Color::RGB(0xfe, 0x80, 0x19);
    c.scrollbar_thumb = Color::RGB(0xfa, 0xbd, 0x2f);  c.scrollbar_track = Color::RGB(0x3c, 0x38, 0x36);
    return c;
}

// Dracula — vivid purple/pink on charcoal.
Colors dracula() {
    Colors c;
    c.grid_bg         = Color::RGB(0x28, 0x2a, 0x36);
    c.cursor_bg       = Color::RGB(0xbd, 0x93, 0xf9);  c.cursor_fg    = Color::RGB(0x28, 0x2a, 0x36);
    c.selection_bg    = Color::RGB(0x44, 0x47, 0x5a);  c.selection_fg = Color::RGB(0xf8, 0xf8, 0xf2);
    c.header          = Color::RGB(0x8b, 0xe9, 0xfd);
    c.row_number      = Color::RGB(0x62, 0x72, 0xa4);  c.dimmed       = Color::RGB(0x62, 0x72, 0xa4);
    c.formula_fg      = Color::RGB(0xff, 0x79, 0xc6);
    c.insert_badge_bg = Color::RGB(0x50, 0xfa, 0x7b);  c.insert_badge_fg = Color::RGB(0x28, 0x2a, 0x36);
    c.normal_badge_bg = Color::RGB(0xbd, 0x93, 0xf9);  c.normal_badge_fg = Color::RGB(0x28, 0x2a, 0x36);
    c.titlebar_bg     = Color::RGB(0xbd, 0x93, 0xf9);  c.titlebar_fg  = Color::RGB(0x28, 0x2a, 0x36);
    c.accent          = Color::RGB(0xbd, 0x93, 0xf9);  c.accent2      = Color::RGB(0xff, 0x79, 0xc6);
    c.border          = Color::RGB(0x44, 0x47, 0x5a);  c.border_focus = Color::RGB(0xbd, 0x93, 0xf9);
    c.zebra_bg        = Color::RGB(0x2d, 0x2f, 0x3d);  c.header_bg    = Color::RGB(0x34, 0x37, 0x46);
    c.crosshair       = Color::RGB(0xf1, 0xfa, 0x8c);
    c.search_bg       = Color::RGB(0x8b, 0xe9, 0xfd);  c.search_fg    = Color::RGB(0x28, 0x2a, 0x36);
    c.yank_fg         = Color::RGB(0xf1, 0xfa, 0x8c);
    c.scrollbar_thumb = Color::RGB(0xbd, 0x93, 0xf9);  c.scrollbar_track = Color::RGB(0x34, 0x37, 0x46);
    return c;
}

}  // namespace

bool apply_theme(const std::string& name, Colors& c) {
    if (name == "tokyo-night" || name == "tokyonight") { c = tokyo_night(); return true; }
    if (name == "catppuccin")                          { c = catppuccin();  return true; }
    if (name == "nord")                                { c = nord();        return true; }
    if (name == "gruvbox")                             { c = gruvbox();     return true; }
    if (name == "dracula")                             { c = dracula();     return true; }
    return false;  // "default" or unknown → keep terminal-native ANSI defaults
}

std::vector<std::string> theme_names() {
    return {"default", "tokyo-night", "catppuccin", "nord", "gruvbox", "dracula"};
}
