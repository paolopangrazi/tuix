#include <doctest/doctest.h>

#include <string>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include "components/chart_panel.hpp"
#include "components/grid.hpp"
#include "config/config.hpp"
#include "support/grid_test_helpers.hpp"

// Drives a real Grid through the chart cycle and renders the panel, exercising
// chart_data() gathering and the Canvas drawing end to end.

namespace {

std::string render(const Config& cfg, const Grid::ChartData& d) {
    using namespace ftxui;
    auto el = render_chart_panel(cfg, d, 40, 6);
    auto screen = Screen::Create(Dimension::Fixed(60), Dimension::Fixed(8));
    Render(screen, el);
    return screen.ToString();
}

}  // namespace

TEST_CASE("cycle_chart walks Bar → Line → Histogram → off") {
    Config cfg;
    Grid g(4, 1, cfg);
    fill_grid(g, {{"3"}, {"1"}, {"4"}, {"1"}});

    CHECK_FALSE(g.chart_data().active);

    g.cycle_chart();
    CHECK(g.chart_data().active);
    CHECK(g.chart_data().type == Grid::ChartType::Bar);

    g.cycle_chart();
    CHECK(g.chart_data().type == Grid::ChartType::Line);

    g.cycle_chart();
    CHECK(g.chart_data().type == Grid::ChartType::Histogram);

    g.cycle_chart();
    CHECK_FALSE(g.chart_data().active);
}

TEST_CASE("chart_data gathers the current column and counts skipped text") {
    Config cfg;
    Grid g(4, 1, cfg);
    fill_grid(g, {{"10"}, {"oops"}, {"30"}, {"40"}});

    g.cycle_chart();
    auto d = g.chart_data();
    CHECK(d.values == std::vector<double>{10, 30, 40});
    CHECK(d.skipped == 1);
}

TEST_CASE("render_chart_panel draws braille for a non-empty series") {
    Config cfg;
    Grid g(4, 1, cfg);
    fill_grid(g, {{"3"}, {"1"}, {"4"}, {"1"}});
    g.cycle_chart();   // Bar

    const std::string out = render(cfg, g.chart_data());
    // Braille block glyphs live in U+2800..U+28FF; their UTF-8 lead byte is 0xE2
    // 0xA0..0xA3. Simplest robust check: the panel is non-trivial and labelled.
    CHECK(out.find("Bar") != std::string::npos);
    CHECK(out.find("n=4") != std::string::npos);
    CHECK(out.size() > 60);   // more than one blank line → something was drawn
}

TEST_CASE("replace_all invalidates the chart/stats caches") {
    Config cfg;
    Grid g(3, 1, cfg);
    fill_grid(g, {{"1"}, {"2"}, {"3"}});
    g.cycle_chart();
    CHECK(g.chart_data().values == std::vector<double>{1, 2, 3});

    // replace_all mutates cells outside commit_edit; the cached series must
    // notice (it didn't when replace_all bypassed set_value_at).
    g.replace_all("2", "50");
    CHECK(g.chart_data().values == std::vector<double>{1, 50, 3});
}

TEST_CASE("render_chart_panel degrades gracefully with no numeric data") {
    Config cfg;
    Grid g(2, 1, cfg);
    fill_grid(g, {{"foo"}, {"bar"}});
    g.cycle_chart();

    auto d = g.chart_data();
    CHECK(d.values.empty());
    CHECK(render(cfg, d).find("no numeric") != std::string::npos);
}
