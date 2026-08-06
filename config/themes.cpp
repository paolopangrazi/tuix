#include "themes.hpp"

#include <ftxui/screen/color.hpp>

using ftxui::Color;

namespace {

// Plain RGB triple — the presets below are pure data; to_colors() is the one
// place that knows how to turn a triple into an ftxui::Color.
struct RGB { unsigned char r, g, b; };

struct ThemeRGB {
    RGB grid_bg;
    RGB cursor_bg, cursor_fg;
    RGB selection_bg, selection_fg;
    RGB header;
    RGB row_number, dimmed;
    RGB formula_fg;
    RGB insert_badge_bg, insert_badge_fg;
    RGB normal_badge_bg, normal_badge_fg;
    RGB titlebar_bg, titlebar_fg;
    RGB accent, accent2;
    RGB border, border_focus;
    RGB zebra_bg, header_bg;
    RGB crosshair;
    RGB search_bg, search_fg;
    RGB yank_fg;
    RGB scrollbar_thumb, scrollbar_track;
};

Colors to_colors(const ThemeRGB& t) {
    auto rgb = [](RGB v) { return Color::RGB(v.r, v.g, v.b); };
    Colors c;
    c.grid_bg         = rgb(t.grid_bg);
    c.cursor_bg       = rgb(t.cursor_bg);        c.cursor_fg       = rgb(t.cursor_fg);
    c.selection_bg    = rgb(t.selection_bg);     c.selection_fg    = rgb(t.selection_fg);
    c.header          = rgb(t.header);
    c.row_number      = rgb(t.row_number);       c.dimmed          = rgb(t.dimmed);
    c.formula_fg      = rgb(t.formula_fg);
    c.insert_badge_bg = rgb(t.insert_badge_bg);  c.insert_badge_fg = rgb(t.insert_badge_fg);
    c.normal_badge_bg = rgb(t.normal_badge_bg);  c.normal_badge_fg = rgb(t.normal_badge_fg);
    c.titlebar_bg     = rgb(t.titlebar_bg);      c.titlebar_fg     = rgb(t.titlebar_fg);
    c.accent          = rgb(t.accent);           c.accent2         = rgb(t.accent2);
    c.border          = rgb(t.border);           c.border_focus    = rgb(t.border_focus);
    c.zebra_bg        = rgb(t.zebra_bg);         c.header_bg       = rgb(t.header_bg);
    c.crosshair       = rgb(t.crosshair);
    c.search_bg       = rgb(t.search_bg);        c.search_fg       = rgb(t.search_fg);
    c.yank_fg         = rgb(t.yank_fg);
    c.scrollbar_thumb = rgb(t.scrollbar_thumb);  c.scrollbar_track = rgb(t.scrollbar_track);
    return c;
}

// Tokyo Night (storm) — dark blue-violet, bright cyan/green accents.
constexpr ThemeRGB k_tokyo_night = {
    .grid_bg         = {0x1a, 0x1b, 0x26},
    .cursor_bg       = {0x7a, 0xa2, 0xf7}, .cursor_fg       = {0x1a, 0x1b, 0x26},
    .selection_bg    = {0x28, 0x34, 0x57}, .selection_fg    = {0xc0, 0xca, 0xf5},
    .header          = {0x7d, 0xcf, 0xff},
    .row_number      = {0x56, 0x5f, 0x89}, .dimmed          = {0x56, 0x5f, 0x89},
    .formula_fg      = {0xbb, 0x9a, 0xf7},
    .insert_badge_bg = {0x9e, 0xce, 0x6a}, .insert_badge_fg = {0x1a, 0x1b, 0x26},
    .normal_badge_bg = {0x7a, 0xa2, 0xf7}, .normal_badge_fg = {0x1a, 0x1b, 0x26},
    .titlebar_bg     = {0x7a, 0xa2, 0xf7}, .titlebar_fg     = {0x1a, 0x1b, 0x26},
    .accent          = {0x7a, 0xa2, 0xf7}, .accent2         = {0x7d, 0xcf, 0xff},
    .border          = {0x3b, 0x42, 0x61}, .border_focus    = {0x7a, 0xa2, 0xf7},
    .zebra_bg        = {0x1f, 0x23, 0x35}, .header_bg       = {0x24, 0x28, 0x3b},
    .crosshair       = {0xe0, 0xaf, 0x68},
    .search_bg       = {0x7d, 0xcf, 0xff}, .search_fg       = {0x1a, 0x1b, 0x26},
    .yank_fg         = {0xe0, 0xaf, 0x68},
    .scrollbar_thumb = {0x7a, 0xa2, 0xf7}, .scrollbar_track = {0x29, 0x2e, 0x42},
};

// Catppuccin Mocha — soft warm pastels on dark.
constexpr ThemeRGB k_catppuccin = {
    .grid_bg         = {0x1e, 0x1e, 0x2e},
    .cursor_bg       = {0x89, 0xb4, 0xfa}, .cursor_fg       = {0x1e, 0x1e, 0x2e},
    .selection_bg    = {0x31, 0x32, 0x44}, .selection_fg    = {0xcd, 0xd6, 0xf4},
    .header          = {0x89, 0xdc, 0xeb},
    .row_number      = {0x6c, 0x70, 0x86}, .dimmed          = {0x6c, 0x70, 0x86},
    .formula_fg      = {0xcb, 0xa6, 0xf7},
    .insert_badge_bg = {0xa6, 0xe3, 0xa1}, .insert_badge_fg = {0x1e, 0x1e, 0x2e},
    .normal_badge_bg = {0x89, 0xb4, 0xfa}, .normal_badge_fg = {0x1e, 0x1e, 0x2e},
    .titlebar_bg     = {0x89, 0xb4, 0xfa}, .titlebar_fg     = {0x1e, 0x1e, 0x2e},
    .accent          = {0xcb, 0xa6, 0xf7}, .accent2         = {0x89, 0xb4, 0xfa},
    .border          = {0x45, 0x47, 0x5a}, .border_focus    = {0x89, 0xb4, 0xfa},
    .zebra_bg        = {0x24, 0x24, 0x37}, .header_bg       = {0x31, 0x32, 0x44},
    .crosshair       = {0xf9, 0xe2, 0xaf},
    .search_bg       = {0x89, 0xdc, 0xeb}, .search_fg       = {0x1e, 0x1e, 0x2e},
    .yank_fg         = {0xfa, 0xb3, 0x87},
    .scrollbar_thumb = {0x89, 0xb4, 0xfa}, .scrollbar_track = {0x31, 0x32, 0x44},
};

// Nord — cool muted arctic blues/grays.
constexpr ThemeRGB k_nord = {
    .grid_bg         = {0x2e, 0x34, 0x40},
    .cursor_bg       = {0x88, 0xc0, 0xd0}, .cursor_fg       = {0x2e, 0x34, 0x40},
    .selection_bg    = {0x43, 0x4c, 0x5e}, .selection_fg    = {0xec, 0xef, 0xf4},
    .header          = {0x8f, 0xbc, 0xbb},
    .row_number      = {0x4c, 0x56, 0x6a}, .dimmed          = {0x4c, 0x56, 0x6a},
    .formula_fg      = {0xb4, 0x8e, 0xad},
    .insert_badge_bg = {0xa3, 0xbe, 0x8c}, .insert_badge_fg = {0x2e, 0x34, 0x40},
    .normal_badge_bg = {0x81, 0xa1, 0xc1}, .normal_badge_fg = {0x2e, 0x34, 0x40},
    .titlebar_bg     = {0x88, 0xc0, 0xd0}, .titlebar_fg     = {0x2e, 0x34, 0x40},
    .accent          = {0x88, 0xc0, 0xd0}, .accent2         = {0x8f, 0xbc, 0xbb},
    .border          = {0x43, 0x4c, 0x5e}, .border_focus    = {0x88, 0xc0, 0xd0},
    .zebra_bg        = {0x35, 0x3b, 0x49}, .header_bg       = {0x3b, 0x42, 0x52},
    .crosshair       = {0xeb, 0xcb, 0x8b},
    .search_bg       = {0x8f, 0xbc, 0xbb}, .search_fg       = {0x2e, 0x34, 0x40},
    .yank_fg         = {0xeb, 0xcb, 0x8b},
    .scrollbar_thumb = {0x88, 0xc0, 0xd0}, .scrollbar_track = {0x3b, 0x42, 0x52},
};

// Gruvbox (dark) — warm retro earth tones.
constexpr ThemeRGB k_gruvbox = {
    .grid_bg         = {0x28, 0x28, 0x28},
    .cursor_bg       = {0xfa, 0xbd, 0x2f}, .cursor_fg       = {0x28, 0x28, 0x28},
    .selection_bg    = {0x50, 0x49, 0x45}, .selection_fg    = {0xeb, 0xdb, 0xb2},
    .header          = {0xb8, 0xbb, 0x26},
    .row_number      = {0x92, 0x83, 0x74}, .dimmed          = {0x92, 0x83, 0x74},
    .formula_fg      = {0x8e, 0xc0, 0x7c},
    .insert_badge_bg = {0xb8, 0xbb, 0x26}, .insert_badge_fg = {0x28, 0x28, 0x28},
    .normal_badge_bg = {0x83, 0xa5, 0x98}, .normal_badge_fg = {0x28, 0x28, 0x28},
    .titlebar_bg     = {0xfa, 0xbd, 0x2f}, .titlebar_fg     = {0x28, 0x28, 0x28},
    .accent          = {0xfa, 0xbd, 0x2f}, .accent2         = {0xfe, 0x80, 0x19},
    .border          = {0x50, 0x49, 0x45}, .border_focus    = {0xfa, 0xbd, 0x2f},
    .zebra_bg        = {0x32, 0x30, 0x2f}, .header_bg       = {0x3c, 0x38, 0x36},
    .crosshair       = {0xfb, 0x49, 0x34},
    .search_bg       = {0xb8, 0xbb, 0x26}, .search_fg       = {0x28, 0x28, 0x28},
    .yank_fg         = {0xfe, 0x80, 0x19},
    .scrollbar_thumb = {0xfa, 0xbd, 0x2f}, .scrollbar_track = {0x3c, 0x38, 0x36},
};

// Dracula — vivid purple/pink on charcoal.
constexpr ThemeRGB k_dracula = {
    .grid_bg         = {0x28, 0x2a, 0x36},
    .cursor_bg       = {0xbd, 0x93, 0xf9}, .cursor_fg       = {0x28, 0x2a, 0x36},
    .selection_bg    = {0x44, 0x47, 0x5a}, .selection_fg    = {0xf8, 0xf8, 0xf2},
    .header          = {0x8b, 0xe9, 0xfd},
    .row_number      = {0x62, 0x72, 0xa4}, .dimmed          = {0x62, 0x72, 0xa4},
    .formula_fg      = {0xff, 0x79, 0xc6},
    .insert_badge_bg = {0x50, 0xfa, 0x7b}, .insert_badge_fg = {0x28, 0x2a, 0x36},
    .normal_badge_bg = {0xbd, 0x93, 0xf9}, .normal_badge_fg = {0x28, 0x2a, 0x36},
    .titlebar_bg     = {0xbd, 0x93, 0xf9}, .titlebar_fg     = {0x28, 0x2a, 0x36},
    .accent          = {0xbd, 0x93, 0xf9}, .accent2         = {0xff, 0x79, 0xc6},
    .border          = {0x44, 0x47, 0x5a}, .border_focus    = {0xbd, 0x93, 0xf9},
    .zebra_bg        = {0x2d, 0x2f, 0x3d}, .header_bg       = {0x34, 0x37, 0x46},
    .crosshair       = {0xf1, 0xfa, 0x8c},
    .search_bg       = {0x8b, 0xe9, 0xfd}, .search_fg       = {0x28, 0x2a, 0x36},
    .yank_fg         = {0xf1, 0xfa, 0x8c},
    .scrollbar_thumb = {0xbd, 0x93, 0xf9}, .scrollbar_track = {0x34, 0x37, 0x46},
};

}  // namespace

bool apply_theme(const std::string& name, Colors& c) {
    if (name == "tokyo-night" || name == "tokyonight") { c = to_colors(k_tokyo_night); return true; }
    if (name == "catppuccin")                          { c = to_colors(k_catppuccin);  return true; }
    if (name == "nord")                                { c = to_colors(k_nord);        return true; }
    if (name == "gruvbox")                             { c = to_colors(k_gruvbox);     return true; }
    if (name == "dracula")                             { c = to_colors(k_dracula);     return true; }
    return false;  // "default" or unknown → keep terminal-native ANSI defaults
}
