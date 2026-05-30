#pragma once
#include <string>
#include <utility>
#include <vector>
#include "loader/csv_loader.hpp"

namespace XlsxWriter {
    void save(const std::string& path, const SheetData& data);
    void save_workbook(const std::string& path,
                       const std::vector<std::pair<std::string, SheetData>>& sheets);
}
