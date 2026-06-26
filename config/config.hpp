#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <ftxui/screen/color.hpp>
#include <ftxui/component/event.hpp>

// Semantic color slots. Defaults are ANSI palette names so the *default* theme
// inherits the user's terminal palette (full compatibility with the underlying
// terminal/OS theme). Named TrueColor presets (config/themes.hpp) override these
// with explicit RGB; a config.toml [colors] block overrides whatever the theme
// set. New "effect" slots use translucent RGBA overlays that blend over whatever
// background is underneath (FTXUI alpha-composites them), so they degrade
// gracefully on the transparent terminal background of the default theme.
struct Colors {
    ftxui::Color cursor_bg       = ftxui::Color::Green;
    ftxui::Color cursor_fg       = ftxui::Color::Black;
    ftxui::Color selection_bg    = ftxui::Color::Blue;
    ftxui::Color selection_fg    = ftxui::Color::White;
    ftxui::Color header          = ftxui::Color::Green;
    ftxui::Color row_number      = ftxui::Color::Green;
    ftxui::Color dimmed          = ftxui::Color::GrayLight;
    ftxui::Color insert_badge_bg = ftxui::Color::Green;
    ftxui::Color insert_badge_fg = ftxui::Color::Black;
    ftxui::Color normal_badge_bg = ftxui::Color::Blue;
    ftxui::Color normal_badge_fg = ftxui::Color::White;
    ftxui::Color titlebar_bg     = ftxui::Color::Green;
    ftxui::Color titlebar_fg     = ftxui::Color::Black;
    ftxui::Color formula_fg      = ftxui::Color::Cyan;

    // ── TrueColor effect slots (new) ─────────────────────────────────────────
    ftxui::Color accent          = ftxui::Color::Green;          // gradient/glow A
    ftxui::Color accent2         = ftxui::Color::Cyan;           // gradient/glow B
    ftxui::Color border          = ftxui::Color::GrayDark;       // idle panel border
    ftxui::Color border_focus    = ftxui::Color::Green;          // focused panel border
    ftxui::Color grid_bg         = ftxui::Color::Default;        // canvas (transparent → terminal bg)
    ftxui::Color zebra_bg        = ftxui::Color::RGBA(255, 255, 255, 10);  // alt-row tint
    ftxui::Color header_bg       = ftxui::Color::RGBA(255, 255, 255, 18);  // header band
    ftxui::Color crosshair       = ftxui::Color::Yellow;         // active row/col label accent
    ftxui::Color search_bg       = ftxui::Color::Cyan;
    ftxui::Color search_fg       = ftxui::Color::Black;
    ftxui::Color yank_fg         = ftxui::Color::Yellow;
    ftxui::Color scrollbar_thumb = ftxui::Color::Green;
    ftxui::Color scrollbar_track = ftxui::Color::GrayDark;
};

struct Keys {
    std::vector<char> nav_up      = {'k'};
    std::vector<char> nav_down    = {'j'};
    std::vector<char> nav_left    = {'h'};
    std::vector<char> nav_right   = {'l'};
    std::vector<char> insert_mode = {'i', 'a'};
    std::vector<char> delete_cell = {'x'};
    std::vector<char> undo        = {'u'};
    std::vector<char> insert_row  = {'+'};
    std::vector<char> delete_row  = {'-'};
    std::vector<char> insert_col  = {'+'};
    std::vector<char> delete_col  = {'-'};
    std::vector<char> rename_col  = {'i', 'a'};
    std::vector<char> col_widen   = {'>'};
    std::vector<char> col_narrow  = {'<'};
    std::vector<char> row_taller  = {'}'};
    std::vector<char> row_shorter = {'{'};
    std::vector<char> sort_col    = {'s'};
    std::vector<char> heatmap     = {'H'};
    std::vector<char> cmd_mode    = {':'};
};

struct GridCfg {
    int  cell_width   = 12;
    bool start_insert = false;
};

struct ThemeCfg {
    std::string name       = "default";  // "default" → inherit terminal palette
    bool        zebra      = true;        // alternate-row tinting
    bool        crosshair  = true;        // highlight active row/col labels
    bool        animations = false;       // gated motion (calc shimmer, logo sweep)
};

struct Config {
    Colors   colors;
    Keys     keys;
    GridCfg  grid;
    ThemeCfg theme;

    static Config load();
    static std::filesystem::path config_file_path();
    static std::string color_to_name(ftxui::Color c);

    bool key_is(const ftxui::Event& e, const std::vector<char>& binding) const;
};
