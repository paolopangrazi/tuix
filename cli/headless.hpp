#pragma once
#include <optional>
#include <string>
#include <vector>

#include "loader/csv_loader.hpp"  // SheetData
#include "loader/xlsx_loader.hpp"  // WorkbookData

// Headless "unix filter" mode: read a sheet from a file or stdin, apply a
// pipeline of row/column transforms, and write it back to a file or stdout —
// no TUI.
//
//   tuix --filter 'salary > 50000' --sort dept --select name,dept in.csv
//   cat in.csv | tuix --filter 'name =~ ^A' > out.csv
//
// The input and output formats are chosen by file extension: a `.xlsx` path is
// read/written as an Excel workbook (one sheet — see `--sheet`), anything else
// is CSV. stdin/stdout are always CSV (xlsx is binary and not pipe-friendly).
//
// The pipeline runs in a fixed order so later stages see earlier results:
//   filter → sort → head/tail → select.
namespace headless {

// A single row predicate, e.g. `salary > 50000` or `name =~ ^A`.
struct Predicate {
    enum class Op { EQ, NE, LT, LE, GT, GE, RE, CONTAINS };
    std::string col;   // column spec as written (header name or letter)
    Op          op = Op::EQ;
    std::string rhs;   // right-hand value / regex / substring
};

struct Options {
    std::string              input;        // path; empty or "-" means stdin
    std::string              output;       // path; empty or "-" means stdout
    std::string              sheet;        // xlsx sheet: name or 1-based index; empty = first
    std::string              sort_spec;    // comma-separated keys: "dept,B desc"
    std::vector<Predicate>   filters;      // AND-ed together
    std::string              select;       // comma-separated columns to keep/reorder
    int                      head = -1;    // keep first N data rows (-1 = all)
    int                      tail = -1;    // keep last N data rows (-1 = all)

    bool        active    = false;         // headless mode requested
    bool        show_help = false;         // --help was passed
    std::string parse_error;               // non-empty → bad flags, exit 2
};

// Parse argv into Options. Returns true when headless mode is in effect (any
// headless flag present, or stdin is piped); false → caller runs the TUI.
bool parse_args(int argc, char* argv[], Options& out);

// Apply the transform pipeline in place. Returns false with `err` set on a bad
// column reference or predicate. Exposed for unit testing.
bool apply(SheetData& data, const Options& opts, std::string& err);

// Resolve an xlsx sheet selector to a 0-based index into `wb.sheets`. `spec` is
// a sheet name (case-insensitive) or a 1-based index; empty selects the first
// sheet. Returns nullopt when the name/index does not resolve. Exposed for
// unit testing.
std::optional<size_t> resolve_sheet(const WorkbookData& wb, const std::string& spec);

// Full run: load input → apply → write output. Returns a process exit code.
int run(const Options& opts);

// Print headless usage to stdout.
void print_help();

}  // namespace headless
