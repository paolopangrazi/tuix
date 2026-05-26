#pragma once
#include <ftxui/component/component.hpp>
#include "grid.hpp"
#include "config/config.hpp"

class Body {
public:
    Body(int rows, int cols, const Config& cfg = {});

    Grid&       grid();
    const Grid& grid() const;

    ftxui::Component make_component();

private:
    Grid m_grid;
};
