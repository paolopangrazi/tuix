#include "text_width.hpp"

#include <ftxui/screen/string.hpp>

namespace tuix {

int display_width(const std::string& s) {
    return ftxui::string_width(s);
}

std::string truncate_to_width(const std::string& s, int max_cols) {
    if (max_cols <= 0) return "";
    if (display_width(s) <= max_cols) return s;
    std::string out;
    int used = 0;
    for (const std::string& g : ftxui::Utf8ToGlyphs(s)) {
        const int w = ftxui::string_width(g);
        if (used + w > max_cols) break;
        out += g;
        used += w;
    }
    return out;
}

}  // namespace tuix
