#include "config.hpp"
#include "config_schema.hpp"
#include "themes.hpp"
#include "util/env.hpp"

#include <toml++/toml.hpp>
#include <ftxui/screen/color.hpp>

#include <cctype>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
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

// Parse "#rgb", "#rrggbb", or "rgb(r,g,b)" → TrueColor; returns false if `s` is
// not a recognized color literal (so the caller can try named/palette forms).
static bool parse_truecolor(const std::string& s, Color& out) {
    auto hex1 = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        c = (char)std::tolower((unsigned char)c);
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        return -1;
    };
    if (!s.empty() && s[0] == '#') {
        const std::string h = s.substr(1);
        auto pair = [&](int i) { return hex1(h[i]) * 16 + hex1(h[i + 1]); };
        if (h.size() == 6) {
            for (char c : h) if (hex1(c) < 0) return false;
            out = Color::RGB(pair(0), pair(2), pair(4));
            return true;
        }
        if (h.size() == 3) {                       // #abc → #aabbcc
            for (char c : h) if (hex1(c) < 0) return false;
            auto dup = [&](int i) { int v = hex1(h[i]); return v * 16 + v; };
            out = Color::RGB(dup(0), dup(1), dup(2));
            return true;
        }
        return false;
    }
    if (s.rfind("rgb(", 0) == 0 && s.back() == ')') {
        // Hand-rolled rather than sscanf: MSVC deprecates the latter (C4996).
        std::string_view body(s);
        body.remove_prefix(4);
        body.remove_suffix(1);
        int comp[3] = {0, 0, 0};
        size_t pos = 0;
        for (int i = 0; i < 3; ++i) {
            while (pos < body.size() && std::isspace((unsigned char)body[pos])) ++pos;
            size_t start = pos;
            while (pos < body.size() && body[pos] >= '0' && body[pos] <= '9') ++pos;
            if (pos == start) return false;                       // no digits
            if (std::from_chars(body.data() + start, body.data() + pos, comp[i]).ec
                != std::errc{}) return false;
            while (pos < body.size() && std::isspace((unsigned char)body[pos])) ++pos;
            bool last = (i == 2);
            if (last ? pos != body.size() : (pos >= body.size() || body[pos++] != ','))
                return false;
        }
        auto clamp8 = [](int v) { return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v); };
        out = Color::RGB(clamp8(comp[0]), clamp8(comp[1]), clamp8(comp[2]));
        return true;
    }
    return false;
}

static Color parse_color(const toml::node& node, Color fallback) {
    if (auto* s = node.as_string()) {
        const std::string& str = s->get();
        Color rgb;
        if (parse_truecolor(str, rgb)) return rgb;   // #rrggbb / rgb(...)
        auto it = k_named.find(str);                 // ANSI name
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

// ---- schema ----------------------------------------------------------------
// The one list of toml key ↔ Config member pairs (see config_schema.hpp).

const ColorSlot k_color_slots[] = {
    { "cursor_bg",       &Colors::cursor_bg       },
    { "cursor_fg",       &Colors::cursor_fg       },
    { "selection_bg",    &Colors::selection_bg    },
    { "selection_fg",    &Colors::selection_fg    },
    { "header",          &Colors::header          },
    { "row_number",      &Colors::row_number      },
    { "dimmed",          &Colors::dimmed          },
    { "insert_badge_bg", &Colors::insert_badge_bg },
    { "insert_badge_fg", &Colors::insert_badge_fg },
    { "normal_badge_bg", &Colors::normal_badge_bg },
    { "normal_badge_fg", &Colors::normal_badge_fg },
    { "titlebar_bg",     &Colors::titlebar_bg     },
    { "titlebar_fg",     &Colors::titlebar_fg     },
    { "formula_fg",      &Colors::formula_fg      },
    { "accent",          &Colors::accent          },
    { "accent2",         &Colors::accent2         },
    { "border",          &Colors::border          },
    { "border_focus",    &Colors::border_focus    },
    { "grid_bg",         &Colors::grid_bg         },
    { "zebra_bg",        &Colors::zebra_bg        },
    { "header_bg",       &Colors::header_bg       },
    { "crosshair",       &Colors::crosshair       },
    { "search_bg",       &Colors::search_bg       },
    { "search_fg",       &Colors::search_fg       },
    { "yank_fg",         &Colors::yank_fg         },
    { "scrollbar_thumb", &Colors::scrollbar_thumb },
    { "scrollbar_track", &Colors::scrollbar_track },
};
const int k_color_slot_count = (int)(sizeof(k_color_slots) / sizeof(k_color_slots[0]));

const KeySlot k_key_slots[] = {
    { "nav_up",      &Keys::nav_up      },
    { "nav_down",    &Keys::nav_down    },
    { "nav_left",    &Keys::nav_left    },
    { "nav_right",   &Keys::nav_right   },
    { "insert_mode", &Keys::insert_mode },
    { "delete_cell", &Keys::delete_cell },
    { "undo",        &Keys::undo        },
    { "redo",        &Keys::redo        },
    { "insert_row",  &Keys::insert_row  },
    { "delete_row",  &Keys::delete_row  },
    { "insert_col",  &Keys::insert_col  },
    { "delete_col",  &Keys::delete_col  },
    { "rename_col",  &Keys::rename_col  },
    { "col_widen",   &Keys::col_widen   },
    { "col_narrow",  &Keys::col_narrow  },
    { "row_taller",  &Keys::row_taller  },
    { "row_shorter", &Keys::row_shorter },
    { "sort_col",    &Keys::sort_col    },
    { "heatmap",     &Keys::heatmap     },
    { "chart",       &Keys::chart       },
    { "cmd_mode",    &Keys::cmd_mode    },
};
const int k_key_slot_count = (int)(sizeof(k_key_slots) / sizeof(k_key_slots[0]));

// ---- config path ----------------------------------------------------------

static std::filesystem::path config_path() {
    using tuix::env_var;
    // XDG is honored on every platform if explicitly set.
    std::string xdg = env_var("XDG_CONFIG_HOME");
    if (!xdg.empty()) return std::filesystem::path(xdg) / "tuix" / "config.toml";
#ifdef _WIN32
    // Windows convention: %APPDATA%\tuix\config.toml, falling back to the
    // user profile if APPDATA is somehow unset.
    std::string appdata = env_var("APPDATA");
    if (!appdata.empty()) return std::filesystem::path(appdata) / "tuix" / "config.toml";
    std::string profile = env_var("USERPROFILE");
    if (!profile.empty())
        return std::filesystem::path(profile) / ".config" / "tuix" / "config.toml";
#else
    std::string home = env_var("HOME");
    if (!home.empty()) return std::filesystem::path(home) / ".config" / "tuix" / "config.toml";
#endif
    return "";
}

// ---- public ---------------------------------------------------------------

Config Config::load() {
    Config cfg;
    auto path = config_path();
    if (path.empty() || !std::filesystem::exists(path)) return cfg;

    toml::table tbl;
    try { tbl = toml::parse_file(path.string()); }
    catch (const std::exception& e) {
        cfg.load_warning = "config.toml ignored: " + std::string(e.what());
        return cfg;
    }
    catch (...) {
        cfg.load_warning = "config.toml ignored: parse error";
        return cfg;
    }

    // [theme] is applied first so a named preset becomes the base palette; the
    // [colors] block below then overrides individual slots on top of it.
    if (auto* t = tbl["theme"].as_table()) {
        if (auto n = (*t)["name"].value<std::string>()) {
            cfg.theme.name = *n;
            apply_theme(*n, cfg.colors);
        }
        if (auto z = (*t)["zebra"].value<bool>())      cfg.theme.zebra      = *z;
        if (auto x = (*t)["crosshair"].value<bool>())  cfg.theme.crosshair  = *x;
        if (auto a = (*t)["animations"].value<bool>()) cfg.theme.animations = *a;
    }

    if (auto* c = tbl["colors"].as_table()) {
        for (int i = 0; i < k_color_slot_count; ++i) {
            const ColorSlot& s = k_color_slots[i];
            if (auto* n = c->get(s.key))
                cfg.colors.*s.member = parse_color(*n, cfg.colors.*s.member);
        }
    }

    if (auto* k = tbl["keys"].as_table()) {
        for (int i = 0; i < k_key_slot_count; ++i) {
            const KeySlot& s = k_key_slots[i];
            if (auto* n = k->get(s.key))
                cfg.keys.*s.member = parse_keys(*n, cfg.keys.*s.member);
        }
    }

    if (auto* g = tbl["grid"].as_table()) {
        if (auto w = (*g)["cell_width"].value<int>())
            cfg.grid.cell_width = std::max(4, *w);
        if (auto s = (*g)["start_mode"].value<std::string>())
            cfg.grid.start_insert = (*s == "insert");
    }

    return cfg;
}

std::filesystem::path Config::config_file_path() { return config_path(); }

std::string Config::color_to_name(ftxui::Color c) {
    for (const auto& [name, col] : k_named)
        if (col == c) return name;
    return "";
}

bool Config::key_is(const Event& e, const std::vector<char>& binding) const {
    for (char k : binding)
        if (e == Event::Character(k)) return true;
    return false;
}
