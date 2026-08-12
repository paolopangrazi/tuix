#include <doctest/doctest.h>

#include <sstream>
#include <string>

#include "cli/sheet_format.hpp"
#include "loader/csv_loader.hpp"

// Exercises the headless output encodings (cli/sheet_format.cpp): format
// selection from a name or an -o extension, and the serialization of each one.

using headless::Format;

namespace {

SheetData sample() {
    SheetData d;
    d.headers = {"name", "salary"};
    d.rows    = {{"Anna", "95000"}, {"Carlos", "62000"}};
    return d;
}

std::string render(const SheetData& d, Format fmt, bool include_header = true) {
    std::ostringstream out;
    headless::write_sheet(out, d, fmt, include_header);
    return out.str();
}

}  // namespace

TEST_CASE("parse_format accepts the documented names and aliases") {
    CHECK(headless::parse_format("csv")      == Format::CSV);
    CHECK(headless::parse_format("TSV")      == Format::TSV);
    CHECK(headless::parse_format("json")     == Format::JSON);
    CHECK(headless::parse_format("ndjson")   == Format::JSONL);
    CHECK(headless::parse_format("markdown") == Format::MD);
    CHECK(headless::parse_format("table")    == Format::TABLE);
    CHECK_FALSE(headless::parse_format("yaml").has_value());
}

TEST_CASE("format_from_extension reads the output path, and stays quiet otherwise") {
    CHECK(headless::format_from_extension("out.tsv")   == Format::TSV);
    CHECK(headless::format_from_extension("out.JSONL") == Format::JSONL);
    CHECK(headless::format_from_extension("out.md")    == Format::MD);
    CHECK_FALSE(headless::format_from_extension("out.txt").has_value());
    CHECK_FALSE(headless::format_from_extension("-").has_value());
    CHECK_FALSE(headless::format_from_extension("").has_value());
}

TEST_CASE("csv keeps the sheet delimiter; tsv forces tabs") {
    SheetData d = sample();
    d.delimiter = ';';
    CHECK(render(d, Format::CSV)  == "name;salary\nAnna;95000\nCarlos;62000\n");
    CHECK(render(d, Format::TSV)  == "name\tsalary\nAnna\t95000\nCarlos\t62000\n");
}

TEST_CASE("include_header=false drops the header line for the row formats") {
    CHECK(render(sample(), Format::CSV, false)   == "Anna,95000\nCarlos,62000\n");
    CHECK(render(sample(), Format::TABLE, false) == "Anna    95000\nCarlos  62000\n");
}

TEST_CASE("json emits an array of objects with numbers unquoted") {
    CHECK(render(sample(), Format::JSON) ==
          "[\n"
          "  {\"name\": \"Anna\", \"salary\": 95000},\n"
          "  {\"name\": \"Carlos\", \"salary\": 62000}\n"
          "]\n");
}

TEST_CASE("jsonl emits one object per line with no array or commas") {
    CHECK(render(sample(), Format::JSONL) ==
          "{\"name\": \"Anna\", \"salary\": 95000}\n"
          "{\"name\": \"Carlos\", \"salary\": 62000}\n");
}

TEST_CASE("json quotes anything that is not exactly a JSON number") {
    SheetData d;
    d.headers = {"a"};
    d.rows    = {{"007"}, {"1,200"}, {"12abc"}, {"1e999"}, {"+5"}, {".5"},
                 {"-1.5e3"}, {"0"}, {""}};
    const std::string out = render(d, Format::JSONL);
    CHECK(out ==
          "{\"a\": \"007\"}\n"
          "{\"a\": \"1,200\"}\n"
          "{\"a\": \"12abc\"}\n"
          "{\"a\": \"1e999\"}\n"      // overflows to inf — not a usable number
          "{\"a\": \"+5\"}\n"          // leading + is not JSON
          "{\"a\": \".5\"}\n"          // bare fraction is not JSON
          "{\"a\": -1.5e3}\n"
          "{\"a\": 0}\n"
          "{\"a\": \"\"}\n");
}

TEST_CASE("json escapes quotes, backslashes and control characters") {
    SheetData d;
    d.headers = {"a"};
    d.rows    = {{"he said \"hi\""}, {"C:\\tmp"}, {"two\nlines"}, {std::string("\x01")}};
    CHECK(render(d, Format::JSONL) ==
          "{\"a\": \"he said \\\"hi\\\"\"}\n"
          "{\"a\": \"C:\\\\tmp\"}\n"
          "{\"a\": \"two\\nlines\"}\n"
          "{\"a\": \"\\u0001\"}\n");
}

TEST_CASE("json keys fall back to column letters for blank or missing headers") {
    SheetData d;
    d.headers = {"name", ""};
    d.rows    = {{"Anna", "x", "extra"}};   // ragged: wider than the header row
    CHECK(render(d, Format::JSONL) ==
          "{\"name\": \"Anna\", \"B\": \"x\", \"C\": \"extra\"}\n");
}

TEST_CASE("markdown writes a header rule and escapes pipes") {
    SheetData d;
    d.headers = {"a", "b"};
    d.rows    = {{"x|y", "line\nbreak"}};
    CHECK(render(d, Format::MD) ==
          "| a | b |\n"
          "| --- | --- |\n"
          "| x\\|y | line break |\n");
}

TEST_CASE("markdown keeps its header rule even without a header") {
    // A markdown table is only a table with a header row, so it blanks out
    // rather than disappearing.
    CHECK(render(sample(), Format::MD, false) ==
          "|  |  |\n"
          "| --- | --- |\n"
          "| Anna | 95000 |\n"
          "| Carlos | 62000 |\n");
}

TEST_CASE("table pads columns and right-aligns numeric ones") {
    CHECK(render(sample(), Format::TABLE) ==
          "name    salary\n"
          "------  ------\n"
          "Anna     95000\n"
          "Carlos   62000\n");
}

TEST_CASE("table measures width in terminal columns, not bytes") {
    SheetData d;
    d.headers = {"a", "b"};
    d.rows    = {{"é", "1"}, {"xx", "2"}};   // 'é' is 2 bytes, 1 column wide
    CHECK(render(d, Format::TABLE) ==       // so it pads to 2, not to 3
          "a   b\n"
          "--  -\n"
          "é   1\n"
          "xx  2\n");
}

TEST_CASE("an empty sheet produces empty structured output") {
    SheetData empty;
    CHECK(render(empty, Format::JSON)  == "[\n]\n");
    CHECK(render(empty, Format::JSONL).empty());
    CHECK(render(empty, Format::MD).empty());
    CHECK(render(empty, Format::TABLE).empty());
}
