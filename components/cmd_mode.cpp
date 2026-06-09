#include "cmd_mode.hpp"

#include <cctype>
#include <optional>
#include <utility>

#include "col_label.hpp"

using namespace ftxui;

// Parse `s/find/replace/` (any non-alphanumeric delimiter; trailing one optional).
// Returns {find, replace} when the syntax matches and find is non-empty.
static std::optional<std::pair<std::string, std::string>>
parse_subst(const std::string& cmd) {
    if (cmd.size() < 3 || cmd[0] != 's') return std::nullopt;
    char delim = cmd[1];
    if (std::isalnum(static_cast<unsigned char>(delim))) return std::nullopt;

    size_t mid = cmd.find(delim, 2);
    if (mid == std::string::npos) return std::nullopt;       // need a second delimiter
    std::string find = cmd.substr(2, mid - 2);
    if (find.empty()) return std::nullopt;

    size_t end = cmd.find(delim, mid + 1);                   // optional trailing delimiter
    std::string repl = (end == std::string::npos) ? cmd.substr(mid + 1)
                                                  : cmd.substr(mid + 1, end - mid - 1);
    return std::make_pair(find, repl);
}

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
        // A bare A1 address (":B12") jumps the cursor. parse_a1 rejects the
        // command words above (":w", ":wq", ":e file"), so there's no overlap.
        std::string ref = m_buf.substr(1);
        auto subst      = parse_subst(ref);
        // parse_a1 and parse_subst reject the command words above, so no overlap.
        bool is_goto    = !subst && parse_a1(ref).has_value();

        m_active = false;
        m_buf.clear();

        if (quit      && m_actions.quit)               m_actions.quit();
        if (save      && m_actions.save)               m_actions.save();
        if (save_quit && m_actions.save_quit)          m_actions.save_quit();
        if (save_as && !sp.empty() && m_actions.save_as) m_actions.save_as(sp);
        if (edit    && !ep.empty() && m_actions.edit)    m_actions.edit(ep);
        if (subst   && m_actions.replace)              m_actions.replace(subst->first, subst->second);
        if (is_goto && m_actions.goto_cell)            m_actions.goto_cell(ref);
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
