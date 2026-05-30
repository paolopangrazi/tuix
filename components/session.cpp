#include "session.hpp"

#include "body.hpp"
#include "loader/csv_loader.hpp"
#include "loader/xlsx_loader.hpp"
#include "writer/csv_writer.hpp"
#include "writer/xlsx_writer.hpp"

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

bool is_xlsx(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".xlsx";
}

// Build a Sheet by loading SheetData through a temporary Grid-style fill.
// Inactive sheets need cell values for save, so we materialize the SheetData
// straight into a Sheet's cell grid.
Sheet sheet_from_data(std::string name, const SheetData& data) {
    Sheet s;
    s.name = std::move(name);
    const int nrows = std::max<int>(1, static_cast<int>(data.rows.size()));
    const int ncols = std::max<int>(1, static_cast<int>(data.headers.size()));
    s.cells.assign(nrows, std::vector<Cell>(ncols));
    s.col_names.resize(ncols);
    s.col_widths.assign(ncols, 12);
    for (int c = 0; c < ncols; ++c)
        s.col_names[c] = (c < (int)data.headers.size()) ? data.headers[c] : std::string{};
    for (int r = 0; r < (int)data.rows.size(); ++r)
        for (int c = 0; c < std::min(ncols, (int)data.rows[r].size()); ++c)
            s.cells[r][c].set_value(data.rows[r][c]);
    return s;
}

SheetData data_from_sheet(const Sheet& s) {
    SheetData d;
    d.delimiter = ',';
    d.headers   = s.col_names;
    const int nrows = static_cast<int>(s.cells.size());
    const int ncols = nrows ? static_cast<int>(s.cells[0].size()) : 0;
    d.rows.resize(nrows);
    for (int r = 0; r < nrows; ++r) {
        d.rows[r].resize(ncols);
        for (int c = 0; c < ncols; ++c)
            d.rows[r][c] = s.cells[r][c].value();
    }
    return d;
}

} // namespace

Session::Session(Body& body) : m_body(body) {}

Sheet Session::new_empty_sheet(std::string name) const {
    Sheet s;
    s.name = std::move(name);
    constexpr int kRows = 50, kCols = 26;
    s.cells.assign(kRows, std::vector<Cell>(kCols));
    s.col_names.resize(kCols);
    for (int c = 0; c < kCols; ++c) {
        std::string letter;
        int n = c;
        while (true) { letter.insert(letter.begin(), 'A' + (n % 26)); if (n < 26) break; n = n / 26 - 1; }
        s.col_names[c] = letter;
    }
    s.col_widths.assign(kCols, 12);
    return s;
}

void Session::init_empty_if_needed() {
    if (m_workbook.size() > 0) return;
    m_workbook.add(new_empty_sheet("Sheet1"));
    m_workbook.set_active(0);
    m_body.grid().load_from(m_workbook.active());
}

bool Session::is_xlsx_workflow() const {
    if (m_path.empty()) return true;
    return is_xlsx(m_path);
}

void Session::load(const std::string& path) {
    try {
        Workbook fresh;
        char     delim = m_delim;
        if (is_xlsx(path)) {
            WorkbookData wb = XlsxLoader::load_workbook(path);
            if (wb.sheets.empty())
                fresh.add(new_empty_sheet("Sheet1"));
            else
                for (auto& [name, data] : wb.sheets)
                    fresh.add(sheet_from_data(name, data));
            delim = '\0';
        } else {
            SheetData data = CsvLoader::load(path);
            delim = data.delimiter;
            fresh.add(sheet_from_data(
                std::filesystem::path(path).stem().string(), data));
        }
        fresh.set_active(0);
        m_workbook = std::move(fresh);
        m_delim    = delim;
        m_body.grid().load_from(m_workbook.active());
        m_path = path;
        refresh_info();
    } catch (const std::exception&) {}
}

void Session::write(const std::string& path) {
    try {
        // Flush on-screen edits into the active sheet before persisting.
        m_body.grid().save_to(m_workbook.active());

        if (is_xlsx(path)) {
            std::vector<std::pair<std::string, SheetData>> sheets;
            sheets.reserve(m_workbook.size());
            for (int i = 0; i < m_workbook.size(); ++i) {
                const Sheet& s = m_workbook.at(i);
                sheets.emplace_back(
                    s.name.empty() ? ("Sheet" + std::to_string(i + 1)) : s.name,
                    data_from_sheet(s));
            }
            XlsxWriter::save_workbook(path, sheets);
            m_delim = '\0';
        } else {
            char delim = (m_delim != '\0') ? m_delim : ',';
            auto data = m_body.grid().to_csv_data(delim);
            CsvWriter::save(path, data);
            m_delim = delim;
        }
        m_path = path;
        refresh_info();
    } catch (const std::exception&) {}
}

void Session::switch_to(int i) {
    if (i < 0 || i >= m_workbook.size() || i == m_workbook.active_index()) return;
    m_body.grid().save_to(m_workbook.active());
    m_workbook.set_active(i);
    m_body.grid().load_from(m_workbook.active());
    refresh_info();
}

void Session::add_sheet() {
    m_body.grid().save_to(m_workbook.active());
    int idx = m_workbook.add(new_empty_sheet(m_workbook.unique_name("Sheet")));
    m_workbook.set_active(idx);
    m_body.grid().load_from(m_workbook.active());
    refresh_info();
}

void Session::rename_active(std::string name) {
    if (name.empty()) return;
    const int idx = m_workbook.active_index();
    if (m_workbook.at(idx).name == name) return;
    m_workbook.rename(idx, m_workbook.unique_name(std::move(name)));
    refresh_info();
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

void Session::refresh_info() {
    m_info = build_info(m_path);
}

std::string Session::build_info(const std::string& path) const {
    if (path.empty()) return {};
    std::string name = std::filesystem::path(path).filename().string();
    std::string size;
    try { size = format_size(std::filesystem::file_size(path)); } catch (...) { size = "?"; }
    std::string info = name
        + "  |  " + size
        + "  |  " + std::to_string(m_body.grid().cols()) + " cols"
        + "  |  " + std::to_string(m_body.grid().rows()) + " rows";
    if (m_delim != '\0')
        info += "  |  " + delimiter_name(m_delim);
    if (m_workbook.size() > 1)
        info += "  |  " + std::to_string(m_workbook.size()) + " sheets";
    return info;
}
