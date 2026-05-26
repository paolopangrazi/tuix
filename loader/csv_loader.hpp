#pragma once
#include <string>
#include <vector>

struct CsvData {
    std::vector<std::string>              headers;
    std::vector<std::vector<std::string>> rows;
    char                                  delimiter = ',';
};

namespace CsvLoader {
    CsvData load(const std::string& path);
}
