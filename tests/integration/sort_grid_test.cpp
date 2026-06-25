#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "components/grid.hpp"
#include "config/config.hpp"

// Exercises Grid::sort_by / sort_spec on the live grid: typed comparison,
// multi-key, stability, blanks-last, and undo/redo of the reorder.

namespace {

// Read column `c` top-to-bottom as displayed strings.
std::vector<std::string> column(Grid& g, int c) {
    std::vector<std::string> out;
    for (int r = 0; r < g.rows(); ++r) out.push_back(g.cell_value(r, c).to_display());
    return out;
}

// Grid owns a worker thread (non-movable), so fill an existing one in place.
void fill(Grid& g, const std::vector<std::vector<std::string>>& rows) {
    for (int r = 0; r < (int)rows.size(); ++r)
        for (int c = 0; c < (int)rows[r].size(); ++c)
            g.at(r, c).set_value(rows[r][c]);
}

}  // namespace

TEST_CASE("sort_by orders a column numerically, not lexicographically") {
    Config cfg;
    Grid g(4, 1, cfg);
    fill(g, {{"3"}, {"100"}, {"20"}, {"1"}});

    g.sort_by({{0, /*descending=*/false}});
    CHECK(column(g, 0) == std::vector<std::string>{"1", "3", "20", "100"});

    g.sort_by({{0, /*descending=*/true}});
    CHECK(column(g, 0) == std::vector<std::string>{"100", "20", "3", "1"});
}

TEST_CASE("sort_by compares text case-insensitively and puts blanks last") {
    Config cfg;
    Grid g(4, 1, cfg);
    fill(g, {{"banana"}, {""}, {"Apple"}, {"cherry"}});

    g.sort_by({{0, false}});
    CHECK(column(g, 0) == std::vector<std::string>{"Apple", "banana", "cherry", ""});

    // Blanks stay last even descending.
    g.sort_by({{0, true}});
    CHECK(column(g, 0) == std::vector<std::string>{"cherry", "banana", "Apple", ""});
}

TEST_CASE("multi-key sort and stability") {
    Config cfg;
    // (dept, name): primary dept asc, secondary name asc.
    Grid g(4, 2, cfg);
    fill(g, {
        {"B", "Zoe"},
        {"A", "Tom"},
        {"B", "Amy"},
        {"A", "Eve"},
    });

    g.sort_by({{0, false}, {1, false}});
    CHECK(column(g, 0) == std::vector<std::string>{"A", "A", "B", "B"});
    CHECK(column(g, 1) == std::vector<std::string>{"Eve", "Tom", "Amy", "Zoe"});

    // Single-key sort on dept is stable: equal depts keep their current order.
    g.sort_by({{0, false}});
    CHECK(column(g, 1) == std::vector<std::string>{"Eve", "Tom", "Amy", "Zoe"});
}

TEST_CASE("sort carries whole rows and is undoable") {
    Config cfg;
    Grid g(3, 2, cfg);
    fill(g, {{"3", "c"}, {"1", "a"}, {"2", "b"}});

    g.sort_by({{0, false}});
    CHECK(column(g, 0) == std::vector<std::string>{"1", "2", "3"});
    CHECK(column(g, 1) == std::vector<std::string>{"a", "b", "c"});   // row pairing preserved

    g.undo();
    CHECK(column(g, 0) == std::vector<std::string>{"3", "1", "2"});
    CHECK(column(g, 1) == std::vector<std::string>{"c", "a", "b"});

    g.redo();
    CHECK(column(g, 0) == std::vector<std::string>{"1", "2", "3"});
    CHECK(column(g, 1) == std::vector<std::string>{"a", "b", "c"});
}

TEST_CASE("sort_spec parses a column label and direction") {
    Config cfg;
    Grid g(3, 2, cfg);
    fill(g, {{"x", "3"}, {"y", "1"}, {"z", "2"}});

    g.sort_spec("B desc");   // sort by the second column, descending
    CHECK(column(g, 1) == std::vector<std::string>{"3", "2", "1"});
    CHECK(column(g, 0) == std::vector<std::string>{"x", "z", "y"});
}
