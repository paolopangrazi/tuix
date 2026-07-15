#include "cmd_mode.hpp"

#include <cctype>
#include <optional>
#include <utility>

#include "util/col_label.hpp"

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
        bool quit       = (m_buf == ":q");
        bool force_quit = (m_buf == ":q!");
        bool save       = (m_buf == ":w");
        bool save_quit  = (m_buf == ":wq");
        bool save_as    = (m_buf.size() > 3 && m_buf.compare(0, 3, ":w ") == 0);
        bool edit       = (m_buf.size() > 3 && m_buf.compare(0, 3, ":e ") == 0);
        bool sort       = (m_buf == ":sort" || (m_buf.size() > 6 && m_buf.compare(0, 6, ":sort ") == 0));
        std::string sp = save_as ? m_buf.substr(3) : "";
        std::string ep = edit    ? m_buf.substr(3) : "";
        std::string srt = sort && m_buf.size() > 6 ? m_buf.substr(6) : "";
        // A bare A1 address (":B12") jumps the cursor; `:s/old/new/` replaces.
        // parse_a1 and parse_subst both reject the command words above, so the
        // forms can't collide.
        std::string ref = m_buf.substr(1);
        auto subst      = parse_subst(ref);
        bool is_goto    = !subst && parse_a1(ref).has_value();
        const std::string entered = m_buf;

        m_active = false;
        m_buf.clear();

        bool handled = false;
        auto run = [&](bool match, auto& fn, auto&&... args) {
            if (match) { handled = true; if (fn) fn(std::forward<decltype(args)>(args)...); }
        };
        run(quit,                   m_actions.quit);
        run(force_quit,             m_actions.force_quit);
        run(save,                   m_actions.save);
        run(save_quit,              m_actions.save_quit);
        run(save_as && !sp.empty(), m_actions.save_as, sp);
        run(edit && !ep.empty(),    m_actions.edit, ep);
        run(subst.has_value(),      m_actions.replace,
            subst ? subst->first : "", subst ? subst->second : "");
        run(sort,                   m_actions.sort, srt);
        run(is_goto,                m_actions.goto_cell, ref);
        if (!handled && entered.size() > 1 && m_actions.unknown)
            m_actions.unknown(entered);
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
