#include "session.hpp"

#include "body.hpp"
#include "loader/csv_loader.hpp"
#include "loader/xlsx_loader.hpp"
#include "writer/csv_writer.hpp"

#include <filesystem>
#include <algorithm>

namespace {
std::string format_size(uintmax_t bytes) {
    if (bytes < 1024)        return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
    return std::to_string(bytes / (1024 * 1024)) + " MB";
}

std::string delimiter_name(char d) {
    switch (d) {
        case ',':  return "comma";
        case ';':  return "semicolon";
        case '\t': return "tab";
        case '|':  return "pipe";
        default:   return std::string(1, d);
    }
}
}

Session::Session(Body& body) : m_body(body) {}

static bool is_xlsx(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".xlsx";
}

void Session::load(const std::string& path) {
    try {
        CsvData data;
        if (is_xlsx(path)) {
            data = XlsxLoader::load(path);
            m_delim = '\0';
        } else {
            data = CsvLoader::load(path);
            m_delim = data.delimiter;
        }
        m_body.grid().load(data);
        m_path = path;
        m_info = build_info(path);
    } catch (const std::exception&) {}
}

void Session::write(const std::string& path) {
    try {
        auto data = m_body.grid().to_csv_data(m_delim);
        CsvWriter::save(path, data);
        m_path = path;
        m_info = build_info(path);
    } catch (const std::exception&) {}
}

std::string Session::resolve(const std::string& s) const {
    std::filesystem::path p = s;
    if (p.is_relative()) {
        std::filesystem::path base = m_path.empty()
            ? std::filesystem::current_path()
            : std::filesystem::path(m_path).parent_path();
        p = base / p;
    }
    return p.string();
}

std::string Session::dir_of_current() const {
    return m_path.empty()
        ? std::filesystem::current_path().string()
        : std::filesystem::path(m_path).parent_path().string();
}

std::string Session::build_info(const std::string& path) const {
    std::string name = std::filesystem::path(path).filename().string();
    std::string size;
    try { size = format_size(std::filesystem::file_size(path)); } catch (...) { size = "?"; }
    std::string info = name
        + "  |  " + size
        + "  |  " + std::to_string(m_body.grid().cols()) + " cols"
        + "  |  " + std::to_string(m_body.grid().rows()) + " rows";
    if (m_delim != '\0')
        info += "  |  " + delimiter_name(m_delim);
    return info;
}
