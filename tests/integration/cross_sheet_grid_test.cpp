#include <doctest/doctest.h>

#include <cctype>
#include <string>
#include <vector>

#include "components/grid.hpp"
#include "components/sheet.hpp"
#include "config/config.hpp"

// These exercise the live Grid path for cross-sheet references: the active
// sheet resolves against the grid's own cells, while qualified references to
// other sheets go through the Sheet lookup and a read-only SheetView.

namespace {

bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    return true;
}

Sheet make_sheet(std::string name, int rows, int cols) {
    Sheet s;
    s.name = std::move(name);
    s.cells.assign(rows, std::vector<Cell>(cols));
    s.col_names.resize(cols);
    s.col_widths.assign(cols, 12);
    return s;
}

}  // namespace

TEST_CASE("live grid resolves references to another sheet") {
    Config cfg;
    Sheet sheet1 = make_sheet("Sheet1", 5, 5);
    Sheet sheet2 = make_sheet("Sheet2", 5, 5);
    sheet2.cells[0][0].set_value("10");          // Sheet2!A1 = 10
    sheet2.cells[1][0].set_value("20");          // Sheet2!A2 = 20
    sheet2.cells[2][0].set_value("=A1+A2");      // Sheet2!A3 = formula within Sheet2

    Grid g(5, 5, cfg);
    g.set_sheet_lookup([&](const std::string& n) -> const Sheet* {
        if (iequals(n, "Sheet1")) return &sheet1;
        if (iequals(n, "Sheet2")) return &sheet2;
        return nullptr;
    });
    g.load_from(sheet1);   // active sheet = Sheet1

    g.at(0, 0).set_value("=Sheet2!A1");      // → 10
    g.at(0, 1).set_value("=Sheet2!A3");      // → Sheet2's own formula, 30
    g.at(0, 2).set_value("=SUM(Sheet2!A1:A2)"); // → 30
    g.at(0, 3).set_value("=ghost!A1");       // unknown sheet → #REF!

    CHECK(g.cell_value(0, 0).to_display() == "10");
    CHECK(g.cell_value(0, 1).to_display() == "30");
    CHECK(g.cell_value(0, 2).to_display() == "30");
    CHECK(g.cell_value(0, 3).to_display() == "#REF!");
}

TEST_CASE("self-qualified references read the live active sheet, not the snapshot") {
    Config cfg;
    Sheet sheet1 = make_sheet("Sheet1", 5, 5);   // snapshot is empty

    Grid g(5, 5, cfg);
    g.set_sheet_lookup([&](const std::string& n) -> const Sheet* {
        return iequals(n, "Sheet1") ? &sheet1 : nullptr;
    });
    g.load_from(sheet1);

    g.at(0, 3).set_value("100");             // D1, only in the live grid
    g.at(1, 3).set_value("=Sheet1!D1 + 5");  // qualified ref to the active sheet

    CHECK(g.cell_value(1, 3).to_display() == "105");  // sees live edit, not empty snapshot
}

TEST_CASE("cross-sheet cycles are caught") {
    Config cfg;
    Sheet sheet1 = make_sheet("Sheet1", 3, 3);
    Sheet sheet2 = make_sheet("Sheet2", 3, 3);
    sheet2.cells[0][0].set_value("=Sheet1!A1");  // Sheet2!A1 → Sheet1!A1

    Grid g(3, 3, cfg);
    g.set_sheet_lookup([&](const std::string& n) -> const Sheet* {
        if (iequals(n, "Sheet1")) return &sheet1;
        if (iequals(n, "Sheet2")) return &sheet2;
        return nullptr;
    });
    g.load_from(sheet1);

    g.at(0, 0).set_value("=Sheet2!A1");  // Sheet1!A1 → Sheet2!A1 → back to Sheet1!A1
    CHECK(g.cell_value(0, 0).to_display() == "#REF!");
}
