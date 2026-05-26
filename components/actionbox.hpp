#pragma once
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

class ActionBox {
public:
    static constexpr int k_width = 3;

    ftxui::Element render(bool active = false) const;
    void set_hovered(bool h);
    bool is_hovered()        const;
    bool contains_cell (int x, int y) const;
    bool contains_plus (int x, int y) const;
    bool contains_minus(int x, int y) const;
    ftxui::Box& cell_box() const;

private:
    bool               m_hovered   = false;
    mutable ftxui::Box m_cell_box  = {};
    mutable ftxui::Box m_plus_box  = {};
    mutable ftxui::Box m_minus_box = {};
};
