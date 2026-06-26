#pragma once
#include <string>

#include "ast.hpp"
#include "eval_context.hpp"
#include "value.hpp"

class Evaluator;

// Signature every built-in spreadsheet function implements: it receives the
// call node, the evaluation context, and the Evaluator (to recurse into args).
using BuiltinFn = Value(*)(const FuncCallExpr&, const EvalContext&, const Evaluator&);

// Look up a built-in by name (the parser preserves the source casing, so the
// table is matched case-sensitively). Returns nullptr when none matches, which
// the evaluator turns into a #NAME? error.
BuiltinFn lookup_builtin(const std::string& name);
