#pragma once
#include <string>
#include <ftxui/dom/elements.hpp>

class StatusBar {
public:
    explicit StatusBar(std::string message = "");

    void               set_message(std::string msg);
    const std::string& message() const;
    ftxui::Element     render()  const;

private:
    std::string m_message;
};
