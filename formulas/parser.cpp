#include "parser.hpp"
#include <stdexcept>

Parser::Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens)) {}

const Token& Parser::peek(int offset) const {
    size_t idx = m_pos + static_cast<size_t>(offset);
    if (idx >= m_tokens.size()) return m_tokens.back();
    return m_tokens[idx];
}

Token Parser::consume() {
    Token t = m_tokens[m_pos];
    if (m_pos + 1 < m_tokens.size()) ++m_pos;
    return t;
}

Token Parser::expect(TokenType type) {
    if (peek().type != type)
        throw std::runtime_error("Expected token, got: " + peek().text);
    return consume();
}

std::unique_ptr<Expr> Parser::parse()            { return parse_expr(); }
std::unique_ptr<Expr> Parser::parse_expr()       { return parse_comparison(); }

std::unique_ptr<Expr> Parser::parse_comparison() {
    auto lhs = parse_concat();
    while (true) {
        auto tt = peek().type;
        if (tt != TokenType::EQ  && tt != TokenType::NEQ &&
            tt != TokenType::LT  && tt != TokenType::GT  &&
            tt != TokenType::LTE && tt != TokenType::GTE)
            break;
        auto op  = consume().text;
        auto rhs = parse_concat();
        lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parse_concat() {
    auto lhs = parse_additive();
    while (peek().type == TokenType::CONCAT) {
        consume();
        auto rhs = parse_additive();
        lhs = std::make_unique<BinaryExpr>("&", std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parse_additive() {
    auto lhs = parse_multiplicative();
    while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
        auto op  = consume().text;
        auto rhs = parse_multiplicative();
        lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parse_multiplicative() {
    auto lhs = parse_power();
    while (peek().type == TokenType::MUL || peek().type == TokenType::DIV) {
        auto op  = consume().text;
        auto rhs = parse_power();
        lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<Expr> Parser::parse_power() {
    auto base = parse_unary();
    if (peek().type == TokenType::POW) {
        consume();
        auto exp = parse_power(); // right-associative
        return std::make_unique<BinaryExpr>("^", std::move(base), std::move(exp));
    }
    return base;
}

std::unique_ptr<Expr> Parser::parse_unary() {
    if (peek().type == TokenType::MINUS) {
        consume();
        return std::make_unique<UnaryExpr>('-', parse_unary());
    }
    if (peek().type == TokenType::PLUS) {
        consume();
        return parse_unary();
    }
    return parse_primary();
}

std::unique_ptr<Expr> Parser::parse_primary() {
    Token t = peek();

    if (t.type == TokenType::NUMBER) {
        consume();
        return std::make_unique<NumberExpr>(t.number);
    }
    if (t.type == TokenType::STRING) {
        consume();
        return std::make_unique<StringExpr>(t.text);
    }
    if (t.type == TokenType::BOOLEAN) {
        consume();
        return std::make_unique<BoolExpr>(t.boolean);
    }
    if (t.type == TokenType::CELL_REF) {
        consume();
        auto ref = std::make_unique<CellRefExpr>();
        ref->row     = t.coord.row;
        ref->col     = t.coord.col;
        ref->abs_row = t.coord.abs_row;
        ref->abs_col = t.coord.abs_col;
        ref->sheet   = t.coord.sheet;
        if (peek().type == TokenType::COLON) {
            consume();
            Token t2 = expect(TokenType::CELL_REF);
            auto range  = std::make_unique<RangeExpr>();
            range->from = *ref;
            range->to.row     = t2.coord.row;
            range->to.col     = t2.coord.col;
            range->to.abs_row = t2.coord.abs_row;
            range->to.abs_col = t2.coord.abs_col;
            // A qualifier on the start (Sheet2!A1:B3) covers the whole range.
            range->to.sheet   = t2.coord.sheet.empty() ? ref->sheet : t2.coord.sheet;
            return range;
        }
        return ref;
    }
    if (t.type == TokenType::IDENT) {
        consume();
        expect(TokenType::LPAREN);
        auto call  = std::make_unique<FuncCallExpr>();
        call->name = t.text; // already uppercased by lexer
        if (peek().type != TokenType::RPAREN) {
            call->args.push_back(parse_expr());
            while (peek().type == TokenType::COMMA || peek().type == TokenType::SEMICOLON) {
                consume();
                call->args.push_back(parse_expr());
            }
        }
        expect(TokenType::RPAREN);
        return call;
    }
    if (t.type == TokenType::LPAREN) {
        consume();
        auto inner = parse_expr();
        expect(TokenType::RPAREN);
        return inner;
    }

    throw std::runtime_error("Unexpected token: " + t.text);
}
