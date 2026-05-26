#include "body.hpp"

Body::Body(int rows, int cols, const Config& cfg) : m_grid(rows, cols, cfg) {}

Grid&       Body::grid()       { return m_grid; }
const Grid& Body::grid() const { return m_grid; }

ftxui::Component Body::make_component() {
    return m_grid.make_component();
}
