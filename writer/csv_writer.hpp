#pragma once
#include <string>
#include "loader/csv_loader.hpp"  // CsvData

namespace CsvWriter {
    // Writes CsvData to `path` using rapidcsv. Throws std::exception on failure.
    void save(const std::string& path, const CsvData& data);
}
