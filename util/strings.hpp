#pragma once
#include <cctype>
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

// Direction word in a sort spec ("B desc, A"): shared by the interactive
// :sort command and the headless --sort flag so they accept the same forms.
inline bool is_desc_token(const std::string& word) {
    const std::string l = to_lower(word);
    return l == "desc" || l == "descending" || l == "d";
}

}  // namespace tuix
