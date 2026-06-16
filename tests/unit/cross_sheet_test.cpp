#include <doctest/doctest.h>

#include <cctype>
#include <map>
#include <memory>
#include <string>

#include "formulas/evaluator.hpp"
#include "support/fake_eval_context.hpp"

namespace {

std::string lower(std::string s) {
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

// An EvalContext that mirrors how Grid resolves cross-sheet references: a
// "current" sheet plus a set of named sheets, each its own FakeEvalContext.
// Qualified refs (Sheet2!A1) bounds-check against the named sheet.
class Workbook : public EvalContext {
public:
    FakeEvalContext current{100, 26};

    FakeEvalContext& sheet(const std::string& name) {
        auto& p = m_sheets[lower(name)];
        if (!p) p = std::make_unique<FakeEvalContext>(100, 26);
        return *p;
    }
    FakeEvalContext& sheet(const std::string& name, int rows, int cols) {
        auto& p = m_sheets[lower(name)];
        p = std::make_unique<FakeEvalContext>(rows, cols);
        return *p;
    }

    int   rows() const override { return current.rows(); }
    int   cols() const override { return current.cols(); }
    Value cell_value(int r, int c) const override { return current.cell_value(r, c); }

    Value cell_value_in(const std::string& name, int r, int c) const override {
        auto it = m_sheets.find(lower(name));
        if (it == m_sheets.end()) return Value::error(FormulaError::REF);
        const FakeEvalContext& s = *it->second;
        if (r < 0 || r >= s.rows() || c < 0 || c >= s.cols())
            return Value::error(FormulaError::REF);
        return s.cell_value(r, c);
    }

private:
    std::map<std::string, std::unique_ptr<FakeEvalContext>> m_sheets;
};

std::string disp(const std::string& formula, const EvalContext& ctx) {
    return Evaluator::evaluate_formula(formula, ctx).to_display();
}

}  // namespace

TEST_CASE("qualified single-cell reference reads the named sheet") {
    Workbook wb;
    wb.current.set(0, 0, Value::number(1));            // this sheet A1 = 1
    wb.sheet("Sheet2").set(0, 0, Value::number(42));   // Sheet2!A1 = 42
    wb.sheet("Sheet2").set(1, 1, Value::string("hi")); // Sheet2!B2 = hi

    CHECK(disp("=Sheet2!A1", wb) == "42");
    CHECK(disp("=Sheet2!B2", wb) == "hi");
    CHECK(disp("=Sheet2!A1 + A1", wb) == "43");   // mixes sheets
}

TEST_CASE("sheet names are matched case-insensitively") {
    Workbook wb;
    wb.sheet("Data").set(2, 0, Value::number(7));   // Data!A3
    CHECK(disp("=data!A3", wb) == "7");
    CHECK(disp("=DATA!A3", wb) == "7");
}

TEST_CASE("quoted sheet names with spaces") {
    Workbook wb;
    wb.sheet("My Sheet").set(0, 0, Value::number(99));
    CHECK(disp("='My Sheet'!A1", wb) == "99");
    CHECK(disp("='My Sheet'!A1 * 2", wb) == "198");
}

TEST_CASE("qualified ranges feed aggregate functions") {
    Workbook wb;
    auto& s = wb.sheet("Nums");
    s.set(0, 0, Value::number(10)).set(1, 0, Value::number(20)).set(2, 0, Value::number(30));
    CHECK(disp("=SUM(Nums!A1:A3)", wb) == "60");
    CHECK(disp("=AVERAGE(Nums!A1:A3)", wb) == "20");
    CHECK(disp("=MAX(Nums!A1:A3)", wb) == "30");
}

TEST_CASE("unknown sheet and out-of-range yield #REF!") {
    Workbook wb;
    wb.sheet("Small", /*rows=*/3, /*cols=*/3).set(0, 0, Value::number(1));
    CHECK(disp("=Ghost!A1", wb) == "#REF!");    // no such sheet
    CHECK(disp("=Small!A9", wb) == "#REF!");    // row past the sheet
    CHECK(disp("=Small!Z1", wb) == "#REF!");    // column past the sheet
    CHECK(disp("=Small!A1", wb) == "1");        // in range
}

TEST_CASE("a sheet qualifier without a cell reference is an error") {
    Workbook wb;
    // Lexer emits an ERROR token → parse/eval falls back to #VALUE!.
    CHECK(disp("=Sheet2!", wb) == "#VALUE!");
    CHECK(disp("=Sheet2!+1", wb) == "#VALUE!");
}
