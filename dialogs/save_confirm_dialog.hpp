#pragma once
#include <functional>
#include <string>
#include <ftxui/component/component.hpp>

class TitleBar;
struct Config;

class SaveConfirmDialog {
public:
    SaveConfirmDialog(TitleBar& titlebar,
                      const Config& cfg,
                      std::function<std::string()> get_path,
                      std::function<void()> on_yes,
                      std::function<void()> on_close);
    ftxui::Component component();

private:
    TitleBar&     m_tb;
    const Config& m_cfg;
    std::function<std::string()> m_get_path;
    std::function<void()>        m_on_close;
    ftxui::Component m_yes, m_no, m_container;
};
