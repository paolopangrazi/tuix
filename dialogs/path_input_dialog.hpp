#pragma once
#include <functional>
#include <string>
#include <ftxui/component/component.hpp>

struct Config;

// Generic "type a path and Enter" dialog, used by Save As and Open.
class PathInputDialog {
public:
    PathInputDialog(const Config& cfg,
                    std::string title,                                 // e.g. "Save As"
                    std::string action_label,                          // e.g. "save"
                    std::string placeholder,                           // e.g. "filename.csv"
                    std::function<std::string()> get_dir_display,      // for the "dir:" line
                    std::function<void(const std::string&)> on_submit, // called with raw buffer
                    std::function<void()> on_close);

    void set_buffer(std::string s);
    void clear_buffer();
    ftxui::Component component();

private:
    const Config& m_cfg;
    std::string   m_title;
    std::string   m_action_label;
    std::function<std::string()>            m_get_dir;
    std::function<void(const std::string&)> m_on_submit;
    std::function<void()>                   m_on_close;
    std::string      m_buf;
    ftxui::Component m_input;
};
