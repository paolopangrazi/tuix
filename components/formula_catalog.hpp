#pragma once
#include <string>

// Catalog of built-in formulas used by the autocomplete popup and the editing
// hints. The Grid renders/filters these; the data itself lives here so both the
// render and input code can share one definition.
struct FormulaInfo { const char* name; const char* sig; const char* desc; };
extern const FormulaInfo k_formulas[];
extern const int         k_formulas_count;

// The trailing run of alphabetic characters in `buf` — the token the
// autocomplete is currently matching against (e.g. "=SU" → "SU").
std::string ac_prefix(const std::string& buf);

// Whether the formula autocomplete popup should be offered for `buf`: it must
// start with '=' and either be just "=" or end on an alphabetic character.
bool should_show_ac(const std::string& buf);
