#pragma once
#include <iosfwd>
#include <optional>
#include <string>

#include "loader/csv_loader.hpp"  // SheetData

// Output encodings for headless mode. The transform pipeline is CSV-shaped
// throughout (SheetData in, SheetData out); these decide only how the finished
// sheet is serialized, so tuix can feed `jq`, a markdown doc, or a terminal.
namespace headless {

enum class Format { CSV, TSV, JSON, JSONL, MD, TABLE };

// Parse a --format value: csv, tsv, json, jsonl (ndjson), md (markdown),
// table. Case-insensitive; nullopt for an unknown name.
std::optional<Format> parse_format(const std::string& name);

// Infer the format from an output path's extension. nullopt when the extension
// says nothing, leaving the fallback to the caller.
std::optional<Format> format_from_extension(const std::string& path);

// Serialize `data` to `out`. `include_header` = false drops the header row for
// the row-oriented formats (csv/tsv/table); JSON objects always need keys and a
// markdown table always needs its header rule, so those emit column letters and
// a blank header row respectively.
void write_sheet(std::ostream& out, const SheetData& data, Format fmt,
                 bool include_header = true);

}  // namespace headless
