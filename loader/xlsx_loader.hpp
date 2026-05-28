#pragma once
#include <string>
#include "csv_loader.hpp"

namespace XlsxLoader {
    SheetData load(const std::string& path);
}
