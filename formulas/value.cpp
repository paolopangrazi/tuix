#include "value.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>

#include "util/numbers.hpp"

Value Value::number(double v)      { Value r; r.m_type = Type::NUMBER;  r.m_number  = v;            return r; }
Value Value::string(std::string v) { Value r; r.m_type = Type::STRING;  r.m_string  = std::move(v); return r; }
Value Value::boolean(bool v)       { Value r; r.m_type = Type::BOOLEAN; r.m_boolean = v;            return r; }
Value Value::error(FormulaError e) { Value r; r.m_type = Type::ERROR;   r.m_error   = e;            return r; }
Value Value::empty()               { return Value{}; }

bool Value::to_number(double& out) const {
    switch (m_type) {
        case Type::NUMBER:  out = m_number;                    return true;
        case Type::BOOLEAN: out = m_boolean ? 1.0 : 0.0;      return true;
        case Type::EMPTY:   out = 0.0;                         return true;
        case Type::STRING:
            return tuix::parse_number(m_string, out);   // whole string, no "12abc"
        case Type::ERROR:   return false;
    }
    return false;
}

std::string Value::to_display() const {
    switch (m_type) {
        case Type::NUMBER: {
            double n = m_number;
            if (std::isnan(n) || std::isinf(n)) return "#NUM!";
            if (n == std::floor(n) && std::abs(n) < 1e15)
                return std::to_string(static_cast<long long>(n));
            // 15 significant digits: enough to round-trip typical inputs
            // without the default precision's 6-digit truncation (which turned
            // 1234567.89 into "1.23457e+06").
            std::ostringstream ss;
            ss << std::setprecision(15) << n;
            return ss.str();
        }
        case Type::STRING:  return m_string;
        case Type::BOOLEAN: return m_boolean ? "TRUE" : "FALSE";
        case Type::ERROR:
            switch (m_error) {
                case FormulaError::NULL_: return "#NULL!";
                case FormulaError::DIV0:  return "#DIV/0!";
                case FormulaError::VALUE: return "#VALUE!";
                case FormulaError::REF:   return "#REF!";
                case FormulaError::NAME:  return "#NAME?";
                case FormulaError::NUM:   return "#NUM!";
                case FormulaError::NA:    return "#N/A";
            }
            break;
        case Type::EMPTY: return "";
    }
    return "";
}
