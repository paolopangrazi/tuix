// evaluator.cpp — the formula evaluation engine: range expansion, the AST
// walker (Evaluator::eval), and the top-level evaluate_formula entry point.
// The built-in function library it dispatches into lives in
// evaluator_builtins.cpp, reached only through lookup_builtin().
#include "evaluator.hpp"
#include "evaluator_builtins.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <algorithm>
#include <cmath>
#include <exception>

// ── Range expansion ───────────────────────────────────────────────────────────

void Evaluator::collect(const Expr& expr, const EvalContext& ctx, std::vector<Value>& out) const {
    if (expr.kind == Expr::Kind::RANGE) {
        const auto& r = static_cast<const RangeExpr&>(expr);
        int r0 = std::min(r.from.row, r.to.row), r1 = std::max(r.from.row, r.to.row);
        int c0 = std::min(r.from.col, r.to.col), c1 = std::max(r.from.col, r.to.col);
        const std::string& sheet = r.from.sheet;
        for (int row = r0; row <= r1; ++row)
            for (int col = c0; col <= c1; ++col)
                out.push_back(sheet.empty() ? ctx.cell_value(row, col)
                                            : ctx.cell_value_in(sheet, row, col));
    } else {
        out.push_back(eval(expr, ctx));
    }
}

// ── AST walker ────────────────────────────────────────────────────────────────

Value Evaluator::eval(const Expr& expr, const EvalContext& ctx) const {
    switch (expr.kind) {
        case Expr::Kind::NUMBER:
            return Value::number(static_cast<const NumberExpr&>(expr).value);

        case Expr::Kind::STRING:
            return Value::string(static_cast<const StringExpr&>(expr).value);

        case Expr::Kind::BOOL:
            return Value::boolean(static_cast<const BoolExpr&>(expr).value);

        case Expr::Kind::CELL_REF: {
            const auto& e = static_cast<const CellRefExpr&>(expr);
            if (!e.sheet.empty())
                return ctx.cell_value_in(e.sheet, e.row, e.col);
            if (e.row < 0 || e.row >= ctx.rows() || e.col < 0 || e.col >= ctx.cols())
                return Value::error(FormulaError::REF);
            return ctx.cell_value(e.row, e.col);
        }

        case Expr::Kind::RANGE:
            return Value::error(FormulaError::VALUE); // range outside function context

        case Expr::Kind::UNARY: {
            const auto& e = static_cast<const UnaryExpr&>(expr);
            Value v = eval(*e.operand, ctx);
            if (v.is_error()) return v;
            if (e.op == '-') {
                double n;
                if (!v.to_number(n)) return Value::error(FormulaError::VALUE);
                return Value::number(-n);
            }
            return v;
        }

        case Expr::Kind::BINARY: {
            const auto& e  = static_cast<const BinaryExpr&>(expr);
            Value lv = eval(*e.lhs, ctx);
            Value rv = eval(*e.rhs, ctx);
            if (lv.is_error()) return lv;
            if (rv.is_error()) return rv;

            if (e.op == "&")
                return Value::string(lv.to_display() + rv.to_display());

            if (e.op == "+" || e.op == "-" || e.op == "*" || e.op == "/" || e.op == "^") {
                double l, r;
                if (!lv.to_number(l) || !rv.to_number(r)) return Value::error(FormulaError::VALUE);
                if (e.op == "+") return Value::number(l + r);
                if (e.op == "-") return Value::number(l - r);
                if (e.op == "*") return Value::number(l * r);
                if (e.op == "/") return r == 0.0 ? Value::error(FormulaError::DIV0) : Value::number(l / r);
                if (e.op == "^") return Value::number(std::pow(l, r));
            }

            // Comparison: numbers < strings < booleans (OpenFormula ordering)
            auto rank = [](const Value& v) {
                if (v.is_number())  return 0;
                if (v.is_string())  return 1;
                return 2;
            };
            int cmp;
            if (lv.is_number() && rv.is_number())
                cmp = (lv.as_number() < rv.as_number()) ? -1 : (lv.as_number() > rv.as_number() ? 1 : 0);
            else if (lv.is_string() && rv.is_string())
                cmp = lv.as_string().compare(rv.as_string());
            else if (lv.is_boolean() && rv.is_boolean())
                cmp = static_cast<int>(lv.as_boolean()) - static_cast<int>(rv.as_boolean());
            else
                cmp = rank(lv) - rank(rv);

            if (e.op == "=")  return Value::boolean(cmp == 0);
            if (e.op == "<>") return Value::boolean(cmp != 0);
            if (e.op == "<")  return Value::boolean(cmp <  0);
            if (e.op == ">")  return Value::boolean(cmp >  0);
            if (e.op == "<=") return Value::boolean(cmp <= 0);
            if (e.op == ">=") return Value::boolean(cmp >= 0);
            return Value::error(FormulaError::VALUE);
        }

        case Expr::Kind::FUNC_CALL: {
            const auto& e = static_cast<const FuncCallExpr&>(expr);
            if (BuiltinFn fn = lookup_builtin(e.name))
                return fn(e, ctx, *this);
            return Value::error(FormulaError::NAME);
        }
    }
    return Value::error(FormulaError::VALUE);
}

Value Evaluator::evaluate_formula(const std::string& formula, const EvalContext& ctx) {
    try {
        Lexer  lexer(formula);
        Parser parser(lexer.tokenize());
        return Evaluator{}.eval(*parser.parse(), ctx);
    } catch (const std::exception&) {
        return Value::error(FormulaError::VALUE);
    }
}
