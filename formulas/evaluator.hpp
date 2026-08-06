#pragma once
#include "ast.hpp"
#include "eval_context.hpp"
#include "value.hpp"
#include <optional>
#include <string>
#include <vector>

class Evaluator {
public:
    Value eval(const Expr& expr, const EvalContext& ctx) const;

    // Expand an expression into a flat list of values (ranges expand to all cells).
    void collect(const Expr& expr, const EvalContext& ctx, std::vector<Value>& out) const;

    // Parse and evaluate a formula string (must start with '=').
    static Value evaluate_formula(const std::string& formula, const EvalContext& ctx);

    // Evaluate a formula string for display, e.g. building a suggestion list
    // ("try SUM(range), skip it if it errors"): nullopt on error, else the
    // result's display string. Shared by CalcCache::compute_for and
    // Grid::range_suggestions.
    static std::optional<std::string> try_formula(const std::string& formula, const EvalContext& ctx);
};
