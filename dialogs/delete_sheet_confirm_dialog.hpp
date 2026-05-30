#pragma once
#include <functional>
#include <string>
#include <ftxui/component/component.hpp>

struct Config;

class DeleteSheetConfirmDialog {
public:
    DeleteSheetConfirmDialog(const Config& cfg,
                             std::function<std::string()> get_sheet_name,
                             std::function<void()> on_yes,
                             std::function<void()> on_close);
    ftxui::Component component();

private:
    const Config& m_cfg;
    std::function<std::string()> m_get_sheet_name;
    std::function<void()>        m_on_close;
    ftxui::Component m_yes, m_no, m_container;
};
