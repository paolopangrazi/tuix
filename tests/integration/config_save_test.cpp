#include <doctest/doctest.h>

#include <string>
#include <vector>

#include <toml++/toml.hpp>

#include "config/config.hpp"
#include "config/config_schema.hpp"
#include "config_dialog.hpp"
#include "support/config_test_env.hpp"

// Regression: saving from the F12 editor used to rebuild config.toml from a
// hard-coded subset, silently dropping [theme] and every slot the dialog
// didn't know about. Save must merge into the existing file instead.
TEST_CASE("config editor save preserves [theme] and unrepresentable colors") {
    tuix::test::ConfigTestEnv env("save");
    Config cfg = env.load(R"CFG(
[theme]
name  = "gruvbox"
zebra = false

[colors]
search_bg = "#123456"

[keys]
chart = ["X"]
)CFG");
    REQUIRE(cfg.theme.name == "gruvbox");

    ConfigDialog dlg(cfg, [] {});
    dlg.save_to_file();

    toml::table tbl = toml::parse_file(env.file().string());

    // [theme] is not managed by the editor — it must survive untouched.
    CHECK(tbl["theme"]["name"].value_or(std::string{}) == "gruvbox");
    CHECK(tbl["theme"]["zebra"].value_or(true) == false);

    // search_bg is an RGB value with no ANSI name: the dialog shows it blank
    // and must therefore leave the file's value alone.
    CHECK(tbl["colors"]["search_bg"].value_or(std::string{}) == "#123456");

    // A managed key binding round-trips through the editor unchanged.
    auto* chart = tbl["keys"]["chart"].as_array();
    REQUIRE(chart != nullptr);
    REQUIRE(chart->size() == 1);
    CHECK((*chart)[0].value_or(std::string{}) == "X");

    // Reloading the saved file yields the same effective config.
    Config reloaded = Config::load();
    CHECK(reloaded.theme.name == "gruvbox");
    CHECK(reloaded.keys.chart == std::vector<char>{'X'});
}

// The schema is the single list both the loader and the editor iterate; a slot
// present there must actually load. (Guards against the old drift where newer
// keys existed in Config but were never parsed or shown.)
TEST_CASE("every schema key slot is loaded from config.toml") {
    std::string body = "[keys]\n";
    for (int i = 0; i < k_key_slot_count; ++i)
        body += std::string(k_key_slots[i].key) + " = [\"Z\"]\n";

    tuix::test::ConfigTestEnv env("save");
    Config cfg = env.load(body);
    for (int i = 0; i < k_key_slot_count; ++i)
        CHECK(cfg.keys.*k_key_slots[i].member == std::vector<char>{'Z'});
}
