#pragma once
#include <string>
#include <ftxui/dom/elements.hpp>

struct Config;

// Renders the file-info strip shown below the formula/suggestion bar.
// Caller only renders this when file_info is non-empty.
ftxui::Element render_file_bar(const Config& cfg, const std::string& file_info);
