#include "xlsx_writer.hpp"
#include <OpenXLSX.hpp>

namespace XlsxWriter {

namespace {

void write_sheet(OpenXLSX::XLWorksheet wks, const SheetData& data) {
    for (size_t c = 0; c < data.headers.size(); ++c)
        wks.cell(1, static_cast<uint16_t>(c + 1)).value() = data.headers[c];
    for (size_t r = 0; r < data.rows.size(); ++r) {
        const auto& row = data.rows[r];
        for (size_t c = 0; c < row.size(); ++c)
            wks.cell(static_cast<uint32_t>(r + 2), static_cast<uint16_t>(c + 1)).value() = row[c];
    }
}

} // namespace

void save(const std::string& path, const SheetData& data) {
    OpenXLSX::XLDocument doc;
    doc.create(path, OpenXLSX::XLForceOverwrite);
    write_sheet(doc.workbook().worksheet(1), data);
    doc.save();
    doc.close();
}

void save_workbook(const std::string& path,
                   const std::vector<std::pair<std::string, SheetData>>& sheets) {
    OpenXLSX::XLDocument doc;
    doc.create(path, OpenXLSX::XLForceOverwrite);
    auto wb = doc.workbook();

    // doc.create() seeds one default worksheet (e.g. "Sheet1"). Rename it to
    // the first input sheet, then add the rest.
    if (sheets.empty()) { doc.save(); doc.close(); return; }

    const auto seed = wb.worksheetNames();
    const std::string& seed_name = seed.empty() ? std::string{} : seed.front();

    auto rename_or_add = [&](size_t i) {
        const auto& [name, data] = sheets[i];
        if (i == 0 && !seed_name.empty()) {
            if (name != seed_name) wb.worksheet(seed_name).setName(name);
        } else {
            wb.addWorksheet(name);
        }
        write_sheet(wb.worksheet(name), data);
    };
    for (size_t i = 0; i < sheets.size(); ++i) rename_or_add(i);

    doc.save();
    doc.close();
}

} // namespace XlsxWriter
