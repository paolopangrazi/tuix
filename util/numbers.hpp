#pragma once
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>

#include "strings.hpp"

// Shared numeric-cell helpers so every consumer — formula evaluation, sorting,
// column stats, suggestions, headless filters — agrees on what counts as a
// number and how mixed values order.
namespace tuix {

// Strict "the whole string is one number" parse. Surrounding whitespace is
// allowed; stod's extensions (hex, inf, nan, trailing junk like "12abc") are
// not — by spreadsheet convention those are text.
inline bool parse_number(const std::string& s, double& out) {
    const char* p = s.c_str();
    while (*p && std::isspace((unsigned char)*p)) ++p;
    const char* q = p;
    if (*q == '+' || *q == '-') ++q;
    if (!std::isdigit((unsigned char)*q) && *q != '.') return false;  // inf/nan/empty
    if (q[0] == '0' && (q[1] == 'x' || q[1] == 'X'))   return false;  // hex
    char* end = nullptr;
    const double v = std::strtod(p, &end);
    if (end == p) return false;
    while (*end && std::isspace((unsigned char)*end)) ++end;
    if (*end || !std::isfinite(v)) return false;
    out = v;
    return true;
}

// Order two mixed cell values: numbers numerically (and before text), text
// case-insensitively. an/bn say whether da/db hold numbers; sa/sb are the text
// forms compared when they don't. Blank handling stays with the caller.
inline int cmp_mixed(bool an, double da, const std::string& sa,
                     bool bn, double db, const std::string& sb) {
    if (an && bn) return (da < db) ? -1 : (da > db ? 1 : 0);
    if (an != bn) return an ? -1 : 1;
    const std::string la = to_lower(sa), lb = to_lower(sb);
    return (la < lb) ? -1 : (la > lb ? 1 : 0);
}

}  // namespace tuix
