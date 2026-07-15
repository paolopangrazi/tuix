#include "cell.hpp"

Cell::Cell(std::string value) : m_value(std::move(value)) {}

const std::string& Cell::value()   const { return m_value; }
void               Cell::set_value(std::string v) { m_value = std::move(v); }
bool               Cell::is_formula() const { return !m_value.empty() && m_value[0] == '='; }
