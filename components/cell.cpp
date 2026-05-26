#include "cell.hpp"

Cell::Cell(std::string value) : m_value(std::move(value)) {}

const std::string& Cell::value()   const { return m_value; }
void               Cell::set_value(std::string v) { m_value = std::move(v); }
const std::string& Cell::display() const { return m_value; }
bool               Cell::empty()   const { return m_value.empty(); }
