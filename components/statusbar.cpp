#include "statusbar.hpp"

using namespace ftxui;

StatusBar::StatusBar(std::string message) : m_message(std::move(message)) {}

void               StatusBar::set_message(std::string msg) { m_message = std::move(msg); }
const std::string& StatusBar::message()                const { return m_message; }

Element StatusBar::render() const {
    return hbox({ text(m_message) | color(Color::GrayDark), filler() });
}
