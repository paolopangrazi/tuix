#include "file_bar.hpp"

#include "config/config.hpp"

using namespace ftxui;

Element render_file_bar(const Config& cfg, const std::string& file_info) {
    return hbox({
        text(" ▸ ") | bold | color(cfg.colors.header),
        text(file_info) | color(cfg.colors.dimmed),
    });
}
