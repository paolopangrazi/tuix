#pragma once

#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "formulas/evaluator.hpp"

// Shared helpers for tests that evaluate formulas against an EvalContext and
// compare the resulting display string.

inline Value eval(const std::string& formula, const EvalContext& ctx) {
    return Evaluator::evaluate_formula(formula, ctx);
}

inline std::string disp(const std::string& formula, const EvalContext& ctx) {
    return eval(formula, ctx).to_display();
}

struct EvalCase {
    std::string formula;
    std::string expected;
};

inline void run_eval_cases(const std::vector<EvalCase>& cases, const EvalContext& ctx) {
    for (const auto& c : cases) {
        CAPTURE(c.formula);
        CHECK(disp(c.formula, ctx) == c.expected);
    }
}
