#pragma once
#include "token.hpp"
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(std::string input);
    std::vector<Token> tokenize();

private:
    std::string m_input;
    size_t      m_pos = 0;

    char peek(int offset = 0) const;
    char advance();
    void skip_whitespace();

    Token read_number();
    Token read_string();
    Token read_ident_or_ref();
};
