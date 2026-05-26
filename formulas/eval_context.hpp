#pragma once
#include "value.hpp"

class EvalContext {
public:
    virtual ~EvalContext() = default;
    virtual Value cell_value(int row, int col) const = 0;
    virtual int   rows() const = 0;
    virtual int   cols() const = 0;
};
