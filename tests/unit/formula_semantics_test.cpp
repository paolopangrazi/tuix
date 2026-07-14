#include <doctest/doctest.h>

#include <string>

#include "formulas/evaluator.hpp"
#include "support/fake_eval_context.hpp"

// Formula-language semantics: whole-input parsing, range clamping, comparison
// case rules, and sheet qualifiers on ranges.

namespace {

Value eval(const std::string& f, const EvalContext& ctx) {
    return Evaluator::evaluate_formula(f, ctx);
}

// A fake that also resolves one named sheet, so qualified ranges can be tested.
class TwoSheetContext : public FakeEvalContext {
public:
    TwoSheetContext() : FakeEvalContext(10, 5), m_other(10, 5) {}
    FakeEvalContext& other() { return m_other; }
    Value cell_value_in(const std::string& sheet, int r, int c) const override {
        if (sheet == "Sheet2") return m_other.cell_value(r, c);
        return Value::error(FormulaError::REF);
    }
private:
    FakeEvalContext m_other;
};

}  // namespace

TEST_CASE("trailing tokens after an expression are an error, not ignored") {
    FakeEvalContext ctx;
    ctx.set(0, 0, Value::number(7));
    CHECK(eval("=A1", ctx).as_number() == 7);
    CHECK(eval("=A1 B2", ctx).is_error());   // used to evaluate as =A1
    CHECK(eval("=1 2", ctx).is_error());
    CHECK(eval("=SUM(A1) junk(", ctx).is_error());
}

TEST_CASE("ranges clamp to the sheet instead of erroring past the edge") {
    FakeEvalContext ctx(5, 3);   // 5 rows only
    ctx.set(0, 0, Value::number(1)).set(1, 0, Value::number(2)).set(4, 0, Value::number(4));
    // A1:A100 reaches past row 5 — previously #REF!, now sums what exists.
    CHECK(eval("=SUM(A1:A100)", ctx).as_number() == 7);
    CHECK(eval("=COUNTIF(A1:A100, \">1\")", ctx).as_number() == 2);
}

TEST_CASE("string comparison operators are case-insensitive, like COUNTIF") {
    FakeEvalContext ctx;
    CHECK(eval("=\"Apple\" = \"apple\"", ctx).as_boolean());
    CHECK(eval("=\"Apple\" <> \"pear\"", ctx).as_boolean());
    CHECK(eval("=IF(\"ABC\"=\"abc\", 1, 0)", ctx).as_number() == 1);
    // Ordering is case-insensitive too: "b" > "A" regardless of case.
    CHECK(eval("=\"b\" > \"A\"", ctx).as_boolean());
}

TEST_CASE("lookup functions honor a sheet qualifier on their range") {
    TwoSheetContext ctx;
    ctx.set(0, 0, Value::string("local"));           // current sheet A1
    ctx.other().set(0, 0, Value::string("key"))      // Sheet2 A1:B1
               .set(0, 1, Value::number(42));
    // Previously these read the *current* sheet regardless of the qualifier.
    CHECK(eval("=VLOOKUP(\"key\", Sheet2!A1:B1, 2, FALSE)", ctx).as_number() == 42);
    CHECK(eval("=COUNTIF(Sheet2!A1:A1, \"key\")", ctx).as_number() == 1);
    CHECK(eval("=INDEX(Sheet2!A1:B1, 1, 2)", ctx).as_number() == 42);
}

TEST_CASE("a qualifier on the range end covers the whole range") {
    TwoSheetContext ctx;
    ctx.other().set(0, 0, Value::number(10)).set(1, 0, Value::number(20));
    CHECK(eval("=SUM(A1:Sheet2!A2)", ctx).as_number() == 30);   // was: current sheet
}

TEST_CASE("COUNTIF criteria use the strict number parse") {
    FakeEvalContext ctx(5, 2);
    ctx.set(0, 0, Value::string("0x10"));
    // "0x10" is text, not 16: it equals itself but is not > 1.
    CHECK(eval("=COUNTIF(A1:A5, \"0x10\")", ctx).as_number() == 1);
    CHECK(eval("=COUNTIF(A1:A5, \">1\")", ctx).as_number() == 0);
}
