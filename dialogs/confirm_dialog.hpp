#pragma once
#include <functional>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

struct Config;

// A generic "Confirm" modal: a caller-supplied message body with Yes/No buttons.
// The dialog never knows what confirming does — the distinct action is injected
// as `on_yes`. The body is a callback so dynamic messages (a filename, a sheet
// name) re-render with current state. `width` caps the modal's WIDTH.
class ConfirmDialog {
public:
    ConfirmDialog(const Config& cfg,
                  int width,
                  std::function<ftxui::Element()> body,
                  std::function<void()> on_yes,
                  std::function<void()> on_close);
    ftxui::Component component();

private:
    const Config& m_cfg;
    int           m_width;
    std::function<ftxui::Element()> m_body;
    std::function<void()>           m_on_close;
    ftxui::Component m_yes, m_no, m_container;
};
