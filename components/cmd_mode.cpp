#include "cmd_mode.hpp"

using namespace ftxui;

CmdMode::CmdMode(Actions actions) : m_actions(std::move(actions)) {}

void CmdMode::enter() {
    m_active = true;
    m_buf    = ":";
}

bool CmdMode::handle(Event e) {
    if (!m_active) return false;

    if (e == Event::Escape) {
        m_active = false;
        m_buf.clear();
        return true;
    }
    if (e == Event::Return) {
        bool quit      = (m_buf == ":q" || m_buf == ":q!");
        bool save      = (m_buf == ":w");
        bool save_quit = (m_buf == ":wq");
        bool save_as   = (m_buf.size() > 3 && m_buf.compare(0, 3, ":w ") == 0);
        bool edit      = (m_buf.size() > 3 && m_buf.compare(0, 3, ":e ") == 0);
        std::string sp = save_as ? m_buf.substr(3) : "";
        std::string ep = edit    ? m_buf.substr(3) : "";

        m_active = false;
        m_buf.clear();

        if (quit      && m_actions.quit)               m_actions.quit();
        if (save      && m_actions.save)               m_actions.save();
        if (save_quit && m_actions.save_quit)          m_actions.save_quit();
        if (save_as && !sp.empty() && m_actions.save_as) m_actions.save_as(sp);
        if (edit    && !ep.empty() && m_actions.edit)    m_actions.edit(ep);
        return true;
    }
    if (e == Event::Backspace) {
        if (m_buf.size() > 1) m_buf.pop_back();
        else { m_active = false; m_buf.clear(); }
        return true;
    }
    if (e.is_character()) {
        m_buf += e.character();
        return true;
    }
    return true;  // swallow everything else while active
}
