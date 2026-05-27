#pragma once
#include <functional>
#include <string>
#include <vector>
#include <ftxui/component/component.hpp>

class TitleBar;
struct Config;

class HelpDialog {
public:
    HelpDialog(TitleBar& titlebar,
               const Config& cfg,
               std::function<void()> on_close);

    void reset_tab();
    ftxui::Component component();

private:
    TitleBar&     m_tb;
    const Config& m_cfg;
    std::function<void()> m_on_close;

    int                      m_tab = 0;
    std::vector<std::string> m_tab_names;
    ftxui::Component         m_tabs_menu;
    ftxui::Component         m_content;
    ftxui::Component         m_container;
};
