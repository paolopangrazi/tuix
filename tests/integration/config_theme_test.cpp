#include <doctest/doctest.h>

#include <string>

#include <ftxui/screen/color.hpp>

#include "config/config.hpp"
#include "support/config_test_env.hpp"

using ftxui::Color;

TEST_CASE("color parser accepts hex, rgb(), names and palette indices") {
    tuix::test::ConfigTestEnv env("theme");
    Config c = env.load(R"CFG(
[colors]
cursor_bg    = "#ff8800"
selection_bg = "rgb(10, 20, 30)"
header       = "#fff"
dimmed       = "magenta"
row_number   = 4
)CFG");
    CHECK(c.colors.cursor_bg    == Color::RGB(0xff, 0x88, 0x00));
    CHECK(c.colors.selection_bg == Color::RGB(10, 20, 30));
    CHECK(c.colors.header       == Color::RGB(0xff, 0xff, 0xff));  // #fff → #ffffff
    CHECK(c.colors.dimmed       == Color::Magenta);
    CHECK(c.colors.row_number   == Color(Color::Palette16(4)));    // Blue
}

TEST_CASE("a named theme becomes the base palette") {
    tuix::test::ConfigTestEnv env("theme");
    Config c = env.load("[theme]\nname = \"tokyo-night\"\n");
    CHECK(c.theme.name    == "tokyo-night");
    CHECK(c.colors.header == Color::RGB(0x7d, 0xcf, 0xff));
    CHECK(c.colors.grid_bg == Color::RGB(0x1a, 0x1b, 0x26));
}

TEST_CASE("[colors] overrides win over the [theme] preset") {
    tuix::test::ConfigTestEnv env("theme");
    Config c = env.load(R"CFG(
[theme]
name = "tokyo-night"
[colors]
header = "#ff0000"
)CFG");
    CHECK(c.colors.header  == Color::RGB(0xff, 0x00, 0x00));        // override wins
    CHECK(c.colors.grid_bg == Color::RGB(0x1a, 0x1b, 0x26));        // theme still applies elsewhere
}

TEST_CASE("default theme keeps terminal-native ANSI colors") {
    tuix::test::ConfigTestEnv env("theme");
    Config c = env.load("[theme]\nname = \"default\"\n");
    CHECK(c.colors.header  == Color::Green);    // palette name → terminal palette
    CHECK(c.colors.grid_bg == Color::Default);  // transparent canvas
}

TEST_CASE("theme toggles and bad color input fall back") {
    tuix::test::ConfigTestEnv env("theme");
    Config c = env.load(R"CFG(
[theme]
name = "nord"
animations = true
zebra = false
[colors]
cursor_bg = "not-a-color"
)CFG");
    CHECK(c.theme.animations == true);
    CHECK(c.theme.zebra      == false);
    // Unparseable string → keep whatever the theme set (Nord's cursor_bg).
    CHECK(c.colors.cursor_bg == Color::RGB(0x88, 0xc0, 0xd0));
}
