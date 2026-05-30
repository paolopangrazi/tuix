#include "xlsx_loader.hpp"

#include <OpenXLSX.hpp>
#include <sstream>
#include <iomanip>

namespace {

std::string cell_to_string(const OpenXLSX::XLCellValue& val) {
    using OpenXLSX::XLValueType;
    switch (val.type()) {
        case XLValueType::Empty:
            return "";
        case XLValueType::Boolean:
            return val.get<bool>() ? "TRUE" : "FALSE";
        case XLValueType::Integer:
            return std::to_string(val.get<int64_t>());
        case XLValueType::Float: {
            std::ostringstream oss;
            oss << std::setprecision(15) << std::noshowpoint << val.get<double>();
            return oss.str();
        }
        case XLValueType::String:
            return val.get<std::string>();
        case XLValueType::Error:
            return "#ERR";
    }
    return "";
}

SheetData read_sheet(OpenXLSX::XLWorksheet wks) {
    uint32_t nrows = wks.rowCount();
    uint16_t ncols = wks.columnCount();

    SheetData data;
    data.delimiter = ',';

    if (nrows == 0) return data;

    auto hdr_row = wks.rows(1, 1);
    for (auto& row : hdr_row) {
        auto cells = row.cells(ncols);
        for (auto& cell : cells)
            data.headers.push_back(cell_to_string(cell.value()));
    }

    if (nrows > 1) {
        for (auto& row : wks.rows(2, nrows)) {
            std::vector<std::string> r;
            r.reserve(ncols);
            auto cells = row.cells(ncols);
            for (auto& cell : cells)
                r.push_back(cell_to_string(cell.value()));
            data.rows.push_back(std::move(r));
        }
    }
    return data;
}

} // namespace

SheetData XlsxLoader::load(const std::string& path) {
    OpenXLSX::XLDocument doc;
    doc.open(path);
    SheetData data = read_sheet(doc.workbook().worksheet(1));
    doc.close();
    return data;
}

WorkbookData XlsxLoader::load_workbook(const std::string& path) {
    OpenXLSX::XLDocument doc;
    doc.open(path);

    WorkbookData wb;
    for (const auto& name : doc.workbook().worksheetNames())
        wb.sheets.emplace_back(name, read_sheet(doc.workbook().worksheet(name)));

    doc.close();
    return wb;
}
