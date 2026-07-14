#pragma once
#include <string>
#include <utility>
#include <vector>
#include "csv_loader.hpp"

struct WorkbookData {
    std::vector<std::pair<std::string, SheetData>> sheets;
};

namespace XlsxLoader {
    WorkbookData load_workbook(const std::string& path);  // all sheets
}
