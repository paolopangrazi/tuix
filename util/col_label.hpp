#pragma once
#include <cctype>
#include <optional>
#include <string>
#include <utility>

// Spreadsheet-style column label for a zero-based column index:
//   0 → "A", 25 → "Z", 26 → "AA", 27 → "AB", …
inline std::string col_letter(int c) {
    std::string s;
    do { s = char('A' + c % 26) + s; c = c / 26 - 1; } while (c >= 0);
    return s;
}

// Parse a bare column label ("B", "AA") into a zero-based column index.
// Returns nullopt unless the whole string is column letters (no digits).
inline std::optional<int> parse_col_label(const std::string& s) {
    if (s.empty()) return std::nullopt;
    int col = 0;
    for (char ch : s) {
        if (!std::isalpha((unsigned char)ch)) return std::nullopt;
        col = col * 26 + (std::toupper((unsigned char)ch) - 'A' + 1);
    }
    return col - 1;
}

// Parse an A1 cell address ("B12") into a zero-based (row, col).
// Returns nullopt unless the whole string is column letters followed by a
// non-zero row number — so "w", "wq", "1", "A" and "e file" all reject.
inline std::optional<std::pair<int, int>> parse_a1(const std::string& s) {
    size_t i = 0;
    int col = 0;
    while (i < s.size() && std::isalpha((unsigned char)s[i])) {
        col = col * 26 + (std::toupper((unsigned char)s[i]) - 'A' + 1);
        ++i;
    }
    if (i == 0) return std::nullopt;            // no column letters
    int row = 0; size_t digits = 0;
    while (i < s.size() && std::isdigit((unsigned char)s[i])) {
        row = row * 10 + (s[i] - '0'); ++i; ++digits;
    }
    if (digits == 0 || i != s.size() || row == 0) return std::nullopt;
    return std::make_pair(row - 1, col - 1);
}
