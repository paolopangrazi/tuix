#pragma once
#include <memory>
#include <string>
#include <vector>

struct Expr {
    enum class Kind { NUMBER, STRING, BOOL, CELL_REF, RANGE, UNARY, BINARY, FUNC_CALL };
    Kind kind;
    explicit Expr(Kind k) : kind(k) {}
    virtual ~Expr() = default;
};

struct NumberExpr : Expr {
    double value;
    explicit NumberExpr(double v) : Expr(Kind::NUMBER), value(v) {}
};

struct StringExpr : Expr {
    std::string value;
    explicit StringExpr(std::string v) : Expr(Kind::STRING), value(std::move(v)) {}
};

struct BoolExpr : Expr {
    bool value;
    explicit BoolExpr(bool v) : Expr(Kind::BOOL), value(v) {}
};

struct CellRefExpr : Expr {
    int  row = 0, col = 0;
    bool abs_row = false, abs_col = false;
    CellRefExpr() : Expr(Kind::CELL_REF) {}
};

struct RangeExpr : Expr {
    CellRefExpr from, to;
    RangeExpr() : Expr(Kind::RANGE) {}
};

struct UnaryExpr : Expr {
    char                  op;
    std::unique_ptr<Expr> operand;
    UnaryExpr(char op, std::unique_ptr<Expr> operand)
        : Expr(Kind::UNARY), op(op), operand(std::move(operand)) {}
};

struct BinaryExpr : Expr {
    std::string           op;
    std::unique_ptr<Expr> lhs, rhs;
    BinaryExpr(std::string op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
        : Expr(Kind::BINARY), op(std::move(op)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
};

struct FuncCallExpr : Expr {
    std::string                        name;
    std::vector<std::unique_ptr<Expr>> args;
    FuncCallExpr() : Expr(Kind::FUNC_CALL) {}
};
