#pragma once
#include <string>
#include <vector>

struct SheetData {
    std::vector<std::string>              headers;
    std::vector<std::vector<std::string>> rows;
    char                                  delimiter = ',';
};

namespace CsvLoader {
    SheetData load(const std::string& path);
}
