#include "lexer.hpp"
#include <cctype>

Lexer::Lexer(std::string input) : m_input(std::move(input)) {}

char Lexer::peek(int offset) const {
    size_t p = m_pos + static_cast<size_t>(offset);
    return p < m_input.size() ? m_input[p] : '\0';
}

char Lexer::advance() {
    return m_pos < m_input.size() ? m_input[m_pos++] : '\0';
}

void Lexer::skip_whitespace() {
    while (m_pos < m_input.size() && std::isspace((unsigned char)m_input[m_pos]))
        ++m_pos;
}

Token Lexer::read_number() {
    size_t start = m_pos;
    while (m_pos < m_input.size() && std::isdigit((unsigned char)m_input[m_pos]))
        ++m_pos;
    if (m_pos < m_input.size() && m_input[m_pos] == '.') {
        ++m_pos;
        while (m_pos < m_input.size() && std::isdigit((unsigned char)m_input[m_pos]))
            ++m_pos;
    }
    if (m_pos < m_input.size() && (m_input[m_pos] == 'e' || m_input[m_pos] == 'E')) {
        ++m_pos;
        if (m_pos < m_input.size() && (m_input[m_pos] == '+' || m_input[m_pos] == '-'))
            ++m_pos;
        while (m_pos < m_input.size() && std::isdigit((unsigned char)m_input[m_pos]))
            ++m_pos;
    }
    Token t;
    t.type   = TokenType::NUMBER;
    t.text   = m_input.substr(start, m_pos - start);
    t.number = std::stod(t.text);
    return t;
}

Token Lexer::read_string() {
    advance(); // consume opening "
    std::string s;
    while (m_pos < m_input.size()) {
        char c = advance();
        if (c == '"') {
            if (peek() == '"') { s += '"'; advance(); } // "" → "
            else break;
        } else {
            s += c;
        }
    }
    Token t;
    t.type = TokenType::STRING;
    t.text = s;
    return t;
}

static int col_letters_to_index(const std::string& s) {
    int col = 0;
    for (char c : s)
        col = col * 26 + (std::toupper((unsigned char)c) - 'A' + 1);
    return col - 1;
}

Token Lexer::read_ident_or_ref() {
    bool abs_col = false;
    if (peek() == '$') { abs_col = true; advance(); }

    size_t letter_start = m_pos;
    while (m_pos < m_input.size() && std::isalpha((unsigned char)m_input[m_pos]))
        ++m_pos;
    std::string letters = m_input.substr(letter_start, m_pos - letter_start);

    if (letters.empty()) {
        Token t; t.type = TokenType::ERROR; t.text = "$"; return t;
    }

    bool abs_row = false;
    size_t saved = m_pos;
    if (peek() == '$') { abs_row = true; ++m_pos; }

    if (m_pos < m_input.size() && std::isdigit((unsigned char)m_input[m_pos])) {
        size_t row_start = m_pos;
        while (m_pos < m_input.size() && std::isdigit((unsigned char)m_input[m_pos]))
            ++m_pos;
        std::string row_str = m_input.substr(row_start, m_pos - row_start);
        Token t;
        t.type          = TokenType::CELL_REF;
        t.text          = (abs_col ? "$" : "") + letters + (abs_row ? "$" : "") + row_str;
        t.coord.col     = col_letters_to_index(letters);
        t.coord.row     = std::stoi(row_str) - 1;
        t.coord.abs_col = abs_col;
        t.coord.abs_row = abs_row;
        return t;
    }

    // Not a cell ref — backtrack the $ we may have consumed
    m_pos = saved;

    std::string upper = letters;
    for (char& c : upper) c = (char)std::toupper((unsigned char)c);

    Token t;
    if (upper == "TRUE")  { t.text = letters; t.type = TokenType::BOOLEAN; t.boolean = true;  return t; }
    if (upper == "FALSE") { t.text = letters; t.type = TokenType::BOOLEAN; t.boolean = false; return t; }
    t.type = TokenType::IDENT;
    t.text = upper;  // function names are matched upper-cased
    return t;
}

std::vector<Token> Lexer::tokenize() {
    if (!m_input.empty() && m_input[0] == '=')
        ++m_pos;

    std::vector<Token> tokens;
    while (true) {
        skip_whitespace();
        if (m_pos >= m_input.size()) {
            tokens.push_back({ TokenType::END });
            break;
        }

        char c = peek();
        if (std::isdigit((unsigned char)c) || (c == '.' && std::isdigit((unsigned char)peek(1)))) {
            tokens.push_back(read_number());
        } else if (c == '"') {
            tokens.push_back(read_string());
        } else if (c == '$' || std::isalpha((unsigned char)c)) {
            tokens.push_back(read_ident_or_ref());
        } else {
            advance();
            Token t;
            t.text = std::string(1, c);
            switch (c) {
                case '+': t.type = TokenType::PLUS;    break;
                case '-': t.type = TokenType::MINUS;   break;
                case '*': t.type = TokenType::MUL;     break;
                case '/': t.type = TokenType::DIV;     break;
                case '^': t.type = TokenType::POW;     break;
                case '&': t.type = TokenType::CONCAT;  break;
                case '(': t.type = TokenType::LPAREN;  break;
                case ')': t.type = TokenType::RPAREN;  break;
                case ',': t.type = TokenType::COMMA;   break;
                case ';': t.type = TokenType::SEMICOLON; break;
                case ':': t.type = TokenType::COLON;   break;
                case '=': t.type = TokenType::EQ;      break;
                case '<':
                    if (peek() == '>') { advance(); t.type = TokenType::NEQ; t.text = "<>"; }
                    else if (peek() == '=') { advance(); t.type = TokenType::LTE; t.text = "<="; }
                    else t.type = TokenType::LT;
                    break;
                case '>':
                    if (peek() == '=') { advance(); t.type = TokenType::GTE; t.text = ">="; }
                    else t.type = TokenType::GT;
                    break;
                default:  t.type = TokenType::ERROR;   break;
            }
            tokens.push_back(t);
        }
    }
    return tokens;
}
