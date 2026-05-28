#pragma once
#include <string>
#include "csv_loader.hpp"

namespace XlsxLoader {
    CsvData load(const std::string& path);
}
