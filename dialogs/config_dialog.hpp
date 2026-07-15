#pragma once
#include <functional>
#include <string>
#include <vector>
#include <ftxui/component/component.hpp>

struct Config;

class ConfigDialog {
public:
    ConfigDialog(const Config& cfg,
                 std::function<void()> on_close);

    void refresh_from_cfg();   // re-read cfg values into buffers (call on open)
    void save_to_file();       // merge current buffers into ~/.config/tuix/config.toml
    ftxui::Component component();

private:
    const Config& m_cfg;
    std::function<void()> m_on_close;

    // One buffer + Input per schema slot (config/config_schema.hpp), in slot
    // order. Sized once in the constructor before the Inputs take pointers
    // into them, and never resized after that.
    std::vector<std::string>      m_color_bufs;
    std::vector<ftxui::Component> m_color_inputs;
    std::vector<std::string>      m_key_bufs;
    std::vector<ftxui::Component> m_key_inputs;

    std::string gb_cell_width;
    std::string gb_start_mode;
    std::string m_status;

    int m_tab = 0;
    std::vector<std::string> m_tab_names;

    ftxui::Component in_cell_width, in_start_mode;
    ftxui::Component m_tab_toggle, m_content, m_save_btn, m_container;
};
