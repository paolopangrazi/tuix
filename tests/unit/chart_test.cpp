#include <doctest/doctest.h>

#include <numeric>
#include <vector>

#include "components/chart_panel.hpp"

// Pure histogram-binning logic behind the chart panel's Histogram mode.

TEST_CASE("histogram_bins distributes values across equal-width buckets") {
    // 0..9 into 5 bins → 2 per bin.
    std::vector<double> v(10);
    std::iota(v.begin(), v.end(), 0.0);
    auto b = charts::histogram_bins(v, 5);
    REQUIRE(b.size() == 5);
    CHECK(b == std::vector<int>{2, 2, 2, 2, 2});
    // Every value is accounted for.
    CHECK(std::accumulate(b.begin(), b.end(), 0) == 10);
}

TEST_CASE("histogram_bins puts the max in the last bin, not out of range") {
    std::vector<double> v{0, 10, 10, 10};
    auto b = charts::histogram_bins(v, 4);
    REQUIRE(b.size() == 4);
    CHECK(b[0] == 1);   // the 0
    CHECK(b[3] == 3);   // the three 10s land in the top bin
}

TEST_CASE("histogram_bins handles a degenerate (all-equal) range") {
    std::vector<double> v{7, 7, 7};
    auto b = charts::histogram_bins(v, 4);
    CHECK(b[0] == 3);
    CHECK(std::accumulate(b.begin(), b.end(), 0) == 3);
}

TEST_CASE("histogram_bins is safe on empty input and bins < 1") {
    CHECK(charts::histogram_bins({}, 5) == std::vector<int>{0, 0, 0, 0, 0});
    CHECK(charts::histogram_bins({1, 2, 3}, 0).size() == 1);   // clamped to 1 bin
}
