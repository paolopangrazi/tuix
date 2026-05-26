#pragma once
#include <string>

class Cell {
public:
    Cell() = default;
    explicit Cell(std::string value);

    const std::string& value()   const;
    void               set_value(std::string v);
    const std::string& display() const;
    bool               empty()   const;

private:
    std::string m_value;
};
