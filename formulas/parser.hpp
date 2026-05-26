#pragma once
#include "ast.hpp"
#include "token.hpp"
#include <memory>
#include <vector>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<Expr> parse();

private:
    std::vector<Token> m_tokens;
    size_t             m_pos = 0;

    const Token& peek(int offset = 0) const;
    Token        consume();
    Token        expect(TokenType type);

    std::unique_ptr<Expr> parse_expr();
    std::unique_ptr<Expr> parse_comparison();
    std::unique_ptr<Expr> parse_concat();
    std::unique_ptr<Expr> parse_additive();
    std::unique_ptr<Expr> parse_multiplicative();
    std::unique_ptr<Expr> parse_power();
    std::unique_ptr<Expr> parse_unary();
    std::unique_ptr<Expr> parse_primary();
};
