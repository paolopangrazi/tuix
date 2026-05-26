#include "actionbox.hpp"

using namespace ftxui;

Element ActionBox::render(bool active) const {
    const bool show = m_hovered || active;
    Element p = text(show ? "+" : " ") | reflect(m_plus_box);
    Element m = text(show ? "-" : " ") | reflect(m_minus_box);
    if (show) {
        p = std::move(p) | bold | color(Color::Green);
        m = std::move(m) | bold | color(Color::Red);
    }
    return hbox({ std::move(p), text(" "), std::move(m) });
}

void ActionBox::set_hovered(bool h) { m_hovered = h; }
bool ActionBox::is_hovered()  const { return m_hovered; }
ftxui::Box& ActionBox::cell_box() const { return m_cell_box; }

bool ActionBox::contains_cell(int x, int y) const {
    return x >= m_cell_box.x_min && x <= m_cell_box.x_max &&
           y >= m_cell_box.y_min && y <= m_cell_box.y_max;
}

bool ActionBox::contains_plus(int x, int y) const {
    return x >= m_plus_box.x_min && x <= m_plus_box.x_max &&
           y >= m_plus_box.y_min && y <= m_plus_box.y_max;
}

bool ActionBox::contains_minus(int x, int y) const {
    return x >= m_minus_box.x_min && x <= m_minus_box.x_max &&
           y >= m_minus_box.y_min && y <= m_minus_box.y_max;
}
