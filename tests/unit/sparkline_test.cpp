#include <doctest/doctest.h>

#include <string>

#include "formulas/evaluator.hpp"
#include "support/fake_eval_context.hpp"

namespace {

std::string disp(const std::string& formula, const EvalContext& ctx) {
    return Evaluator::evaluate_formula(formula, ctx).to_display();
}

}  // namespace

TEST_CASE("SPARKLINE maps a range to one block glyph per value") {
    FakeEvalContext ctx;
    for (int r = 0; r < 8; ++r) ctx.set(r, 0, Value::number(r + 1));  // A1..A8 = 1..8
    // min→▁ … max→█, evenly stepped across the eight block levels.
    CHECK(disp("=SPARKLINE(A1:A8)", ctx) == "▁▂▃▄▅▆▇█");
}

TEST_CASE("SPARKLINE renders a flat series at mid height") {
    FakeEvalContext ctx;
    ctx.set(0, 0, Value::number(5)).set(1, 0, Value::number(5)).set(2, 0, Value::number(5));
    CHECK(disp("=SPARKLINE(A1:A3)", ctx) == "▅▅▅");
}

TEST_CASE("SPARKLINE skips empty and text cells") {
    FakeEvalContext ctx;
    ctx.set(0, 0, Value::number(1))
       .set(1, 0, Value::string("x"))   // skipped
       .set(2, 0, Value::number(3));    // A2 left empty → skipped
    // Numeric series is {1, 3}: min→▁, max→█.
    CHECK(disp("=SPARKLINE(A1:A4)", ctx) == "▁█");
}

TEST_CASE("SPARKLINE with no numeric values is #N/A") {
    FakeEvalContext ctx;
    CHECK(disp("=SPARKLINE(A1:A4)", ctx) == "#N/A");
    ctx.set(0, 0, Value::string("text"));
    CHECK(disp("=SPARKLINE(A1:A4)", ctx) == "#N/A");
}

TEST_CASE("SPARKLINE propagates an error from the range") {
    FakeEvalContext ctx;
    ctx.set(0, 0, Value::number(1)).set(1, 0, Value::error(FormulaError::DIV0));
    CHECK(disp("=SPARKLINE(A1:A3)", ctx) == "#DIV/0!");
}
