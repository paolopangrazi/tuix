#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "formulas/evaluator.hpp"
#include "support/fake_eval_context.hpp"

namespace {

// Evaluate a formula and return its user-visible display string.
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

}  // namespace

TEST_CASE("arithmetic, precedence and unary minus") {
    FakeEvalContext ctx;
    run({
        {"=1+2",        "3"},
        {"=2+3*4",      "14"},      // * binds tighter than +
        {"=(2+3)*4",    "20"},
        {"=10/4",       "2.5"},
        {"=2^10",       "1024"},
        {"=-(3)",       "-3"},
        {"=2+-3",       "-1"},      // unary minus as a sub-expression
        {"=7-2-1",      "4"},       // left-associative
    }, ctx);
}

TEST_CASE("string concatenation and comparison ordering") {
    FakeEvalContext ctx;
    run({
        {"=\"a\"&\"b\"",   "ab"},
        {"=1<2",           "TRUE"},
        {"=2<>3",          "TRUE"},
        {"=2=2",           "TRUE"},
        {"=2>=2",          "TRUE"},
        {"=\"a\"<\"b\"",   "TRUE"},
        {"=1<\"a\"",       "TRUE"},   // numbers rank below strings
    }, ctx);
}

TEST_CASE("aggregate functions over ranges") {
    FakeEvalContext ctx;
    ctx.set(0, 0, Value::number(1))    // A1
       .set(1, 0, Value::number(2))    // A2
       .set(2, 0, Value::number(3));   // A3
    ctx.set(0, 1, Value::number(1))    // B1
       .set(0, 2, Value::string("x")); // C1  (text)
    run({
        {"=SUM(A1:A3)",     "6"},
        {"=AVERAGE(A1:A3)", "2"},
        {"=MIN(A1:A3)",     "1"},
        {"=MAX(A1:A3)",     "3"},
        {"=COUNT(A1:C1)",   "2"},   // only B1 and A1 are numbers
        {"=COUNTA(A1:C1)",  "3"},   // A1, B1, C1 are all non-empty
    }, ctx);
}

TEST_CASE("SUM skips text but treats empty cells as zero") {
    FakeEvalContext ctx;
    ctx.set(0, 0, Value::number(1))
       .set(1, 0, Value::string("x"))   // ignored
       .set(2, 0, Value::number(2));     // A4 left empty
    CHECK(disp("=SUM(A1:A4)", ctx) == "3");
}

TEST_CASE("logical functions") {
    FakeEvalContext ctx;
    run({
        {"=IF(1>0,\"yes\",\"no\")", "yes"},
        {"=IF(0,\"y\",\"n\")",      "n"},
        {"=IF(1>2,\"y\")",          "FALSE"},   // no else branch
        {"=IFERROR(1/0,99)",        "99"},
        {"=IFERROR(5,99)",          "5"},
    }, ctx);
}

TEST_CASE("math functions") {
    FakeEvalContext ctx;
    run({
        {"=ABS(-5)",        "5"},
        {"=ROUND(2.345,2)", "2.35"},
        {"=ROUND(2.5)",     "3"},
        {"=SQRT(16)",       "4"},
        {"=INT(2.9)",       "2"},
        {"=INT(-2.1)",      "-3"},   // floor, not truncation
        {"=MOD(7,3)",       "1"},
    }, ctx);
}

TEST_CASE("text functions") {
    FakeEvalContext ctx;
    run({
        {"=LEN(\"hello\")",            "5"},
        {"=UPPER(\"abc\")",            "ABC"},
        {"=LOWER(\"ABC\")",            "abc"},
        {"=TRIM(\"  a   b  \")",       "a b"},
        {"=CONCATENATE(\"a\",\"b\",\"c\")", "abc"},
    }, ctx);
}

TEST_CASE("cell references resolve through the context") {
    FakeEvalContext ctx;
    ctx.set(0, 0, Value::number(10))   // A1
       .set(0, 1, Value::number(5));   // B1
    run({
        {"=A1+B1", "15"},
        {"=A1*B1", "50"},
        {"=A1",    "10"},
    }, ctx);
}

TEST_CASE("error propagation") {
    FakeEvalContext small(3, 3);   // A1:C3 only
    run({
        {"=1/0",        "#DIV/0!"},   // division by zero
        {"=SQRT(-1)",   "#NUM!"},     // domain error
        {"=FOO()",      "#NAME?"},    // unknown function
        {"=ABS(\"x\")", "#VALUE!"},   // non-numeric argument
        {"=A5",         "#REF!"},     // row 5 is out of a 3-row context
        {"=A1:A3",      "#VALUE!"},   // bare range outside a function
    }, small);
}

TEST_CASE("aggregates over an empty range") {
    FakeEvalContext ctx;
    CHECK(disp("=AVERAGE(A1:A3)", ctx) == "#DIV/0!");  // no numbers → divide by zero
    CHECK(disp("=MIN(A1:A3)", ctx)     == "#NUM!");    // nothing to compare
    CHECK(disp("=MAX(A1:A3)", ctx)     == "#NUM!");
    CHECK(disp("=SUM(A1:A3)", ctx)     == "0");        // empty cells sum to zero
}

TEST_CASE("malformed input does not throw") {
    FakeEvalContext ctx;
    CHECK(disp("=1+", ctx)       == "#VALUE!");
    CHECK(disp("=((1)", ctx)     == "#VALUE!");
    CHECK(disp("=", ctx)         == "#VALUE!");
}
