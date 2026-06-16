#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "formulas/evaluator.hpp"
#include "support/fake_eval_context.hpp"

namespace {

std::string disp(const std::string& formula, const EvalContext& ctx) {
    return Evaluator::evaluate_formula(formula, ctx).to_display();
}

struct Case {
    std::string formula;
    std::string expected;
};

void run(const std::vector<Case>& cases, const EvalContext& ctx) {
    for (const auto& c : cases) {
        CAPTURE(c.formula);
        CHECK(disp(c.formula, ctx) == c.expected);
    }
}

// A small lookup table:
//        A         B        C
//  1  "apple"     10       1.5
//  2  "banana"    20       2.5
//  3  "cherry"    30       3.5
FakeEvalContext fruit_table() {
    FakeEvalContext ctx;
    ctx.set(0, 0, Value::string("apple")).set(0, 1, Value::number(10)).set(0, 2, Value::number(1.5));
    ctx.set(1, 0, Value::string("banana")).set(1, 1, Value::number(20)).set(1, 2, Value::number(2.5));
    ctx.set(2, 0, Value::string("cherry")).set(2, 1, Value::number(30)).set(2, 2, Value::number(3.5));
    return ctx;
}

}  // namespace

TEST_CASE("VLOOKUP exact match") {
    auto ctx = fruit_table();
    run({
        {"=VLOOKUP(\"banana\", A1:C3, 2, FALSE)", "20"},
        {"=VLOOKUP(\"banana\", A1:C3, 3, FALSE)", "2.5"},
        {"=VLOOKUP(\"apple\",  A1:C3, 2, FALSE)", "10"},
        {"=VLOOKUP(\"durian\", A1:C3, 2, FALSE)", "#N/A"},   // not found
        {"=VLOOKUP(\"banana\", A1:C3, 4, FALSE)", "#REF!"},  // column past range
        {"=VLOOKUP(\"banana\", A1:C3, 0, FALSE)", "#REF!"},  // column < 1
    }, ctx);
}

TEST_CASE("VLOOKUP approximate match assumes ascending first column") {
    FakeEvalContext ctx;
    ctx.set(0, 0, Value::number(0)) .set(0, 1, Value::string("F"));
    ctx.set(1, 0, Value::number(60)).set(1, 1, Value::string("D"));
    ctx.set(2, 0, Value::number(70)).set(2, 1, Value::string("C"));
    ctx.set(3, 0, Value::number(80)).set(3, 1, Value::string("B"));
    ctx.set(4, 0, Value::number(90)).set(4, 1, Value::string("A"));
    run({
        {"=VLOOKUP(85, A1:B5, 2)",      "B"},      // largest key <= 85 is 80
        {"=VLOOKUP(90, A1:B5, 2)",      "A"},      // exact boundary
        {"=VLOOKUP(72, A1:B5, 2, TRUE)", "C"},
        {"=VLOOKUP(-5, A1:B5, 2)",      "#N/A"},    // below smallest key
    }, ctx);
}

TEST_CASE("MATCH over a 1-D range") {
    auto ctx = fruit_table();
    run({
        {"=MATCH(\"cherry\", A1:A3, 0)", "3"},    // exact, column vector
        {"=MATCH(\"apple\",  A1:A3, 0)", "1"},
        {"=MATCH(\"durian\", A1:A3, 0)", "#N/A"},
        {"=MATCH(20, B1:B3, 0)",         "2"},
        {"=MATCH(25, B1:B3, 1)",         "2"},    // largest <= 25 (ascending)
        {"=MATCH(25, B1:B3)",            "2"},    // default match_type is 1
    }, ctx);
}

TEST_CASE("MATCH requires a one-dimensional range") {
    auto ctx = fruit_table();
    CHECK(disp("=MATCH(10, A1:B3, 0)", ctx) == "#N/A");
}

TEST_CASE("INDEX into a range") {
    auto ctx = fruit_table();
    run({
        {"=INDEX(A1:C3, 2, 1)", "banana"},
        {"=INDEX(A1:C3, 3, 2)", "30"},
        {"=INDEX(A1:A3, 2)",    "banana"},   // single column: index is the row
        {"=INDEX(A1:C1, 3)",    "1.5"},       // single row: lone index picks the column
        {"=INDEX(A1:C3, 4, 1)", "#REF!"},     // row out of range
        {"=INDEX(A1:C3, 1, 9)", "#REF!"},     // column out of range
    }, ctx);
}

TEST_CASE("INDEX/MATCH composed") {
    auto ctx = fruit_table();
    CHECK(disp("=INDEX(B1:B3, MATCH(\"cherry\", A1:A3, 0))", ctx) == "30");
}

TEST_CASE("COUNTIF with comparison and equality criteria") {
    FakeEvalContext ctx;
    ctx.set(0, 0, Value::number(5)).set(1, 0, Value::number(15))
       .set(2, 0, Value::number(25)).set(3, 0, Value::number(15));
    run({
        {"=COUNTIF(A1:A4, \">10\")", "3"},
        {"=COUNTIF(A1:A4, \"<=15\")", "3"},
        {"=COUNTIF(A1:A4, 15)",       "2"},
        {"=COUNTIF(A1:A4, \"<>15\")", "2"},
        {"=COUNTIF(A1:A4, \">100\")", "0"},
    }, ctx);
}

TEST_CASE("COUNTIF matches text case-insensitively") {
    FakeEvalContext ctx;
    ctx.set(0, 0, Value::string("Yes")).set(1, 0, Value::string("no"))
       .set(2, 0, Value::string("YES"));
    CHECK(disp("=COUNTIF(A1:A3, \"yes\")", ctx) == "2");
}

TEST_CASE("SUMIF with and without a separate sum range") {
    FakeEvalContext ctx;
    // A: category, B: amount
    ctx.set(0, 0, Value::string("food")).set(0, 1, Value::number(10));
    ctx.set(1, 0, Value::string("rent")).set(1, 1, Value::number(500));
    ctx.set(2, 0, Value::string("food")).set(2, 1, Value::number(20));
    run({
        {"=SUMIF(A1:A3, \"food\", B1:B3)", "30"},
        {"=SUMIF(B1:B3, \">100\")",        "500"},  // sum the criteria range itself
        {"=SUMIF(A1:A3, \"travel\", B1:B3)", "0"},
    }, ctx);
}

TEST_CASE("AVERAGEIF averages matching cells, errors on no match") {
    FakeEvalContext ctx;
    ctx.set(0, 0, Value::string("a")).set(0, 1, Value::number(10));
    ctx.set(1, 0, Value::string("a")).set(1, 1, Value::number(30));
    ctx.set(2, 0, Value::string("b")).set(2, 1, Value::number(99));
    run({
        {"=AVERAGEIF(A1:A3, \"a\", B1:B3)", "20"},
        {"=AVERAGEIF(A1:A3, \"z\", B1:B3)", "#DIV/0!"},
    }, ctx);
}

TEST_CASE("IFS returns the first true branch") {
    FakeEvalContext ctx;
    ctx.set(0, 0, Value::number(85));   // A1
    run({
        {"=IFS(A1>=90, \"A\", A1>=80, \"B\", A1>=70, \"C\")", "B"},
        {"=IFS(A1>=90, \"A\", A1>=70, \"C\")",                "C"},
        {"=IFS(A1>=90, \"A\")",                               "#N/A"},  // no branch matches
    }, ctx);
}

TEST_CASE("IFNA only traps #N/A") {
    auto ctx = fruit_table();
    run({
        {"=IFNA(VLOOKUP(\"durian\", A1:C3, 2, FALSE), \"missing\")", "missing"},
        {"=IFNA(VLOOKUP(\"banana\", A1:C3, 2, FALSE), \"missing\")", "20"},
        {"=IFNA(1/0, \"x\")", "#DIV/0!"},   // a non-#N/A error passes through
    }, ctx);
}
