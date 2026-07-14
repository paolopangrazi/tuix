#pragma once
#include <ostream>
#include <string>
#include "loader/csv_loader.hpp"  // SheetData

namespace CsvWriter {
    // Writes SheetData to `path` using rapidcsv. Throws std::exception on failure.
    void save(const std::string& path, const SheetData& data);

    // Writes SheetData to an already-open stream (e.g. stdout).
    void save_stream(std::ostream& out, const SheetData& data);
}
