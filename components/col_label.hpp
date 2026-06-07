#pragma once
#include <string>

// Spreadsheet-style column label for a zero-based column index:
//   0 → "A", 25 → "Z", 26 → "AA", 27 → "AB", …
inline std::string col_letter(int c) {
    std::string s;
    do { s = char('A' + c % 26) + s; c = c / 26 - 1; } while (c >= 0);
    return s;
}
