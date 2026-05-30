#include "workbook.hpp"

#include <algorithm>

void Workbook::set_active(int i) {
    if (i < 0 || i >= size()) return;
    m_active = i;
}

int Workbook::add(Sheet s) {
    m_sheets.push_back(std::move(s));
    return size() - 1;
}

void Workbook::remove(int i) {
    if (size() <= 1 || i < 0 || i >= size()) return;
    m_sheets.erase(m_sheets.begin() + i);
    if (m_active >= size()) m_active = size() - 1;
}

void Workbook::rename(int i, std::string name) {
    if (i < 0 || i >= size()) return;
    m_sheets[i].name = std::move(name);
}

std::string Workbook::unique_name(std::string hint) const {
    auto taken = [&](const std::string& n) {
        return std::any_of(m_sheets.begin(), m_sheets.end(),
                           [&](const Sheet& s) { return s.name == n; });
    };
    if (!taken(hint)) return hint;
    for (int n = 2; ; ++n) {
        std::string candidate = hint + std::to_string(n);
        if (!taken(candidate)) return candidate;
    }
}

void Workbook::clear() {
    m_sheets.clear();
    m_active = 0;
}
