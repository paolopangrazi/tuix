#pragma once
#include <vector>

#include <ftxui/screen/color.hpp>

#include "config.hpp"

// Single source of truth mapping config.toml keys to Config members. Both
// Config::load (config/config.cpp) and the F12 editor (dialogs/config_dialog.cpp)
// iterate these tables, so adding a slot here is all it takes to make it
// loadable, editable, and saveable — the two sides can no longer drift apart.
struct ColorSlot { const char* key; ftxui::Color Colors::*      member; };
struct KeySlot   { const char* key; std::vector<char> Keys::*   member; };

extern const ColorSlot k_color_slots[];
extern const int       k_color_slot_count;
extern const KeySlot   k_key_slots[];
extern const int       k_key_slot_count;
