#pragma once
#include <functional>
#include <string>
#include <ftxui/component/event.hpp>

// Vim-style `:` command line. Owns the active flag and typed buffer, parses
// the ex-commands on Enter, and dispatches to the supplied actions.
class CmdMode {
public:
    struct Actions {
        std::function<void()> quit;       // :q  :q!
        std::function<void()> save;       // :w
        std::function<void()> save_quit;  // :wq
        std::function<void(const std::string&)> save_as; // :w <path>
        std::function<void(const std::string&)> edit;    // :e <path>
        std::function<void(const std::string&)> goto_cell; // :B12  (bare A1 ref)
        std::function<void(const std::string&, const std::string&)> replace; // :s/old/new/
        std::function<void(const std::string&)> sort;    // :sort [col] [asc|desc], …
    };

    explicit CmdMode(Actions actions);

    bool               is_active() const noexcept { return m_active; }
    const std::string& buffer()    const noexcept { return m_buf; }

    void enter();                       // begin a fresh ":" command
    bool handle(ftxui::Event e);        // consume keys while active; false when idle

private:
    Actions     m_actions;
    bool        m_active = false;
    std::string m_buf;
};
