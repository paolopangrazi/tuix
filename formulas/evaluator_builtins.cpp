// evaluator_builtins.cpp — the library of built-in spreadsheet functions
// (SUM, IF, VLOOKUP, …) and the name→function dispatch table. The evaluation
// engine itself lives in evaluator.cpp and reaches these only through
// lookup_builtin().
#include "evaluator_builtins.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "evaluator.hpp"
#include "util/numbers.hpp"
#include "util/strings.hpp"

// ── Shared helpers ───────────────────────────────────────────────────────────

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

// Gather the numeric values across all of f's args, skipping empty and
// non-numeric cells. Returns the first error encountered (empty Value if none).
static Value collect_numbers(const FuncCallExpr& f, const EvalContext& ctx,
                             const Evaluator& ev, std::vector<double>& out) {
    Value err;
    for_each_value(f, ctx, ev, [&](const Value& v) {
        if (v.is_error()) { err = v; return false; }
        double n;
        if (!v.is_empty() && v.to_number(n)) out.push_back(n);
        return true;
    });
    return err;
}

// Evaluate argument i and coerce it to a number; false on failure.
static bool eval_num_arg(const FuncCallExpr& f, size_t i, const EvalContext& ctx,
                         const Evaluator& ev, double& out) {
    return ev.eval(*f.args[i], ctx).to_number(out);
}

// Spreadsheet truthiness: booleans as-is, anything else numeric and non-zero.
static bool is_truthy(const Value& v) {
    if (v.is_boolean()) return v.as_boolean();
    double n;
    return v.to_number(n) && n != 0.0;
}

// ── Numeric aggregates ───────────────────────────────────────────────────────

static Value fn_sum(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    std::vector<double> nums;
    Value err = collect_numbers(f, ctx, ev, nums);
    if (err.is_error()) return err;
    return Value::number(std::accumulate(nums.begin(), nums.end(), 0.0));
}

static Value fn_average(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    std::vector<double> nums;
    Value err = collect_numbers(f, ctx, ev, nums);
    if (err.is_error()) return err;
    if (nums.empty()) return Value::error(FormulaError::DIV0);
    return Value::number(std::accumulate(nums.begin(), nums.end(), 0.0) / (double)nums.size());
}

// Shared body for COUNT/COUNTA: count the values across all of f's args
// matching `pred`.
template <class Pred>
static int count_matching(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev, Pred pred) {
    int count = 0;
    for_each_value(f, ctx, ev, [&](const Value& v) {
        if (pred(v)) ++count;
        return true;
    });
    return count;
}

static Value fn_count(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    int count = count_matching(f, ctx, ev, [](const Value& v) { return v.is_number(); });
    return Value::number(static_cast<double>(count));
}

static Value fn_counta(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    int count = count_matching(f, ctx, ev, [](const Value& v) { return !v.is_empty(); });
    return Value::number(static_cast<double>(count));
}

// Shared body for MIN/MAX: gather numeric args, then reduce with `pick`
// (std::min_element or std::max_element).
template <class Pick>
static Value minmax(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev, Pick pick) {
    std::vector<double> nums;
    Value err = collect_numbers(f, ctx, ev, nums);
    if (err.is_error()) return err;
    if (nums.empty()) return Value::error(FormulaError::NUM);
    return Value::number(*pick(nums.begin(), nums.end()));
}

static Value fn_min(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    return minmax(f, ctx, ev, [](auto b, auto e) { return std::min_element(b, e); });
}

static Value fn_max(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    return minmax(f, ctx, ev, [](auto b, auto e) { return std::max_element(b, e); });
}

// ── Conditionals (IF / IFERROR / IFS / IFNA) ─────────────────────────────────

static Value fn_if(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() < 2) return Value::error(FormulaError::VALUE);
    Value cond = ev.eval(*f.args[0], ctx);
    if (cond.is_error()) return cond;
    if (is_truthy(cond))    return ev.eval(*f.args[1], ctx);
    if (f.args.size() >= 3) return ev.eval(*f.args[2], ctx);
    return Value::boolean(false);
}

static Value fn_iferror(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 2) return Value::error(FormulaError::VALUE);
    Value v = ev.eval(*f.args[0], ctx);
    return v.is_error() ? ev.eval(*f.args[1], ctx) : v;
}

static Value fn_ifs(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.empty() || f.args.size() % 2 != 0) return Value::error(FormulaError::VALUE);
    for (size_t i = 0; i + 1 < f.args.size(); i += 2) {
        Value cond = ev.eval(*f.args[i], ctx);
        if (cond.is_error()) return cond;
        if (is_truthy(cond)) return ev.eval(*f.args[i + 1], ctx);
    }
    return Value::error(FormulaError::NA);
}

static Value fn_ifna(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 2) return Value::error(FormulaError::VALUE);
    Value v = ev.eval(*f.args[0], ctx);
    if (v.is_error() && v.as_error() == FormulaError::NA) return ev.eval(*f.args[1], ctx);
    return v;
}

// ── Math (one-argument and MOD / ROUND) ──────────────────────────────────────

// Shared body for the plain one-argument math functions: check arity, eval
// the single numeric arg, then hand it to `transform` (which returns the
// final Value, so it can itself signal a domain error like SQRT(-1)).
template <class F>
static Value unary_math(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev, F transform) {
    double n;
    if (f.args.size() != 1 || !eval_num_arg(f, 0, ctx, ev, n))
        return Value::error(FormulaError::VALUE);
    return transform(n);
}

static Value fn_abs(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    return unary_math(f, ctx, ev, [](double n) { return Value::number(std::abs(n)); });
}

static Value fn_round(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() < 1) return Value::error(FormulaError::VALUE);
    double n = 0, digits = 0;
    if (!eval_num_arg(f, 0, ctx, ev, n)) return Value::error(FormulaError::VALUE);
    if (f.args.size() >= 2 && !eval_num_arg(f, 1, ctx, ev, digits))
        return Value::error(FormulaError::VALUE);
    double factor = std::pow(10.0, std::floor(digits));
    if (!std::isfinite(factor) || factor == 0) return Value::error(FormulaError::NUM);
    double result = std::round(n * factor) / factor;
    if (!std::isfinite(result)) return Value::error(FormulaError::NUM);
    return Value::number(result);
}

static Value fn_sqrt(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    return unary_math(f, ctx, ev, [](double n) {
        return n < 0 ? Value::error(FormulaError::NUM) : Value::number(std::sqrt(n));
    });
}

static Value fn_int(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    return unary_math(f, ctx, ev, [](double n) { return Value::number(std::floor(n)); });
}

static Value fn_mod(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    double n, d;
    if (f.args.size() != 2 || !eval_num_arg(f, 0, ctx, ev, n) || !eval_num_arg(f, 1, ctx, ev, d))
        return Value::error(FormulaError::VALUE);
    if (d == 0) return Value::error(FormulaError::DIV0);
    return Value::number(n - std::floor(n / d) * d);
}

// ── Text functions ───────────────────────────────────────────────────────────

static Value fn_len(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 1) return Value::error(FormulaError::VALUE);
    Value v = ev.eval(*f.args[0], ctx);
    if (v.is_error()) return v;
    return Value::number(static_cast<double>(v.to_display().size()));
}

static Value fn_upper(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 1) return Value::error(FormulaError::VALUE);
    Value v = ev.eval(*f.args[0], ctx);
    if (v.is_error()) return v;
    return Value::string(tuix::to_upper(v.to_display()));
}

static Value fn_lower(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 1) return Value::error(FormulaError::VALUE);
    Value v = ev.eval(*f.args[0], ctx);
    if (v.is_error()) return v;
    return Value::string(tuix::to_lower(v.to_display()));
}

static Value fn_trim(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 1) return Value::error(FormulaError::VALUE);
    Value arg = ev.eval(*f.args[0], ctx);
    if (arg.is_error()) return arg;
    std::string s = arg.to_display();
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
    for (const auto& arg : f.args) {
        Value v = ev.eval(*arg, ctx);
        if (v.is_error()) return v;
        result += v.to_display();
    }
    return Value::string(result);
}

// In-cell bar chart: one block glyph (▁▂▃▄▅▆▇█) per numeric value in the
// argument range(s), scaled to the data's min/max. Empty/text cells are
// skipped; the first error propagates; a flat series renders all mid-height.
static Value fn_sparkline(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    static const char* k_blocks[8] = {
        "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"
    };
    std::vector<double> nums;
    Value err = collect_numbers(f, ctx, ev, nums);
    if (err.is_error()) return err;
    if (nums.empty()) return Value::error(FormulaError::NA);

    const double lo = *std::min_element(nums.begin(), nums.end());
    const double hi = *std::max_element(nums.begin(), nums.end());
    std::string out;
    for (double n : nums) {
        int level = (hi > lo) ? static_cast<int>((n - lo) / (hi - lo) * 7.0 + 0.5)
                              : 4;                      // flat series → mid glyph
        out += k_blocks[std::clamp(level, 0, 7)];
    }
    return Value::string(out);
}

// ── Lookup & conditional helpers ─────────────────────────────────────────────

// Resolve an argument that should denote a rectangular range. A bare cell ref
// is treated as a 1×1 range. Returns false if the expression is neither.
// `sheet` receives the range's qualifier ("" = current sheet); same-sheet
// bounds are clamped to the grid so oversized ranges neither error nor spin.
static bool arg_bounds(const Expr& e, const EvalContext& ctx,
                       int& r0, int& c0, int& r1, int& c1, std::string& sheet) {
    if (e.kind == Expr::Kind::RANGE) {
        const auto& r = static_cast<const RangeExpr&>(e);
        r0 = std::min(r.from.row, r.to.row); r1 = std::max(r.from.row, r.to.row);
        c0 = std::min(r.from.col, r.to.col); c1 = std::max(r.from.col, r.to.col);
        sheet = r.from.sheet;
    } else if (e.kind == Expr::Kind::CELL_REF) {
        const auto& c = static_cast<const CellRefExpr&>(e);
        r0 = r1 = c.row; c0 = c1 = c.col;
        sheet = c.sheet;
    } else {
        return false;
    }
    if (sheet.empty()) {
        r0 = std::max(r0, 0);
        c0 = std::max(c0, 0);
        r1 = std::min(r1, ctx.rows() - 1);
        c1 = std::min(c1, ctx.cols() - 1);
    }
    return true;
}

// Read one cell of a range resolved by arg_bounds, honoring its qualifier.
static Value range_cell(const EvalContext& ctx, const std::string& sheet, int r, int c) {
    return sheet.empty() ? ctx.cell_value(r, c) : ctx.cell_value_in(sheet, r, c);
}

// A value usable for ordered/equality matching: numbers, booleans, and numeric
// strings collapse to a double; empty cells and errors do not count as numbers.
// String parsing is strict full-string (tuix::parse_number rejects "12abc").
static bool to_strict_number(const Value& v, double& out) {
    if (v.is_number())  { out = v.as_number();              return true; }
    if (v.is_boolean()) { out = v.as_boolean() ? 1.0 : 0.0; return true; }
    if (v.is_string())  return tuix::parse_number(v.as_string(), out);
    return false;
}

// Equality the way COUNTIF/MATCH/VLOOKUP compare: numeric when both look
// numeric, otherwise case-insensitive text.
static bool values_equal(const Value& a, const Value& b) {
    double an, bn;
    if (to_strict_number(a, an) && to_strict_number(b, bn)) return an == bn;
    return tuix::iequals(a.to_display(), b.to_display());
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

// Approximate-match scan shared by VLOOKUP and MATCH: walk a 1-D sequence of
// length `len` via `at(i)`, assumed sorted ascending (or descending), and
// return the index of the last element still on the "key side" of the sort
// order — i.e. the largest value <= key (ascending) or smallest >= key
// (descending) — stopping at the first element that breaks that order.
// -1 if no element qualifies.
template <class At>
static int approx_match(int len, At at, const Value& key, bool ascending) {
    int best = -1;
    for (int i = 0; i < len; ++i) {
        int c = value_cmp(at(i), key);
        if (ascending ? c <= 0 : c >= 0) best = i; else break;
    }
    return best;
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
    if (tuix::parse_number(rest, on)) operand = Value::number(on);

    if (op.empty() || op == "=") return values_equal(cell, operand);
    if (op == "<>")              return !values_equal(cell, operand);
    int c = value_cmp(cell, operand);
    if (op == ">")  return c > 0;
    if (op == "<")  return c < 0;
    if (op == ">=") return c >= 0;
    if (op == "<=") return c <= 0;
    return false;
}

// ── Conditional aggregates (COUNTIF / SUMIF / AVERAGEIF) ──────────────────────

// Resolves the (range, criteria) argument pair shared by COUNTIF/SUMIF/
// AVERAGEIF: arg_bounds on args[0] for the range, then evaluates args[1] as
// the criterion. Returns an error Value on failure (bad range, or a
// criterion expression that itself errors) — check the result's is_error().
static Value resolve_criteria_range(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev,
                                     int& r0, int& c0, int& r1, int& c1, std::string& sheet, Value& crit) {
    if (!arg_bounds(*f.args[0], ctx, r0, c0, r1, c1, sheet)) return Value::error(FormulaError::VALUE);
    crit = ev.eval(*f.args[1], ctx);
    if (crit.is_error()) return crit;
    return Value::empty();
}

static Value fn_countif(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() != 2) return Value::error(FormulaError::VALUE);
    int r0, c0, r1, c1;
    std::string sheet;
    Value crit;
    if (Value err = resolve_criteria_range(f, ctx, ev, r0, c0, r1, c1, sheet, crit); err.is_error())
        return err;
    int count = 0;
    for (int r = r0; r <= r1; ++r)
        for (int c = c0; c <= c1; ++c) {
            Value cell = range_cell(ctx, sheet, r, c);
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
    std::string sheet;
    Value crit;
    if (Value err = resolve_criteria_range(f, ctx, ev, r0, c0, r1, c1, sheet, crit); err.is_error())
        return err;

    bool have_sum = f.args.size() == 3;
    int sr0 = r0, sc0 = c0, sr1 = r1, sc1 = c1;
    std::string ssheet = sheet;
    if (have_sum && !arg_bounds(*f.args[2], ctx, sr0, sc0, sr1, sc1, ssheet))
        return Value::error(FormulaError::VALUE);

    double total = 0.0;
    int count = 0;
    for (int r = r0; r <= r1; ++r)
        for (int c = c0; c <= c1; ++c) {
            Value cell = range_cell(ctx, sheet, r, c);
            if (cell.is_empty() || !match_criterion(cell, crit)) continue;
            Value target = have_sum
                ? range_cell(ctx, ssheet, sr0 + (r - r0), sc0 + (c - c0))
                : cell;
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

// ── Lookup functions (VLOOKUP / MATCH / INDEX) ────────────────────────────────

static Value fn_vlookup(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() < 3 || f.args.size() > 4) return Value::error(FormulaError::VALUE);
    Value key = ev.eval(*f.args[0], ctx);
    if (key.is_error()) return key;
    int r0, c0, r1, c1;
    std::string sheet;
    if (!arg_bounds(*f.args[1], ctx, r0, c0, r1, c1, sheet)) return Value::error(FormulaError::VALUE);
    double cidx;
    if (!eval_num_arg(f, 2, ctx, ev, cidx)) return Value::error(FormulaError::VALUE);
    int col = static_cast<int>(cidx);
    if (col < 1 || c0 + col - 1 > c1) return Value::error(FormulaError::REF);

    bool approx = true;
    if (f.args.size() == 4) approx = is_truthy(ev.eval(*f.args[3], ctx));

    int target_col = c0 + col - 1;
    if (!approx) {
        for (int r = r0; r <= r1; ++r)
            if (values_equal(range_cell(ctx, sheet, r, c0), key))
                return range_cell(ctx, sheet, r, target_col);
        return Value::error(FormulaError::NA);
    }
    // Approximate match: largest first-column value <= key, assuming ascending order.
    int best = approx_match(r1 - r0 + 1,
                             [&](int i) { return range_cell(ctx, sheet, r0 + i, c0); },
                             key, /*ascending=*/true);
    return best < 0 ? Value::error(FormulaError::NA)
                    : range_cell(ctx, sheet, r0 + best, target_col);
}

static Value fn_match(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() < 2 || f.args.size() > 3) return Value::error(FormulaError::VALUE);
    Value key = ev.eval(*f.args[0], ctx);
    if (key.is_error()) return key;
    int r0, c0, r1, c1;
    std::string sheet;
    if (!arg_bounds(*f.args[1], ctx, r0, c0, r1, c1, sheet)) return Value::error(FormulaError::VALUE);
    if (r0 != r1 && c0 != c1) return Value::error(FormulaError::NA);  // needs a 1-D vector

    int match_type = 1;
    if (f.args.size() == 3) {
        double m;
        if (!eval_num_arg(f, 2, ctx, ev, m)) return Value::error(FormulaError::VALUE);
        match_type = static_cast<int>(m);
    }
    int len = (r0 == r1) ? (c1 - c0 + 1) : (r1 - r0 + 1);
    auto at = [&](int i) {
        return (r0 == r1) ? range_cell(ctx, sheet, r0, c0 + i)
                          : range_cell(ctx, sheet, r0 + i, c0);
    };
    if (match_type == 0) {
        for (int i = 0; i < len; ++i)
            if (values_equal(at(i), key)) return Value::number(i + 1);
        return Value::error(FormulaError::NA);
    }
    int best = approx_match(len, at, key, /*ascending=*/match_type == 1);
    return best < 0 ? Value::error(FormulaError::NA) : Value::number(best + 1);
}

static Value fn_index(const FuncCallExpr& f, const EvalContext& ctx, const Evaluator& ev) {
    if (f.args.size() < 2 || f.args.size() > 3) return Value::error(FormulaError::VALUE);
    int r0, c0, r1, c1;
    std::string sheet;
    if (!arg_bounds(*f.args[0], ctx, r0, c0, r1, c1, sheet)) return Value::error(FormulaError::VALUE);
    double rn = 0, cn = 0;
    if (!eval_num_arg(f, 1, ctx, ev, rn)) return Value::error(FormulaError::VALUE);
    bool have_col = f.args.size() == 3;
    if (have_col && !eval_num_arg(f, 2, ctx, ev, cn)) return Value::error(FormulaError::VALUE);

    int nrows = r1 - r0 + 1, ncols = c1 - c0 + 1;
    int ri = static_cast<int>(rn), ci = static_cast<int>(cn);
    if (!have_col && nrows == 1 && ncols > 1) { ci = ri; ri = 1; }  // single row: index picks column
    if (ci == 0) ci = 1;
    if (ri < 1 || ri > nrows || ci < 1 || ci > ncols) return Value::error(FormulaError::REF);
    return range_cell(ctx, sheet, r0 + ri - 1, c0 + ci - 1);
}

// ── Dispatch table ────────────────────────────────────────────────────────────

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
    { "SPARKLINE",   fn_sparkline   },
    { "COUNTIF",     fn_countif     },
    { "SUMIF",       fn_sumif       },
    { "AVERAGEIF",   fn_averageif   },
    { "VLOOKUP",     fn_vlookup     },
    { "MATCH",       fn_match       },
    { "INDEX",       fn_index       },
    { "IFS",         fn_ifs         },
    { "IFNA",        fn_ifna        },
};

BuiltinFn lookup_builtin(const std::string& name) {
    // Built once on first call (thread-safe magic-static init — evaluation
    // runs on both the UI thread and CalcCache's background thread); a
    // hash lookup keeps this O(1) as the table grows instead of scanning.
    static const std::unordered_map<std::string, BuiltinFn> table(
        std::begin(k_functions), std::end(k_functions));
    auto it = table.find(name);
    return it == table.end() ? nullptr : it->second;
}
