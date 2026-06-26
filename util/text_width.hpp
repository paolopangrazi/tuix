#pragma once
#include <string>

// Display-width helpers for grid cell layout. The grid measures and clips cell
// text in terminal columns, but a UTF-8 string's byte length is not its column
// width (e.g. a block glyph "▇" is 3 bytes but occupies 1 column, and CJK
// glyphs occupy 2). These wrap FTXUI's unicode-aware width handling so column
// auto-fit and truncation stay correct for any multibyte content.
namespace tuix {

// Number of terminal columns `s` occupies when rendered.
int display_width(const std::string& s);

// Longest prefix of `s` whose display width does not exceed `max_cols`,
// truncated on glyph boundaries (never mid-codepoint).
std::string truncate_to_width(const std::string& s, int max_cols);

}  // namespace tuix
