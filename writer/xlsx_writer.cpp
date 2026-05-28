#include "xlsx_writer.hpp"
#include <OpenXLSX.hpp>

namespace XlsxWriter {

void save(const std::string& path, const SheetData& data) {
    OpenXLSX::XLDocument doc;
    doc.create(path, OpenXLSX::XLForceOverwrite);

    auto wks = doc.workbook().worksheet(1);

    // Write header row
    for (size_t c = 0; c < data.headers.size(); ++c)
        wks.cell(1, static_cast<uint16_t>(c + 1)).value() = data.headers[c];

    // Write data rows
    for (size_t r = 0; r < data.rows.size(); ++r) {
        const auto& row = data.rows[r];
        for (size_t c = 0; c < row.size(); ++c)
            wks.cell(static_cast<uint32_t>(r + 2), static_cast<uint16_t>(c + 1)).value() = row[c];
    }

    doc.save();
    doc.close();
}

} // namespace XlsxWriter
