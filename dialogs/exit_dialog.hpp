#pragma once
#include <functional>
#include <ftxui/component/component.hpp>

struct Config;

class ExitDialog {
public:
    ExitDialog(const Config& cfg,
               std::function<void()> on_yes,
               std::function<void()> on_close);
    ftxui::Component component();

private:
    const Config& m_cfg;
    std::function<void()> m_on_close;
    ftxui::Component m_yes, m_no, m_container;
};
