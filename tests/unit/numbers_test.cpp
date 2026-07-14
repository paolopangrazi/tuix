#include <doctest/doctest.h>

#include <string>

#include "formulas/value.hpp"
#include "util/numbers.hpp"

// The shared strict numeric parse / mixed compare, and the Value display
// precision they interact with.

TEST_CASE("parse_number accepts whole-string numbers, with whitespace") {
    double d = 0;
    CHECK(tuix::parse_number("42", d));      CHECK(d == 42);
    CHECK(tuix::parse_number(" -3.5 ", d));  CHECK(d == -3.5);
    CHECK(tuix::parse_number("+.5", d));     CHECK(d == 0.5);
    CHECK(tuix::parse_number("5e3", d));     CHECK(d == 5000);
}

TEST_CASE("parse_number rejects prefixes, hex, inf/nan, and text") {
    double d = 0;
    CHECK_FALSE(tuix::parse_number("12abc", d));   // stod would say 12
    CHECK_FALSE(tuix::parse_number("0x10", d));    // stod would say 16
    CHECK_FALSE(tuix::parse_number("inf", d));
    CHECK_FALSE(tuix::parse_number("nan", d));
    CHECK_FALSE(tuix::parse_number("", d));
    CHECK_FALSE(tuix::parse_number("  ", d));
    CHECK_FALSE(tuix::parse_number("1,234", d));
}

TEST_CASE("Value::to_number follows the same strict rules") {
    double d = 0;
    CHECK(Value::string("42").to_number(d));
    CHECK_FALSE(Value::string("12abc").to_number(d));
}

TEST_CASE("cmp_mixed: numbers numerically and before text, text case-insensitive") {
    auto cmp = [](const std::string& a, const std::string& b) {
        double da = 0, db = 0;
        const bool an = tuix::parse_number(a, da), bn = tuix::parse_number(b, db);
        return tuix::cmp_mixed(an, da, a, bn, db, b);
    };
    CHECK(cmp("20", "100") < 0);        // numeric, not lexicographic
    CHECK(cmp("5", "apple") < 0);       // numbers before text
    CHECK(cmp("Banana", "apple") > 0);  // case-insensitive text
    CHECK(cmp("x", "x") == 0);
}

TEST_CASE("to_display keeps 15 significant digits instead of truncating to 6") {
    CHECK(Value::number(1234567.89).to_display() == "1234567.89");
    CHECK(Value::number(0.1 + 0.2).to_display() == "0.3");
    // Integers keep the fast exact path.
    CHECK(Value::number(1234567890.0).to_display() == "1234567890");
}
