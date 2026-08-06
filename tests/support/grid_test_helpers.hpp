#pragma once

#include <string>
#include <vector>

#include "components/grid.hpp"

// Grid owns a worker thread (non-movable), so tests fill an existing instance
// in place rather than constructing it from data.
inline void fill_grid(Grid& g, const std::vector<std::vector<std::string>>& rows) {
    for (int r = 0; r < (int)rows.size(); ++r)
        for (int c = 0; c < (int)rows[r].size(); ++c)
            g.at(r, c).set_value(rows[r][c]);
}
