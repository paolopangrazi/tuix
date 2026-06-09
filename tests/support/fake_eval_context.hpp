#pragma once
#include <map>
#include <utility>

#include "formulas/eval_context.hpp"

// A minimal in-memory EvalContext so the formula pipeline can be tested without
// a Grid (and therefore without FTXUI or the background calc thread). Cells
// default to empty; populate the ones a test needs with set().
class FakeEvalContext : public EvalContext {
public:
    explicit FakeEvalContext(int rows = 100, int cols = 26)
        : m_rows(rows), m_cols(cols) {}

    int rows() const override { return m_rows; }
    int cols() const override { return m_cols; }

    Value cell_value(int row, int col) const override {
        auto it = m_cells.find({row, col});
        return it == m_cells.end() ? Value::empty() : it->second;
    }

    FakeEvalContext& set(int row, int col, Value v) {
        m_cells[{row, col}] = std::move(v);
        return *this;
    }

private:
    int m_rows;
    int m_cols;
    std::map<std::pair<int, int>, Value> m_cells;
};
