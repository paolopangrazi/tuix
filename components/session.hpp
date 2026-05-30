#pragma once
#include <string>

#include "workbook.hpp"

class Body;

// Holds the current document's file state (path, delimiter, displayed info)
// and brokers I/O between disk and the in-memory Grid.
class Session {
public:
    explicit Session(Body& body);

    // Silent no-ops on failure (parsing/IO errors).
    void load (const std::string& path);
    void write(const std::string& path);

    // Resolve a (possibly relative) path against the current file's directory.
    std::string resolve(const std::string& s) const;

    // Directory used for new files: parent of current_path, else $PWD.
    std::string dir_of_current() const;

    const std::string& current_path() const noexcept { return m_path; }
    char               delim()        const noexcept { return m_delim; }
    const std::string& file_info()    const noexcept { return m_info; }
    bool               has_path()     const noexcept { return !m_path.empty(); }

    // Multi-sheet API. Tab strip is shown when is_xlsx_workflow() is true:
    // xlsx file open, or no file open yet (empty/new state).
    Workbook&       workbook()       noexcept { return m_workbook; }
    const Workbook& workbook() const noexcept { return m_workbook; }
    bool            is_xlsx_workflow() const;
    void            switch_to(int sheet_index);
    void            add_sheet();
    void            rename_active(std::string name);

    // Seeds an empty workbook with a single "Sheet1" if none is loaded.
    // Call once at startup, after any argv load attempt.
    void init_empty_if_needed();

private:
    Body&       m_body;
    Workbook    m_workbook;
    std::string m_path;
    char        m_delim = ',';
    std::string m_info;

    std::string build_info(const std::string& path) const;
    void        refresh_info();
    Sheet       new_empty_sheet(std::string name) const;
};
