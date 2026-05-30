#pragma once
#include <functional>
#include <string>
#include <ftxui/component/component.hpp>

struct Config;

// Shown when the user clicks the active tab. Offers Rename / Delete / Cancel.
// Delete is hidden when only a single sheet remains (can_delete() == false).
class SheetActionsDialog {
public:
    SheetActionsDialog(const Config& cfg,
                       std::function<std::string()> get_sheet_name,
                       std::function<bool()>        can_delete,
                       std::function<void()>        on_rename,
                       std::function<void()>        on_delete,
                       std::function<void()>        on_close);
    ftxui::Component component();

private:
    const Config& m_cfg;
    std::function<std::string()> m_get_sheet_name;
    std::function<bool()>        m_can_delete;
    std::function<void()>        m_on_close;
    ftxui::Component m_rename, m_delete, m_cancel, m_container;
};
