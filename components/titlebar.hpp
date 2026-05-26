#pragma once
#include <functional>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include "config/config.hpp"

class TitleBar {
public:
    TitleBar(
        std::function<void()> on_undo,
        std::function<bool()> can_undo,
        std::function<void()> on_redo,
        std::function<bool()> can_redo,
        std::function<void()> on_open,
        std::function<void()> on_save,
        std::function<void()> on_save_as,
        std::function<void()> on_exit,
        const Config& cfg = {}
    );

    ftxui::Element   render_logo()    const;
    ftxui::Element   render_buttons() const;
    ftxui::Component component();

private:
    Config m_cfg;
    std::function<bool()> m_can_undo;
    std::function<bool()> m_can_redo;

    ftxui::Component m_btn_undo;
    ftxui::Component m_btn_redo;
    ftxui::Component m_btn_open;
    ftxui::Component m_btn_save;
    ftxui::Component m_btn_save_as;
    ftxui::Component m_btn_exit;
    ftxui::Component m_btns;
};
