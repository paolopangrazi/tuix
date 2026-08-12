#include "csv_writer.hpp"
#include <rapidcsv.h>

namespace CsvWriter {

namespace {

rapidcsv::Document build(const SheetData& data, bool include_header = true,
                         char delimiter = '\0') {
    rapidcsv::Document doc(
        std::string(),
        // row 0 = column names, or -1 for "no header row at all"
        include_header ? rapidcsv::LabelParams() : rapidcsv::LabelParams(-1, -1),
        rapidcsv::SeparatorParams(delimiter ? delimiter : data.delimiter)
    );

    if (include_header)
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

void save_stream(std::ostream& out, const SheetData& data, bool include_header,
                 char delimiter) {
    build(data, include_header, delimiter).Save(out);
}

} // namespace CsvWriter
