#pragma once
#include <string>

enum class FormulaError { NULL_, DIV0, VALUE, REF, NAME, NUM, NA };

class Value {
public:
    enum class Type { NUMBER, STRING, BOOLEAN, ERROR, EMPTY };

    static Value number(double v);
    static Value string(std::string v);
    static Value boolean(bool v);
    static Value error(FormulaError e);
    static Value empty();

    Type               type()       const noexcept { return m_type; }
    double             as_number()  const noexcept { return m_number; }
    const std::string& as_string()  const noexcept { return m_string; }
    bool               as_boolean() const noexcept { return m_boolean; }
    FormulaError       as_error()   const noexcept { return m_error; }

    bool is_number()  const noexcept { return m_type == Type::NUMBER; }
    bool is_string()  const noexcept { return m_type == Type::STRING; }
    bool is_boolean() const noexcept { return m_type == Type::BOOLEAN; }
    bool is_error()   const noexcept { return m_type == Type::ERROR; }
    bool is_empty()   const noexcept { return m_type == Type::EMPTY; }

    bool        to_number(double& out) const;
    std::string to_display()           const;

private:
    Type         m_type    = Type::EMPTY;
    double       m_number  = 0.0;
    std::string  m_string;
    bool         m_boolean = false;
    FormulaError m_error   = FormulaError::NULL_;
};
