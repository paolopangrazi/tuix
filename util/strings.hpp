#pragma once
#include <cctype>
#include <functional>
#include <string>

// Small ASCII case helpers shared across the codebase (sheet names, column
// labels, search, function names). Spreadsheet matching is ASCII-only by
// convention, so no locale/UTF-8 handling here.
namespace tuix {

inline char lower_ch(char c) { return (char)std::tolower((unsigned char)c); }
inline char upper_ch(char c) { return (char)std::toupper((unsigned char)c); }

inline std::string to_lower(std::string s) {
    for (char& c : s) c = lower_ch(c);
    return s;
}

inline std::string to_upper(std::string s) {
    for (char& c : s) c = upper_ch(c);
    return s;
}

inline bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (lower_ch(a[i]) != lower_ch(b[i])) return false;
    return true;
}

// Case-insensitive three-way text comparison: -1/0/1. Consistent with
// iequals (compare_ci(a,b) == 0 iff iequals(a,b)) — the canonical rule for
// case-insensitive text ordering (comparison operators, MATCH, VLOOKUP, ...).
inline int compare_ci(const std::string& a, const std::string& b) {
    const std::string la = to_lower(a), lb = to_lower(b);
    return la < lb ? -1 : (la > lb ? 1 : 0);
}

// Direction word in a sort spec ("B desc, A"): shared by the interactive
// :sort command and the headless --sort flag so they accept the same forms.
inline bool is_desc_token(const std::string& word) {
    const std::string l = to_lower(word);
    return l == "desc" || l == "descending" || l == "d";
}

// Appends a numeric suffix to `hint` until `taken` returns false. `first_suffix`
// is the number tried first (e.g. sheet names start at 2 for "Sheet2"; column
// rename collisions start at 1 for "A1").
inline std::string make_unique_name(const std::string& hint, int first_suffix,
                                     const std::function<bool(const std::string&)>& taken) {
    if (!taken(hint)) return hint;
    for (int n = first_suffix; ; ++n) {
        std::string candidate = hint + std::to_string(n);
        if (!taken(candidate)) return candidate;
    }
}

}  // namespace tuix
