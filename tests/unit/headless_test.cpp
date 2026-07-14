#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "cli/headless.hpp"
#include "loader/csv_loader.hpp"

// Exercises the headless "unix filter" transform pipeline (filter → sort →
// head/tail → select) on in-memory SheetData, plus argv parsing.

using headless::Options;
using headless::Predicate;
using Op = headless::Predicate::Op;

namespace {

SheetData sample() {
    SheetData d;
    d.headers = {"name", "dept", "salary"};
    d.rows = {
        {"Anna",  "Engineering", "95000"},
        {"Carlos","Engineering", "62000"},
        {"Diana", "Product",     "88000"},
        {"Ethan", "Design",      "74000"},
        {"Fiona", "Engineering", "112000"},
    };
    return d;
}

std::vector<std::string> col(const SheetData& d, int c) {
    std::vector<std::string> out;
    for (const auto& r : d.rows) out.push_back(r[c]);
    return out;
}

}  // namespace

TEST_CASE("filter compares numerically by column name") {
    SheetData d = sample();
    Options o;
    o.filters = {{"salary", Op::GT, "85000"}};
    std::string err;
    REQUIRE(headless::apply(d, o, err));
    CHECK(col(d, 0) == std::vector<std::string>{"Anna", "Diana", "Fiona"});
}

TEST_CASE("filters AND together and resolve columns by letter") {
    SheetData d = sample();
    Options o;
    o.filters = {{"B", Op::EQ, "Engineering"}, {"salary", Op::LT, "100000"}};
    std::string err;
    REQUIRE(headless::apply(d, o, err));
    CHECK(col(d, 0) == std::vector<std::string>{"Anna", "Carlos"});
}

TEST_CASE("contains is case-insensitive; regex is anchored ECMAScript") {
    SheetData d = sample();
    Options o;
    o.filters = {{"dept", Op::CONTAINS, "eng"}};
    std::string err;
    REQUIRE(headless::apply(d, o, err));
    CHECK(d.rows.size() == 3);

    SheetData d2 = sample();
    Options o2;
    o2.filters = {{"name", Op::RE, "^[AF]"}};
    REQUIRE(headless::apply(d2, o2, err));
    CHECK(col(d2, 0) == std::vector<std::string>{"Anna", "Fiona"});
}

TEST_CASE("sort is numeric-aware and honors direction and multiple keys") {
    SheetData d = sample();
    Options o;
    o.sort_spec = "dept,salary desc";
    std::string err;
    REQUIRE(headless::apply(d, o, err));
    CHECK(col(d, 1) == std::vector<std::string>{
        "Design", "Engineering", "Engineering", "Engineering", "Product"});
    // Within Engineering, salary descending.
    CHECK(col(d, 2) == std::vector<std::string>{
        "74000", "112000", "95000", "62000", "88000"});
}

TEST_CASE("head then select trims rows and reorders columns") {
    SheetData d = sample();
    Options o;
    o.sort_spec = "salary desc";
    o.head      = 2;
    o.select    = "salary,name";
    std::string err;
    REQUIRE(headless::apply(d, o, err));
    CHECK(d.headers == std::vector<std::string>{"salary", "name"});
    CHECK(col(d, 0) == std::vector<std::string>{"112000", "95000"});
    CHECK(col(d, 1) == std::vector<std::string>{"Fiona", "Anna"});
}

TEST_CASE("tail keeps the last N rows") {
    SheetData d = sample();
    Options o;
    o.tail = 2;
    std::string err;
    REQUIRE(headless::apply(d, o, err));
    CHECK(col(d, 0) == std::vector<std::string>{"Ethan", "Fiona"});
}

TEST_CASE("unknown column is a reported error") {
    SheetData d = sample();
    Options o;
    o.filters = {{"missing", Op::EQ, "x"}};
    std::string err;
    CHECK_FALSE(headless::apply(d, o, err));
    CHECK(err.find("missing") != std::string::npos);
}

TEST_CASE("parse_args builds filters and flags input as headless") {
    const char* argv[] = {"tuix", "--filter", "salary > 50000",
                          "--sort", "dept", "--select", "name,dept",
                          "in.csv"};
    Options o;
    const bool active = headless::parse_args(8, const_cast<char**>(argv), o);
    CHECK(active);
    CHECK(o.parse_error.empty());
    REQUIRE(o.filters.size() == 1);
    CHECK(o.filters[0].col == "salary");
    CHECK(o.filters[0].op == Op::GT);
    CHECK(o.filters[0].rhs == "50000");
    CHECK(o.sort_spec == "dept");
    CHECK(o.select == "name,dept");
    CHECK(o.input == "in.csv");
}

TEST_CASE("parse_args rejects a predicate with no operator") {
    const char* argv[] = {"tuix", "--filter", "salary"};
    Options o;
    headless::parse_args(3, const_cast<char**>(argv), o);
    CHECK_FALSE(o.parse_error.empty());
}
