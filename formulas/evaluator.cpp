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
        const std::string& sheet = r.from.sheet;
        for (int row = r0; row <= r1; ++row)
            for (int col = c0; col <= c1; ++col)
                out.push_back(sheet.empty() ? ctx.cell_value(row, col)
                                            : ctx.cell_value_in(sheet, row, col));
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

// ── Lookup & conditional helpers ────────────────────────────────────────────────

// Resolve an argument that should denote a rectangular range. A bare cell ref
// is treated as a 1×1 range. Returns false if the expression is neither.
static bool arg_bounds(const Expr& e, int& r0, int& c0, int& r1, int& c1) {
    if (e.kind == Expr::Kind::RANGE) {
        const auto& r = static_cast<const RangeExpr&>(e);
        r0 = std::min(r.from.row, r.to.row); r1 = std::max(r.from.row, r.to.row);
        c0 = std::min(r.from.col, r.to.col); c1 = std::max(r.from.col, r.to.col);
        return true;
    }
    if (e.kind == Expr::Kind::CELL_REF) {
        const auto& c = static_cast<const CellRefExpr&>(e);
        r0 = r1 = c.row; c0 = c1 = c.col;
        return true;
    }
    return false;
}

// Strict full-string parse of a numeric operand (rejects "12abc").
static bool parse_full_number(const std::string& s, double& out) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        out = std::stod(s, &pos);
        return pos == s.size();
    } catch (...) { return false; }
}

// A value usable for ordered/equality matching: numbers, booleans, and numeric
// strings collapse to a double; empty cells and errors do not count as numbers.
static bool to_strict_number(const Value& v, double& out) {
    if (v.is_number())  { out = v.as_number();              return true; }
    if (v.is_boolean()) { out = v.as_boolean() ? 1.0 : 0.0; return true; }
    if (v.is_string())  return parse_full_number(v.as_string(), out);
    return false;
}

// Equality the way COUNTIF/MATCH/VLOOKUP compare: numeric when both look
// numeric, otherwise case-insensitive text.
static bool values_equal(const Value& a, const Value& b) {
    double an, bn;
    if (to_strict_number(a, an) && to_strict_number(b, bn)) return an == bn;
    std::string sa = a.to_display(), sb = b.to_display();
    if (sa.size() != sb.size()) return false;
    for (size_t i = 0; i < sa.size(); ++i)
        if (std::tolower((unsigned char)sa[i]) != std::tolower((unsigned char)sb[i]))
            return false;
    return true;
}

// Ordered comparison returning -1/0/1: numeric when both are numbers, else
// lexicographic on the display text.
static int value_cmp(const Value& a, const Value& b) {
    double an, bn;
    if (to_strict_number(a, an) && to_strict_number(b, bn))
        return an < bn ? -1 : (an > bn ? 1 : 0);
    int c = a.to_display().compare(b.to_display());
    return c < 0 ? -1 : (c > 0 ? 1 : 0);
}

// Apply an Excel-style criterion to a cell. A numeric criterion is plain
// equality; a string criterion may carry a leading comparison operator
// (">10", "<=5", "<>x"), defaulting to equality.
static bool match_criterion(const Value& cell, const Value& crit) {
    if (!crit.is_string()) return values_equal(cell, crit);

    const std::string& s = crit.as_string();
    std::string op;
    size_t i = 0;
    if (!s.empty() && (s[0] == '<' || s[0] == '>' || s[0] == '=')) {
        op += s[i++];
        if (i < s.size() && (s[i] == '=' || (op == "<" && s[i] == '>'))) op += s[i++];
    }
    std::string rest = s.substr(i);
    Value operand = Value::string(rest);
    double on;
    if (parse_full_number(rest, on)) operand = Value::number(on);

    if (op.empty() || op == "=") return values_equal(cell, operand);
    if (op == "<>")              return !values_equal(cell, operand);
    int c = value_cmp(cell, operand);
    if (op == ">")  return c > 0;
    if (op == "<")  return c < 0;
    if (op == ">=") return c >= 0;
    if (op == "<=") return c <= 0;
    return false;
}

// ── Conditional aggregates ────────────────────────────────────────────────────

static Value fn_countif(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 2) return Value::error(FormulaError::VALUE);
    int r0, c0, r1, c1;
    if (!arg_bounds(*f.args[0], r0, c0, r1, c1)) return Value::error(FormulaError::VALUE);
    Value crit = ev.eval(*f.args[1], ctx);
    if (crit.is_error()) return crit;
    int count = 0;
    for (int r = r0; r <= r1; ++r)
        for (int c = c0; c <= c1; ++c) {
            Value cell = ctx.cell_value(r, c);
            if (!cell.is_empty() && match_criterion(cell, crit)) ++count;
        }
    return Value::number(static_cast<double>(count));
}

// Shared core for SUMIF/AVERAGEIF: walk the criteria range, and for each match
// fold in the corresponding cell of sum_range (or the criteria cell itself).
static Value sumif_core(const FuncCallExpr& f, const EvalContext& ctx,
                        const Evaluator& ev, bool average) {
    if (f.args.size() < 2 || f.args.size() > 3) return Value::error(FormulaError::VALUE);
    int r0, c0, r1, c1;
    if (!arg_bounds(*f.args[0], r0, c0, r1, c1)) return Value::error(FormulaError::VALUE);
    Value crit = ev.eval(*f.args[1], ctx);
    if (crit.is_error()) return crit;

    bool have_sum = f.args.size() == 3;
    int sr0 = r0, sc0 = c0, sr1 = r1, sc1 = c1;
    if (have_sum && !arg_bounds(*f.args[2], sr0, sc0, sr1, sc1))
        return Value::error(FormulaError::VALUE);

    double total = 0.0;
    int count = 0;
    for (int r = r0; r <= r1; ++r)
        for (int c = c0; c <= c1; ++c) {
            Value cell = ctx.cell_value(r, c);
            if (cell.is_empty() || !match_criterion(cell, crit)) continue;
            Value target = have_sum ? ctx.cell_value(sr0 + (r - r0), sc0 + (c - c0)) : cell;
            double n;
            if (!target.is_empty() && target.to_number(n)) { total += n; ++count; }
        }
    if (average) return count == 0 ? Value::error(FormulaError::DIV0)
                                   : Value::number(total / count);
    return Value::number(total);
}

static Value fn_sumif(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    return sumif_core(f, ctx, ev, /*average=*/false);
}

static Value fn_averageif(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    return sumif_core(f, ctx, ev, /*average=*/true);
}

// ── Lookup functions ──────────────────────────────────────────────────────────

static Value fn_vlookup(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() < 3 || f.args.size() > 4) return Value::error(FormulaError::VALUE);
    Value key = ev.eval(*f.args[0], ctx);
    if (key.is_error()) return key;
    int r0, c0, r1, c1;
    if (!arg_bounds(*f.args[1], r0, c0, r1, c1)) return Value::error(FormulaError::VALUE);
    double cidx;
    if (!ev.eval(*f.args[2], ctx).to_number(cidx)) return Value::error(FormulaError::VALUE);
    int col = static_cast<int>(cidx);
    if (col < 1 || c0 + col - 1 > c1) return Value::error(FormulaError::REF);

    bool approx = true;
    if (f.args.size() == 4) {
        Value rl = ev.eval(*f.args[3], ctx);
        double n = 0;
        approx = rl.is_boolean() ? rl.as_boolean() : (rl.to_number(n) && n != 0.0);
    }
    int target_col = c0 + col - 1;
    if (!approx) {
        for (int r = r0; r <= r1; ++r)
            if (values_equal(ctx.cell_value(r, c0), key))
                return ctx.cell_value(r, target_col);
        return Value::error(FormulaError::NA);
    }
    // Approximate match: largest first-column value <= key, assuming ascending order.
    int best = -1;
    for (int r = r0; r <= r1; ++r) {
        if (value_cmp(ctx.cell_value(r, c0), key) <= 0) best = r; else break;
    }
    return best < 0 ? Value::error(FormulaError::NA) : ctx.cell_value(best, target_col);
}

static Value fn_match(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() < 2 || f.args.size() > 3) return Value::error(FormulaError::VALUE);
    Value key = ev.eval(*f.args[0], ctx);
    if (key.is_error()) return key;
    int r0, c0, r1, c1;
    if (!arg_bounds(*f.args[1], r0, c0, r1, c1)) return Value::error(FormulaError::VALUE);
    if (r0 != r1 && c0 != c1) return Value::error(FormulaError::NA);  // needs a 1-D vector

    int match_type = 1;
    if (f.args.size() == 3) {
        double m;
        if (!ev.eval(*f.args[2], ctx).to_number(m)) return Value::error(FormulaError::VALUE);
        match_type = static_cast<int>(m);
    }
    int len = (r0 == r1) ? (c1 - c0 + 1) : (r1 - r0 + 1);
    auto at = [&](int i) {
        return (r0 == r1) ? ctx.cell_value(r0, c0 + i) : ctx.cell_value(r0 + i, c0);
    };
    if (match_type == 0) {
        for (int i = 0; i < len; ++i)
            if (values_equal(at(i), key)) return Value::number(i + 1);
        return Value::error(FormulaError::NA);
    }
    int best = -1;
    for (int i = 0; i < len; ++i) {
        int c = value_cmp(at(i), key);
        if (match_type == 1 ? c <= 0 : c >= 0) best = i; else break;
    }
    return best < 0 ? Value::error(FormulaError::NA) : Value::number(best + 1);
}

static Value fn_index(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() < 2 || f.args.size() > 3) return Value::error(FormulaError::VALUE);
    int r0, c0, r1, c1;
    if (!arg_bounds(*f.args[0], r0, c0, r1, c1)) return Value::error(FormulaError::VALUE);
    double rn = 0, cn = 0;
    if (!ev.eval(*f.args[1], ctx).to_number(rn)) return Value::error(FormulaError::VALUE);
    bool have_col = f.args.size() == 3;
    if (have_col && !ev.eval(*f.args[2], ctx).to_number(cn)) return Value::error(FormulaError::VALUE);

    int nrows = r1 - r0 + 1, ncols = c1 - c0 + 1;
    int ri = static_cast<int>(rn), ci = static_cast<int>(cn);
    if (!have_col && nrows == 1 && ncols > 1) { ci = ri; ri = 1; }  // single row: index picks column
    if (ci == 0) ci = 1;
    if (ri < 1 || ri > nrows || ci < 1 || ci > ncols) return Value::error(FormulaError::REF);
    return ctx.cell_value(r0 + ri - 1, c0 + ci - 1);
}

// ── Extra conditionals ─────────────────────────────────────────────────────────

static Value fn_ifs(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.empty() || f.args.size() % 2 != 0) return Value::error(FormulaError::VALUE);
    for (size_t i = 0; i + 1 < f.args.size(); i += 2) {
        Value cond = ev.eval(*f.args[i], ctx);
        if (cond.is_error()) return cond;
        double n = 0;
        bool truthy = cond.is_boolean() ? cond.as_boolean() : (cond.to_number(n) && n != 0.0);
        if (truthy) return ev.eval(*f.args[i + 1], ctx);
    }
    return Value::error(FormulaError::NA);
}

static Value fn_ifna(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 2) return Value::error(FormulaError::VALUE);
    Value v = ev.eval(*f.args[0], ctx);
    if (v.is_error() && v.as_error() == FormulaError::NA) return ev.eval(*f.args[1], ctx);
    return v;
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
    { "COUNTIF",     fn_countif     },
    { "SUMIF",       fn_sumif       },
    { "AVERAGEIF",   fn_averageif   },
    { "VLOOKUP",     fn_vlookup     },
    { "MATCH",       fn_match       },
    { "INDEX",       fn_index       },
    { "IFS",         fn_ifs         },
    { "IFNA",        fn_ifna        },
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
