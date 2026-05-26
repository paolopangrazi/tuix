#include "config.hpp"

#include <toml++/toml.hpp>
#include <ftxui/screen/color.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

using namespace ftxui;

// ---- color parsing --------------------------------------------------------

static const std::unordered_map<std::string, Color> k_named = {
    {"black",         Color::Black},
    {"red",           Color::Red},
    {"green",         Color::Green},
    {"yellow",        Color::Yellow},
    {"blue",          Color::Blue},
    {"magenta",       Color::Magenta},
    {"cyan",          Color::Cyan},
    {"gray_light",    Color::GrayLight},
    {"gray_dark",     Color::GrayDark},
    {"red_light",     Color::RedLight},
    {"green_light",   Color::GreenLight},
    {"yellow_light",  Color::YellowLight},
    {"blue_light",    Color::BlueLight},
    {"magenta_light", Color::MagentaLight},
    {"cyan_light",    Color::CyanLight},
    {"white",         Color::White},
};

static Color parse_color(const toml::node& node, Color fallback) {
    if (auto* s = node.as_string()) {
        auto it = k_named.find(s->get());
        if (it != k_named.end()) return it->second;
        return fallback;
    }
    if (auto* i = node.as_integer()) {
        int idx = static_cast<int>(i->get());
        if (idx >= 0 && idx <= 15) return Color(Color::Palette16(idx));
    }
    return fallback;
}

// ---- key parsing ----------------------------------------------------------

static std::vector<char> parse_keys(const toml::node& node, std::vector<char> fallback) {
    auto* arr = node.as_array();
    if (!arr) return fallback;
    std::vector<char> result;
    for (const auto& elem : *arr) {
        if (auto* s = elem.as_string()) {
            const auto& str = s->get();
            if (str.size() == 1) result.push_back(str[0]);
        }
    }
    return result.empty() ? fallback : result;
}

// ---- config path ----------------------------------------------------------

static std::filesystem::path config_path() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) return std::filesystem::path(xdg) / "tuix" / "config.toml";
    const char* home = std::getenv("HOME");
    if (home && *home) return std::filesystem::path(home) / ".config" / "tuix" / "config.toml";
    return "";
}

// ---- public ---------------------------------------------------------------

Config Config::load() {
    Config cfg;
    auto path = config_path();
    if (path.empty() || !std::filesystem::exists(path)) return cfg;

    toml::table tbl;
    try { tbl = toml::parse_file(path.string()); }
    catch (...) { return cfg; }

    if (auto* c = tbl["colors"].as_table()) {
        auto col = [&](std::string_view key, Color def) -> Color {
            if (auto* n = c->get(key)) return parse_color(*n, def);
            return def;
        };
        cfg.colors.cursor_bg       = col("cursor_bg",       cfg.colors.cursor_bg);
        cfg.colors.cursor_fg       = col("cursor_fg",       cfg.colors.cursor_fg);
        cfg.colors.selection_bg    = col("selection_bg",    cfg.colors.selection_bg);
        cfg.colors.selection_fg    = col("selection_fg",    cfg.colors.selection_fg);
        cfg.colors.header          = col("header",          cfg.colors.header);
        cfg.colors.row_number      = col("row_number",      cfg.colors.row_number);
        cfg.colors.dimmed          = col("dimmed",          cfg.colors.dimmed);
        cfg.colors.insert_badge_bg = col("insert_badge_bg", cfg.colors.insert_badge_bg);
        cfg.colors.insert_badge_fg = col("insert_badge_fg", cfg.colors.insert_badge_fg);
        cfg.colors.normal_badge_bg = col("normal_badge_bg", cfg.colors.normal_badge_bg);
        cfg.colors.normal_badge_fg = col("normal_badge_fg", cfg.colors.normal_badge_fg);
        cfg.colors.titlebar_bg     = col("titlebar_bg",     cfg.colors.titlebar_bg);
        cfg.colors.titlebar_fg     = col("titlebar_fg",     cfg.colors.titlebar_fg);
        cfg.colors.formula_fg      = col("formula_fg",      cfg.colors.formula_fg);
    }

    if (auto* k = tbl["keys"].as_table()) {
        auto keys = [&](std::string_view key, std::vector<char> def) -> std::vector<char> {
            if (auto* n = k->get(key)) return parse_keys(*n, def);
            return def;
        };
        cfg.keys.nav_up      = keys("nav_up",      cfg.keys.nav_up);
        cfg.keys.nav_down    = keys("nav_down",     cfg.keys.nav_down);
        cfg.keys.nav_left    = keys("nav_left",     cfg.keys.nav_left);
        cfg.keys.nav_right   = keys("nav_right",    cfg.keys.nav_right);
        cfg.keys.insert_mode = keys("insert_mode",  cfg.keys.insert_mode);
        cfg.keys.delete_cell = keys("delete_cell",  cfg.keys.delete_cell);
        cfg.keys.undo        = keys("undo",         cfg.keys.undo);
        cfg.keys.insert_row  = keys("insert_row",   cfg.keys.insert_row);
        cfg.keys.delete_row  = keys("delete_row",   cfg.keys.delete_row);
        cfg.keys.insert_col  = keys("insert_col",   cfg.keys.insert_col);
        cfg.keys.delete_col  = keys("delete_col",   cfg.keys.delete_col);
        cfg.keys.rename_col  = keys("rename_col",   cfg.keys.rename_col);
        cfg.keys.cmd_mode    = keys("cmd_mode",     cfg.keys.cmd_mode);
    }

    if (auto* g = tbl["grid"].as_table()) {
        if (auto w = (*g)["cell_width"].value<int>())
            cfg.grid.cell_width = std::max(4, *w);
    }

    return cfg;
}

bool Config::key_is(const Event& e, const std::vector<char>& binding) const {
    for (char k : binding)
        if (e == Event::Character(k)) return true;
    return false;
}
