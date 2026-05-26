#pragma once
#include <string>

enum class TokenType {
    NUMBER, STRING, BOOLEAN,
    CELL_REF,   // e.g. A1, $B$2
    IDENT,      // function name or unknown
    COLON, COMMA, SEMICOLON,
    LPAREN, RPAREN,
    PLUS, MINUS, MUL, DIV, POW, CONCAT,
    EQ, NEQ, LT, GT, LTE, GTE,
    END, ERROR
};

struct CellCoord {
    int  col = 0, row = 0;
    bool abs_col = false, abs_row = false;
};

struct Token {
    TokenType   type    = TokenType::END;
    std::string text;
    double      number  = 0.0;
    bool        boolean = false;
    CellCoord   coord;
};
