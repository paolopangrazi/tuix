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

TEST_CASE("parse_args rejects non-numeric and negative --head/--tail counts") {
    const char* bad_text[] = {"tuix", "--head", "xyz"};
    Options o1;
    headless::parse_args(3, const_cast<char**>(bad_text), o1);
    CHECK_FALSE(o1.parse_error.empty());

    const char* bad_neg[] = {"tuix", "--tail", "-2"};
    Options o2;
    headless::parse_args(3, const_cast<char**>(bad_neg), o2);
    CHECK_FALSE(o2.parse_error.empty());

    const char* good[] = {"tuix", "--head", "10"};
    Options o3;
    headless::parse_args(3, const_cast<char**>(good), o3);
    CHECK(o3.parse_error.empty());
    CHECK(o3.head == 10);
}

TEST_CASE("parse_args rejects multiple input files") {
    const char* argv[] = {"tuix", "--head", "5", "a.csv", "b.csv"};
    Options o;
    headless::parse_args(5, const_cast<char**>(argv), o);
    CHECK(o.parse_error.find("multiple input") != std::string::npos);
}

TEST_CASE("parse_args captures --sheet") {
    const char* argv[] = {"tuix", "--sheet", "Sales", "book.xlsx"};
    Options o;
    headless::parse_args(4, const_cast<char**>(argv), o);
    CHECK(o.parse_error.empty());
    CHECK(o.sheet == "Sales");
    CHECK(o.input == "book.xlsx");

    const char* missing[] = {"tuix", "--sheet"};
    Options o2;
    headless::parse_args(2, const_cast<char**>(missing), o2);
    CHECK_FALSE(o2.parse_error.empty());
}

TEST_CASE("parse_args captures --version and it wins over a bad flag combo") {
    for (const char* flag : {"--version", "-V"}) {
        const char* argv[] = {"tuix", flag};
        Options o;
        CHECK(headless::parse_args(2, const_cast<char**>(argv), o));  // headless, not the TUI
        CHECK(o.show_version);
        CHECK(o.parse_error.empty());
    }
}

namespace {
WorkbookData workbook() {
    WorkbookData wb;
    wb.sheets.emplace_back("Summary", SheetData{});
    wb.sheets.emplace_back("Sales",   SheetData{});
    wb.sheets.emplace_back("Costs",   SheetData{});
    return wb;
}
}  // namespace

TEST_CASE("resolve_sheet: empty selects the first sheet") {
    auto idx = headless::resolve_sheet(workbook(), "");
    REQUIRE(idx.has_value());
    CHECK(*idx == 0);
}

TEST_CASE("resolve_sheet: matches sheet name case-insensitively") {
    auto idx = headless::resolve_sheet(workbook(), "sALes");
    REQUIRE(idx.has_value());
    CHECK(*idx == 1);
}

TEST_CASE("resolve_sheet: accepts a 1-based index") {
    auto idx = headless::resolve_sheet(workbook(), "3");
    REQUIRE(idx.has_value());
    CHECK(*idx == 2);
}

TEST_CASE("resolve_sheet: rejects unknown name and out-of-range index") {
    CHECK_FALSE(headless::resolve_sheet(workbook(), "Nope").has_value());
    CHECK_FALSE(headless::resolve_sheet(workbook(), "0").has_value());
    CHECK_FALSE(headless::resolve_sheet(workbook(), "4").has_value());
    CHECK_FALSE(headless::resolve_sheet(WorkbookData{}, "").has_value());
}

TEST_CASE("resolve_sheet: a numeric sheet name wins over index interpretation") {
    WorkbookData wb;
    wb.sheets.emplace_back("2024", SheetData{});
    wb.sheets.emplace_back("2025", SheetData{});
    auto idx = headless::resolve_sheet(wb, "2025");  // name, not index 2025
    REQUIRE(idx.has_value());
    CHECK(*idx == 1);
}
