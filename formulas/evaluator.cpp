#include "evaluator.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <exception>


// ── Range expansion ───────────────────────────────────────────────────────────

void Evaluator::collect(const Expr& expr, const EvalContext& ctx, std::vector<Value>& out) const {
    if (expr.kind == Expr::Kind::RANGE) {
        const auto& r = static_cast<const RangeExpr&>(expr);
        int r0 = std::min(r.from.row, r.to.row), r1 = std::max(r.from.row, r.to.row);
        int c0 = std::min(r.from.col, r.to.col), c1 = std::max(r.from.col, r.to.col);
        for (int row = r0; row <= r1; ++row)
            for (int col = c0; col <= c1; ++col)
                out.push_back(ctx.cell_value(row, col));
    } else {
        out.push_back(eval(expr, ctx));
    }
}

// ── Built-in functions ────────────────────────────────────────────────────────

// Visit every value across all of f's args (ranges expanded). The visitor
// returns false to stop early — used to bail out on the first error.
template <class F>
static void for_each_value(const FuncCallExpr& f, const EvalContext& ctx,
                           const Evaluator& ev, F&& visit) {
    for (const auto& arg : f.args) {
        std::vector<Value> vals;
        ev.collect(*arg, ctx, vals);
        for (const auto& v : vals)
            if (!visit(v)) return;
    }
}

static Value fn_sum(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    double total = 0.0;
    Value err;
    for_each_value(f, ctx, ev, [&](const Value& v) {
        if (v.is_error()) { err = v; return false; }
        double n; if (v.to_number(n)) total += n;
        return true;
    });
    return err.is_error() ? err : Value::number(total);
}

static Value fn_average(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    double total = 0.0; int count = 0;
    Value err;
    for_each_value(f, ctx, ev, [&](const Value& v) {
        if (v.is_error()) { err = v; return false; }
        double n;
        if (!v.is_empty() && v.to_number(n)) { total += n; ++count; }
        return true;
    });
    if (err.is_error()) return err;
    return count == 0 ? Value::error(FormulaError::DIV0) : Value::number(total / count);
}

static Value fn_count(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    int count = 0;
    for_each_value(f, ctx, ev, [&](const Value& v) {
        if (v.is_number()) ++count;
        return true;
    });
    return Value::number(static_cast<double>(count));
}

static Value fn_counta(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    int count = 0;
    for_each_value(f, ctx, ev, [&](const Value& v) {
        if (!v.is_empty()) ++count;
        return true;
    });
    return Value::number(static_cast<double>(count));
}

static Value fn_min(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    double result = std::numeric_limits<double>::infinity(); bool found = false;
    Value err;
    for_each_value(f, ctx, ev, [&](const Value& v) {
        if (v.is_error()) { err = v; return false; }
        double n;
        if (!v.is_empty() && v.to_number(n)) { result = std::min(result, n); found = true; }
        return true;
    });
    if (err.is_error()) return err;
    return found ? Value::number(result) : Value::error(FormulaError::NUM);
}

static Value fn_max(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    double result = -std::numeric_limits<double>::infinity(); bool found = false;
    Value err;
    for_each_value(f, ctx, ev, [&](const Value& v) {
        if (v.is_error()) { err = v; return false; }
        double n;
        if (!v.is_empty() && v.to_number(n)) { result = std::max(result, n); found = true; }
        return true;
    });
    if (err.is_error()) return err;
    return found ? Value::number(result) : Value::error(FormulaError::NUM);
}

static Value fn_if(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() < 2) return Value::error(FormulaError::VALUE);
    Value cond = ev.eval(*f.args[0], ctx);
    if (cond.is_error()) return cond;
    double n = 0;
    bool truthy = cond.is_boolean() ? cond.as_boolean() : (cond.to_number(n) && n != 0.0);
    if (truthy)             return ev.eval(*f.args[1], ctx);
    if (f.args.size() >= 3) return ev.eval(*f.args[2], ctx);
    return Value::boolean(false);
}

static Value fn_iferror(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() < 2) return Value::error(FormulaError::VALUE);
    Value v = ev.eval(*f.args[0], ctx);
    return v.is_error() ? ev.eval(*f.args[1], ctx) : v;
}

static Value fn_abs(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 1) return Value::error(FormulaError::VALUE);
    Value v = ev.eval(*f.args[0], ctx);
    double n; if (!v.to_number(n)) return Value::error(FormulaError::VALUE);
    return Value::number(std::abs(n));
}

static Value fn_round(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() < 1) return Value::error(FormulaError::VALUE);
    Value vn = ev.eval(*f.args[0], ctx);
    double n = 0, digits = 0;
    if (!vn.to_number(n)) return Value::error(FormulaError::VALUE);
    if (f.args.size() >= 2) {
        Value vd = ev.eval(*f.args[1], ctx);
        if (!vd.to_number(digits)) return Value::error(FormulaError::VALUE);
    }
    double factor = std::pow(10.0, std::floor(digits));
    return Value::number(std::round(n * factor) / factor);
}

static Value fn_sqrt(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 1) return Value::error(FormulaError::VALUE);
    Value v = ev.eval(*f.args[0], ctx);
    double n; if (!v.to_number(n)) return Value::error(FormulaError::VALUE);
    if (n < 0) return Value::error(FormulaError::NUM);
    return Value::number(std::sqrt(n));
}

static Value fn_int(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 1) return Value::error(FormulaError::VALUE);
    Value v = ev.eval(*f.args[0], ctx);
    double n; if (!v.to_number(n)) return Value::error(FormulaError::VALUE);
    return Value::number(std::floor(n));
}

static Value fn_mod(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 2) return Value::error(FormulaError::VALUE);
    double n, d;
    if (!ev.eval(*f.args[0], ctx).to_number(n)) return Value::error(FormulaError::VALUE);
    if (!ev.eval(*f.args[1], ctx).to_number(d)) return Value::error(FormulaError::VALUE);
    if (d == 0) return Value::error(FormulaError::DIV0);
    return Value::number(n - std::floor(n / d) * d);
}

static Value fn_len(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 1) return Value::error(FormulaError::VALUE);
    Value v = ev.eval(*f.args[0], ctx);
    return Value::number(static_cast<double>(v.to_display().size()));
}

static Value fn_upper(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 1) return Value::error(FormulaError::VALUE);
    std::string s = ev.eval(*f.args[0], ctx).to_display();
    for (char& c : s) c = (char)std::toupper((unsigned char)c);
    return Value::string(s);
}

static Value fn_lower(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 1) return Value::error(FormulaError::VALUE);
    std::string s = ev.eval(*f.args[0], ctx).to_display();
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return Value::string(s);
}

static Value fn_trim(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 1) return Value::error(FormulaError::VALUE);
    std::string s = ev.eval(*f.args[0], ctx).to_display();
    size_t start = s.find_first_not_of(' ');
    if (start == std::string::npos) return Value::string("");
    size_t end = s.find_last_not_of(' ');
    std::string result = s.substr(start, end - start + 1);
    // collapse internal runs of spaces to single space
    std::string out;
    bool prev_space = false;
    for (char c : result) {
        if (c == ' ') { if (!prev_space) out += c; prev_space = true; }
        else          { out += c; prev_space = false; }
    }
    return Value::string(out);
}

static Value fn_concatenate(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    std::string result;
    for (const auto& arg : f.args)
        result += ev.eval(*arg, ctx).to_display();
    return Value::string(result);
}

// ── Dispatch table ────────────────────────────────────────────────────────────

using BuiltinFn = Value(*)(const FuncCallExpr&, const EvalContext&, const Evaluator&);

static const std::pair<const char*, BuiltinFn> k_functions[] = {
    { "SUM",         fn_sum         },
    { "AVERAGE",     fn_average     },
    { "COUNT",       fn_count       },
    { "COUNTA",      fn_counta      },
    { "MIN",         fn_min         },
    { "MAX",         fn_max         },
    { "IF",          fn_if          },
    { "IFERROR",     fn_iferror     },
    { "ABS",         fn_abs         },
    { "ROUND",       fn_round       },
    { "SQRT",        fn_sqrt        },
    { "INT",         fn_int         },
    { "MOD",         fn_mod         },
    { "LEN",         fn_len         },
    { "UPPER",       fn_upper       },
    { "LOWER",       fn_lower       },
    { "TRIM",        fn_trim        },
    { "CONCATENATE", fn_concatenate },
};

// ── Evaluator::eval ───────────────────────────────────────────────────────────

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
            for (const auto& [name, fn] : k_functions)
                if (e.name == name) return fn(e, ctx, *this);
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
