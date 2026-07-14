#include "csv_writer.hpp"
#include <rapidcsv.h>

namespace CsvWriter {

namespace {

rapidcsv::Document build(const SheetData& data) {
    rapidcsv::Document doc(
        std::string(),
        rapidcsv::LabelParams(),  // row 0 = column names
        rapidcsv::SeparatorParams(data.delimiter)
    );

    for (size_t c = 0; c < data.headers.size(); ++c)
        doc.SetColumnName(c, data.headers[c]);

    for (size_t r = 0; r < data.rows.size(); ++r)
        for (size_t c = 0; c < data.rows[r].size(); ++c)
            doc.SetCell<std::string>(c, r, data.rows[r][c]);

    return doc;
}

} // anonymous namespace

void save(const std::string& path, const SheetData& data) {
    build(data).Save(path);
}

void save_stream(std::ostream& out, const SheetData& data) {
    build(data).Save(out);
}

} // namespace CsvWriter
