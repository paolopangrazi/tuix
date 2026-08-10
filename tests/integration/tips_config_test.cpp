#include <doctest/doctest.h>

#include <string>

#include <toml++/toml.hpp>

#include "config/config.hpp"
#include "support/config_test_env.hpp"

TEST_CASE("[tips] show_at_startup loads, defaulting to on") {
    tuix::test::ConfigTestEnv env("tips");

    CHECK(env.load("[grid]\ncell_width = 20\n").tips.show_at_startup == true);
    CHECK(env.load("[tips]\nshow_at_startup = false\n").tips.show_at_startup == false);
    CHECK(env.load("[tips]\nshow_at_startup = true\n").tips.show_at_startup == true);
}

// The popup's checkbox writes straight to config.toml; like the F12 editor's
// save it must merge, not rewrite — a user's theme and colors have to survive.
TEST_CASE("save_show_tips merges into the existing config.toml") {
    tuix::test::ConfigTestEnv env("tips");
    Config cfg = env.load(R"CFG(
[theme]
name = "nord"

[colors]
search_bg = "#123456"

[grid]
cell_width = 20
)CFG");
    REQUIRE(cfg.tips.show_at_startup == true);

    REQUIRE(Config::save_show_tips(false));

    toml::table tbl = toml::parse_file(env.file().string());
    CHECK(tbl["tips"]["show_at_startup"].value_or(true) == false);
    CHECK(tbl["theme"]["name"].value_or(std::string{}) == "nord");
    CHECK(tbl["colors"]["search_bg"].value_or(std::string{}) == "#123456");
    CHECK(tbl["grid"]["cell_width"].value_or(0) == 20);

    Config reloaded = Config::load();
    CHECK(reloaded.tips.show_at_startup == false);
    CHECK(reloaded.theme.name == "nord");
    CHECK(reloaded.grid.cell_width == 20);

    // And back on again, so re-checking the box is not a one-way door.
    REQUIRE(Config::save_show_tips(true));
    CHECK(Config::load().tips.show_at_startup == true);
}
