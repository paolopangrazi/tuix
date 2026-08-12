#include "csv_loader.hpp"
#include <rapidcsv.h>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace {

// Pick the delimiter that appears most on the header line.
char detect_delimiter(const std::string& first_line) {
    const char candidates[] = { ',', ';', '\t', '|' };
    char   best       = ',';
    size_t best_count = 0;
    for (char c : candidates) {
        size_t n = static_cast<size_t>(std::count(first_line.begin(), first_line.end(), c));
        if (n > best_count) { best_count = n; best = c; }
    }
    return best;
}

char detect_delimiter_of_path(const std::string& path) {
    std::ifstream f(path);
    std::string line;
    if (!std::getline(f, line)) return ',';
    return detect_delimiter(line);
}

SheetData harvest(rapidcsv::Document& doc, char delim) {
    SheetData result;
    result.delimiter = delim;
    result.headers   = doc.GetColumnNames();

    const size_t row_count = doc.GetRowCount();
    result.rows.reserve(row_count);
    for (size_t r = 0; r < row_count; ++r)
        result.rows.push_back(doc.GetRow<std::string>(r));

    return result;
}

} // anonymous namespace

namespace CsvLoader {

SheetData load(const std::string& path, char delimiter) {
    char delim = delimiter ? delimiter : detect_delimiter_of_path(path);
    rapidcsv::Document doc(path,
        rapidcsv::LabelParams(),
        rapidcsv::SeparatorParams(delim));
    return harvest(doc, delim);
}

SheetData load_stream(std::istream& in, char delimiter) {
    // rapidcsv consumes the whole stream, so slurp it once and detect the
    // delimiter from the first line before re-parsing.
    std::stringstream buf;
    buf << in.rdbuf();
    std::string text = buf.str();

    const size_t nl    = text.find('\n');
    char         delim = delimiter ? delimiter : detect_delimiter(text.substr(0, nl));

    std::istringstream parse(text);
    rapidcsv::Document doc(parse,
        rapidcsv::LabelParams(),
        rapidcsv::SeparatorParams(delim));
    return harvest(doc, delim);
}

} // namespace CsvLoader
