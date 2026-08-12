#include "sheet_format.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <ostream>
#include <vector>

#include "util/col_label.hpp"
#include "util/numbers.hpp"
#include "util/strings.hpp"
#include "util/text_width.hpp"
#include "writer/csv_writer.hpp"

namespace headless {

namespace {

// Column count: a ragged row can be wider than the header line.
size_t width_of(const SheetData& d) {
    size_t w = d.headers.size();
    for (const auto& r : d.rows) w = std::max(w, r.size());
    return w;
}

const std::string& at(const std::vector<std::string>& row, size_t c) {
    static const std::string empty;
    return c < row.size() ? row[c] : empty;
}

// Column name for the formats that need one, falling back to the spreadsheet
// letter when the header is missing or blank (as it is under --no-header).
std::string key_of(const SheetData& d, size_t c) {
    if (c < d.headers.size() && !d.headers[c].empty()) return d.headers[c];
    return col_letter(static_cast<int>(c));
}

// ── JSON / JSONL ─────────────────────────────────────────────────────────────

// True when the cell is exactly a JSON number literal, so it can be emitted
// unquoted and `jq` sees a number. The JSON grammar is stricter than tuix's
// parse_number — no leading +, no leading zeros, no bare ".5" — and the extra
// parse_number check rejects overflow like 1e999. Everything else stays a
// string, which keeps "007" and "1,200" intact.
bool is_json_number(const std::string& s) {
    auto digit = [&](size_t i) { return i < s.size() && std::isdigit((unsigned char)s[i]); };

    size_t i = 0;
    if (i < s.size() && s[i] == '-') ++i;
    if (!digit(i)) return false;
    if (s[i] == '0') ++i;
    else while (digit(i)) ++i;

    if (i < s.size() && s[i] == '.') {
        ++i;
        if (!digit(i)) return false;
        while (digit(i)) ++i;
    }
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
        ++i;
        if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;
        if (!digit(i)) return false;
        while (digit(i)) ++i;
    }
    if (i != s.size()) return false;

    double n;
    return tuix::parse_number(s, n);
}

void write_json_string(std::ostream& o, const std::string& s) {
    static const char* hex = "0123456789abcdef";
    o << '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"':  o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\n': o << "\\n";  break;
            case '\r': o << "\\r";  break;
            case '\t': o << "\\t";  break;
            default:
                if (c < 0x20) o << "\\u00" << hex[c >> 4] << hex[c & 0x0F];
                else          o << static_cast<char>(c);  // UTF-8 passes through
        }
    }
    o << '"';
}

// `lines` = JSONL: one object per line, no wrapping array or commas.
void write_json(std::ostream& o, const SheetData& d, bool lines) {
    const size_t w = width_of(d);
    if (!lines) o << "[\n";

    for (size_t r = 0; r < d.rows.size(); ++r) {
        if (!lines) o << "  ";
        o << '{';
        for (size_t c = 0; c < w; ++c) {
            if (c) o << ", ";
            write_json_string(o, key_of(d, c));
            o << ": ";
            const std::string& cell = at(d.rows[r], c);
            if (is_json_number(cell)) o << cell;
            else                      write_json_string(o, cell);
        }
        o << '}';
        if (!lines && r + 1 < d.rows.size()) o << ',';
        o << '\n';
    }

    if (!lines) o << "]\n";
}

// ── Markdown ─────────────────────────────────────────────────────────────────

// A cell has to survive inside `| ... |`: pipes escape, newlines flatten.
std::string md_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '|')                    out += "\\|";
        else if (c == '\n' || c == '\r') out += ' ';
        else                             out += c;
    }
    return out;
}

void write_md(std::ostream& o, const SheetData& d, bool include_header) {
    const size_t w = width_of(d);
    if (w == 0) return;

    // A markdown table is only a table with a header row and its rule, so an
    // omitted header becomes blank cells rather than disappearing.
    o << '|';
    for (size_t c = 0; c < w; ++c)
        o << ' ' << (include_header ? md_escape(key_of(d, c)) : std::string()) << " |";
    o << "\n|";
    for (size_t c = 0; c < w; ++c) o << " --- |";
    o << '\n';

    for (const auto& row : d.rows) {
        o << '|';
        for (size_t c = 0; c < w; ++c) o << ' ' << md_escape(at(row, c)) << " |";
        o << '\n';
    }
}

// ── Aligned plain text ───────────────────────────────────────────────────────

void write_table(std::ostream& o, const SheetData& d, bool include_header) {
    const size_t w = width_of(d);
    if (w == 0) return;

    // Widths in terminal columns (not bytes), and which columns are numeric —
    // those right-align so digits line up.
    std::vector<int>  width(w, 0);
    std::vector<char> numeric(w, 1);
    for (size_t c = 0; c < w; ++c) {
        bool any = false;
        if (include_header) width[c] = tuix::display_width(key_of(d, c));
        for (const auto& row : d.rows) {
            const std::string& cell = at(row, c);
            width[c] = std::max(width[c], tuix::display_width(cell));
            if (cell.empty()) continue;
            any = true;
            double n;
            if (!tuix::parse_number(cell, n)) numeric[c] = 0;
        }
        if (!any) numeric[c] = 0;  // an all-blank column is not a number column
    }

    auto emit = [&](const std::vector<std::string>& cells) {
        std::string line;
        for (size_t c = 0; c < w; ++c) {
            const std::string& cell = at(cells, c);
            const int pad = std::max(0, width[c] - tuix::display_width(cell));
            if (c) line += "  ";
            if (numeric[c]) line.append(static_cast<size_t>(pad), ' ');
            line += cell;
            if (!numeric[c]) line.append(static_cast<size_t>(pad), ' ');
        }
        while (!line.empty() && line.back() == ' ') line.pop_back();
        o << line << '\n';
    };

    if (include_header) {
        std::vector<std::string> header, rule;
        for (size_t c = 0; c < w; ++c) {
            header.push_back(key_of(d, c));
            rule.push_back(std::string(static_cast<size_t>(width[c]), '-'));
        }
        emit(header);
        emit(rule);
    }
    for (const auto& row : d.rows) emit(row);
}

}  // anonymous namespace

// ── Format selection ─────────────────────────────────────────────────────────

std::optional<Format> parse_format(const std::string& name) {
    const std::string n = tuix::to_lower(name);
    if (n == "csv")                      return Format::CSV;
    if (n == "tsv")                      return Format::TSV;
    if (n == "json")                     return Format::JSON;
    if (n == "jsonl" || n == "ndjson")   return Format::JSONL;
    if (n == "md"    || n == "markdown") return Format::MD;
    if (n == "table")                    return Format::TABLE;
    return std::nullopt;
}

std::optional<Format> format_from_extension(const std::string& path) {
    if (path.empty() || path == "-") return std::nullopt;
    const std::string ext =
        tuix::to_lower(std::filesystem::path(path).extension().string());
    if (ext == ".csv")                     return Format::CSV;
    if (ext == ".tsv" || ext == ".tab")    return Format::TSV;
    if (ext == ".json")                    return Format::JSON;
    if (ext == ".jsonl" || ext == ".ndjson") return Format::JSONL;
    if (ext == ".md" || ext == ".markdown") return Format::MD;
    return std::nullopt;
}

void write_sheet(std::ostream& out, const SheetData& data, Format fmt,
                 bool include_header) {
    switch (fmt) {
        case Format::CSV:   CsvWriter::save_stream(out, data, include_header); break;
        case Format::TSV:   CsvWriter::save_stream(out, data, include_header, '\t'); break;
        case Format::JSON:  write_json(out, data, /*lines=*/false); break;
        case Format::JSONL: write_json(out, data, /*lines=*/true);  break;
        case Format::MD:    write_md(out, data, include_header);    break;
        case Format::TABLE: write_table(out, data, include_header); break;
    }
}

}  // namespace headless
