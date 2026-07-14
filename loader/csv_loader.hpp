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
    SheetData load(const std::string& path);

    // Load CSV from an already-open stream (e.g. stdin). The delimiter is
    // auto-detected from the first line, exactly as load() does for a path.
    SheetData load_stream(std::istream& in);
}
