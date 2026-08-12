#pragma once
#include <ostream>
#include <string>
#include "loader/csv_loader.hpp"  // SheetData

namespace CsvWriter {
    // Writes SheetData to `path` using rapidcsv. Throws std::exception on failure.
    void save(const std::string& path, const SheetData& data);

    // Writes SheetData to an already-open stream (e.g. stdout).
    // `include_header` = false omits the header line (headless --no-header);
    // `delimiter` overrides data.delimiter when non-zero (headless --format tsv).
    void save_stream(std::ostream& out, const SheetData& data,
                     bool include_header = true, char delimiter = '\0');
}
