#pragma once
#include <string>
#include "loader/csv_loader.hpp"

namespace XlsxWriter {
    void save(const std::string& path, const SheetData& data);
}
