#pragma once
#include <string>

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

private:
    Body&       m_body;
    std::string m_path;
    char        m_delim = ',';
    std::string m_info;

    std::string build_info(const std::string& path) const;
};
