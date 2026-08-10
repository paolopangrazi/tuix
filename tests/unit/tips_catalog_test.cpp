#include <doctest/doctest.h>

#include <string>

#include "tips_catalog.hpp"

// The popup indexes into this list and renders every field, so an empty
// catalog (or a half-filled entry) would show up as a blank card at startup.
TEST_CASE("tip catalog entries are all renderable") {
    const auto& tips = tuix::tips();
    REQUIRE(!tips.empty());

    for (const auto& tip : tips) {
        REQUIRE(tip.title != nullptr);
        REQUIRE(tip.keys  != nullptr);   // may be empty, but never null
        REQUIRE(tip.body  != nullptr);
        CHECK(std::string(tip.title).size() > 0);
        CHECK(std::string(tip.body).size() > 0);
    }
}
