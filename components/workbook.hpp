#pragma once
#include <string>
#include <vector>

#include "sheet.hpp"

class Workbook {
public:
    int          size()         const noexcept { return static_cast<int>(m_sheets.size()); }
    int          active_index() const noexcept { return m_active; }

    Sheet&       active()                       { return m_sheets[m_active]; }
    const Sheet& active()       const           { return m_sheets[m_active]; }
    Sheet&       at(int i)                      { return m_sheets[i]; }
    const Sheet& at(int i)      const           { return m_sheets[i]; }

    void         set_active(int i);
    int          add(Sheet s);                  // returns new index
    void         remove(int i);                 // no-op if size() == 1
    void         rename(int i, std::string name);

    std::string  unique_name(std::string hint) const;
    void         clear();                       // empties the sheet list

private:
    std::vector<Sheet> m_sheets;
    int                m_active = 0;
};
