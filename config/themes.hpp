#pragma once
#include <string>
#include <vector>

#include "config.hpp"

// Built-in TrueColor theme presets. apply_theme() overwrites `c` with the named
// palette and returns true; an unknown name (including "default") leaves `c`
// untouched so it keeps the terminal-native ANSI defaults. A config.toml
// [colors] block is applied *after* this, so per-color overrides always win.
bool apply_theme(const std::string& name, Colors& c);

// Names of the shipped presets (for the config dialog / docs), "default" first.
std::vector<std::string> theme_names();
