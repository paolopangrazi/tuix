#pragma once
#include <istream>
#include <string>
#include <vector>

struct SheetData {
    std::vector<std::string>              headers;
    std::vector<std::vector<std::string>> rows;
    char                                  delimiter = ',';
};

namespace CsvLoader {
    // `delimiter` forces the separator (headless --delimiter); '\0' means
    // auto-detect it from the first line.
    SheetData load(const std::string& path, char delimiter = '\0');

    // Load CSV from an already-open stream (e.g. stdin). The delimiter is
    // handled exactly as load() does for a path.
    SheetData load_stream(std::istream& in, char delimiter = '\0');
}
