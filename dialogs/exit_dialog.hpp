#pragma once
#include <functional>
#include <ftxui/component/component.hpp>

class TitleBar;
struct Config;

class ExitDialog {
public:
    ExitDialog(TitleBar& titlebar,
               const Config& cfg,
               std::function<void()> on_yes,
               std::function<void()> on_close);
    ftxui::Component component();

private:
    TitleBar&     m_tb;
    const Config& m_cfg;
    std::function<void()> m_on_close;
    ftxui::Component m_yes, m_no, m_container;
};
