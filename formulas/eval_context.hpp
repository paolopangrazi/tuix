#pragma once
#include <string>

#include "value.hpp"

class EvalContext {
public:
    virtual ~EvalContext() = default;
    virtual Value cell_value(int row, int col) const = 0;
    virtual int   rows() const = 0;
    virtual int   cols() const = 0;

    // Resolve a reference qualified by a sheet name (e.g. Sheet2!A1). The
    // implementation owns bounds-checking and returns #REF! for an unknown
    // sheet or an out-of-range cell. The default knows no other sheets.
    virtual Value cell_value_in(const std::string& /*sheet*/, int /*row*/, int /*col*/) const {
        return Value::error(FormulaError::REF);
    }
};
