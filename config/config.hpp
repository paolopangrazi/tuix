#pragma once
#include <string>
#include <vector>
#include <ftxui/screen/color.hpp>
#include <ftxui/component/event.hpp>

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
    std::vector<char> cmd_mode    = {':'};
};

struct GridCfg {
    int cell_width = 12;
};

struct Config {
    Colors  colors;
    Keys    keys;
    GridCfg grid;

    static Config load();  // loads from ~/.config/tuix/config.toml, falls back to defaults

    bool key_is(const ftxui::Event& e, const std::vector<char>& binding) const;
};
