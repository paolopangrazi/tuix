#pragma once
#include <functional>
#include <string>

#include <ftxui/component/component.hpp>

struct Config;

// The "Did you know?" popup: one hint at a time from tuix::tips(), navigated
// with ← / → (or the ◀ ▶ buttons), plus a checkbox that decides whether it
// shows again at startup. Opened by main() on launch and re-openable with F3.
class TipsDialog {
public:
    TipsDialog(const Config& cfg, std::function<void()> on_close);

    // Picks a fresh tip to open on — call whenever the dialog is (re)opened.
    // The checkbox keeps whatever the user last set it to this session.
    void reset();

    ftxui::Component component();

    // Exposed for the callers that need the state without going through the
    // component (main() decides whether to open on startup).
    bool show_at_startup() const { return m_show_at_startup; }

private:
    void close();      // persist a changed checkbox, then hand back to the caller
    void step(int d);  // move `d` tips forward/backward, wrapping

    const Config& m_cfg;
    std::function<void()> m_on_close;

    int  m_index            = 0;
    bool m_show_at_startup  = true;   // bound to the checkbox
    bool m_saved_preference = true;   // last value written to config.toml
    std::string m_status;             // shown when saving the preference fails

    ftxui::Component m_prev, m_next, m_checkbox, m_close_btn, m_container;
};
