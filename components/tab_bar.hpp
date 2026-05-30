#pragma once
#include <functional>
#include <string>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include "config/config.hpp"

// Bottom-of-screen sheet tab strip (Excel/LibreOffice style).
//
// The bar holds no data — it queries the active index, count, and per-index
// names through callbacks each frame, so it stays correct without explicit
// notifications when sheets are added or renamed.
class TabBar {
public:
    TabBar(const Config& cfg,
           std::function<int()>                active,
           std::function<int()>                count,
           std::function<std::string(int)>     name_at,
           std::function<void(int)>            on_select,
           std::function<void()>               on_add,
           std::function<void(int)>            on_rename_request);

    ftxui::Component component();
    ftxui::Element   render() const;

private:
    Config                              m_cfg;
    std::function<int()>                m_active;
    std::function<int()>                m_count;
    std::function<std::string(int)>     m_name_at;
    std::function<void(int)>            m_on_select;
    std::function<void()>               m_on_add;
    std::function<void(int)>            m_on_rename_request;

    ftxui::Component m_container;
};
