#include "csv_loader.hpp"
#include <rapidcsv.h>
#include <fstream>
#include <algorithm>

namespace {

char detect_delimiter(const std::string& path) {
    std::ifstream f(path);
    std::string line;
    if (!std::getline(f, line)) return ',';

    const char candidates[] = { ',', ';', '\t', '|' };
    char   best       = ',';
    size_t best_count = 0;
    for (char c : candidates) {
        size_t n = static_cast<size_t>(std::count(line.begin(), line.end(), c));
        if (n > best_count) { best_count = n; best = c; }
    }
    return best;
}

} // anonymous namespace

namespace CsvLoader {

CsvData load(const std::string& path) {
    char delim = detect_delimiter(path);
    rapidcsv::Document doc(path,
        rapidcsv::LabelParams(),
        rapidcsv::SeparatorParams(delim));

    CsvData result;
    result.delimiter = delim;
    result.headers   = doc.GetColumnNames();

    const size_t row_count = doc.GetRowCount();
    result.rows.reserve(row_count);
    for (size_t r = 0; r < row_count; ++r)
        result.rows.push_back(doc.GetRow<std::string>(r));

    return result;
}

} // namespace CsvLoader
