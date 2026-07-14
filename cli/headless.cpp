#include "headless.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <numeric>
#include <optional>
#include <regex>
#include <sstream>

#include <unistd.h>  // isatty, STDIN_FILENO

#include "col_label.hpp"
#include "loader/csv_loader.hpp"
#include "util/numbers.hpp"
#include "util/strings.hpp"
#include "writer/csv_writer.hpp"

namespace headless {

namespace {

// ── Small value helpers ──────────────────────────────────────────────────────

// Whole-string numeric parse, shared with the grid so headless filters and
// interactive sorting agree on what counts as a number.
bool parse_num(const std::string& s, double& out) {
    return tuix::parse_number(s, out);
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t");
    if (a == std::string::npos) return {};
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

// Strip one layer of matching surrounding quotes.
std::string unquote(std::string s) {
    if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'') && s.back() == s.front())
        return s.substr(1, s.size() - 2);
    return s;
}

// Resolve a column spec to a zero-based index: exact header name (case-
// insensitive) wins, else a spreadsheet column letter (A, B, AA...).
std::optional<int> resolve_column(const SheetData& d, const std::string& spec) {
    const std::string t = trim(spec);
    for (size_t c = 0; c < d.headers.size(); ++c)
        if (tuix::iequals(d.headers[c], t)) return static_cast<int>(c);
    if (auto c = parse_col_label(t)) {
        if (*c >= 0 && *c < static_cast<int>(d.headers.size())) return c;
    }
    return std::nullopt;
}

// Cell text at (row, col), tolerating short/ragged rows.
const std::string& cell_at(const SheetData& d, size_t row, size_t col) {
    static const std::string empty;
    if (row >= d.rows.size() || col >= d.rows[row].size()) return empty;
    return d.rows[row][col];
}

// ── Filtering ────────────────────────────────────────────────────────────────

std::optional<Predicate> parse_predicate(const std::string& raw, std::string& err) {
    const std::string s = trim(raw);

    // Keyword operator first (whitespace-delimited).
    if (auto pos = tuix::to_lower(s).find(" contains "); pos != std::string::npos) {
        return Predicate{trim(s.substr(0, pos)), Predicate::Op::CONTAINS,
                         unquote(trim(s.substr(pos + 10)))};
    }

    // Symbolic operators, longest-first at each position.
    struct OpTok { const char* txt; Predicate::Op op; };
    static const OpTok two[] = {
        {"==", Predicate::Op::EQ}, {"!=", Predicate::Op::NE},
        {"<=", Predicate::Op::LE}, {">=", Predicate::Op::GE},
        {"=~", Predicate::Op::RE},
    };
    static const OpTok one[] = {
        {"<", Predicate::Op::LT}, {">", Predicate::Op::GT}, {"=", Predicate::Op::EQ},
    };

    for (size_t i = 0; i < s.size(); ++i) {
        for (const auto& t : two)
            if (s.compare(i, 2, t.txt) == 0)
                return Predicate{trim(s.substr(0, i)), t.op, unquote(trim(s.substr(i + 2)))};
        for (const auto& t : one)
            if (s[i] == t.txt[0])
                return Predicate{trim(s.substr(0, i)), t.op, unquote(trim(s.substr(i + 1)))};
    }

    err = "filter: no operator in '" + raw + "'";
    return std::nullopt;
}

bool row_matches(const SheetData& d, size_t row, int col, const Predicate& p,
                 const std::regex* re) {
    const std::string& cell = cell_at(d, row, static_cast<size_t>(col));

    switch (p.op) {
        case Predicate::Op::RE:
            return re && std::regex_search(cell, *re);
        case Predicate::Op::CONTAINS:
            return tuix::to_lower(cell).find(tuix::to_lower(p.rhs)) != std::string::npos;
        default: break;
    }

    double cn, rn;
    const bool numeric = parse_num(cell, cn) && parse_num(p.rhs, rn);
    int cmp;  // <0, 0, >0
    if (numeric) {
        cmp = (cn < rn) ? -1 : (cn > rn ? 1 : 0);
    } else {
        const std::string a = tuix::to_lower(cell), b = tuix::to_lower(p.rhs);
        cmp = (a < b) ? -1 : (a > b ? 1 : 0);
    }

    switch (p.op) {
        case Predicate::Op::EQ: return cmp == 0;
        case Predicate::Op::NE: return cmp != 0;
        case Predicate::Op::LT: return cmp < 0;
        case Predicate::Op::LE: return cmp <= 0;
        case Predicate::Op::GT: return cmp > 0;
        case Predicate::Op::GE: return cmp >= 0;
        default:                return false;
    }
}

bool apply_filters(SheetData& d, const std::vector<Predicate>& preds, std::string& err) {
    struct Compiled { int col; const Predicate* p; std::optional<std::regex> re; };
    std::vector<Compiled> compiled;
    compiled.reserve(preds.size());

    for (const auto& p : preds) {
        auto col = resolve_column(d, p.col);
        if (!col) { err = "filter: unknown column '" + p.col + "'"; return false; }
        Compiled c{*col, &p, std::nullopt};
        if (p.op == Predicate::Op::RE) {
            try {
                c.re.emplace(p.rhs, std::regex::ECMAScript);
            } catch (const std::regex_error&) {
                err = "filter: bad regex '" + p.rhs + "'";
                return false;
            }
        }
        compiled.push_back(std::move(c));
    }

    std::vector<std::vector<std::string>> kept;
    kept.reserve(d.rows.size());
    for (size_t r = 0; r < d.rows.size(); ++r) {
        bool ok = true;
        for (const auto& c : compiled)
            if (!row_matches(d, r, c.col, *c.p, c.re ? &*c.re : nullptr)) { ok = false; break; }
        if (ok) kept.push_back(std::move(d.rows[r]));
    }
    d.rows = std::move(kept);
    return true;
}

// ── Sorting ──────────────────────────────────────────────────────────────────

struct SortKey { int col; bool desc; };

// Numeric-aware, blanks-last comparison mirroring the interactive grid's sort.
int cmp_cells(const std::string& a, const std::string& b) {
    double da, db;
    const bool an = parse_num(a, da), bn = parse_num(b, db);
    return tuix::cmp_mixed(an, da, a, bn, db, b);
}

bool apply_sort(SheetData& d, const std::string& spec, std::string& err) {
    std::vector<SortKey> keys;
    std::istringstream ss(spec);
    std::string token;
    while (std::getline(ss, token, ',')) {
        std::istringstream ts(token);
        std::string col_tok, dir_tok;
        ts >> col_tok >> dir_tok;
        if (col_tok.empty()) continue;
        auto col = resolve_column(d, col_tok);
        if (!col) { err = "sort: unknown column '" + col_tok + "'"; return false; }
        const std::string dl = tuix::to_lower(dir_tok);
        keys.push_back({*col, dl == "desc" || dl == "descending" || dl == "d"});
    }
    if (keys.empty()) { err = "sort: no valid column"; return false; }

    std::vector<size_t> order(d.rows.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        for (const auto& k : keys) {
            const std::string& va = cell_at(d, a, k.col);
            const std::string& vb = cell_at(d, b, k.col);
            const bool ea = trim(va).empty(), eb = trim(vb).empty();
            if (ea || eb) {
                if (ea && eb) continue;
                return eb;                       // blanks last, both directions
            }
            const int c = cmp_cells(va, vb);
            if (c != 0) return k.desc ? c > 0 : c < 0;
        }
        return false;
    });

    std::vector<std::vector<std::string>> sorted;
    sorted.reserve(d.rows.size());
    for (size_t i : order) sorted.push_back(std::move(d.rows[i]));
    d.rows = std::move(sorted);
    return true;
}

// ── Column selection ─────────────────────────────────────────────────────────

bool apply_select(SheetData& d, const std::string& spec, std::string& err) {
    std::vector<int> cols;
    std::istringstream ss(spec);
    std::string token;
    while (std::getline(ss, token, ',')) {
        const std::string t = trim(token);
        if (t.empty()) continue;
        auto col = resolve_column(d, t);
        if (!col) { err = "select: unknown column '" + t + "'"; return false; }
        cols.push_back(*col);
    }
    if (cols.empty()) { err = "select: no valid columns"; return false; }

    SheetData out;
    out.delimiter = d.delimiter;
    for (int c : cols)
        out.headers.push_back(c < static_cast<int>(d.headers.size()) ? d.headers[c] : "");
    out.rows.reserve(d.rows.size());
    for (size_t r = 0; r < d.rows.size(); ++r) {
        std::vector<std::string> row;
        row.reserve(cols.size());
        for (int c : cols) row.push_back(cell_at(d, r, static_cast<size_t>(c)));
        out.rows.push_back(std::move(row));
    }
    d = std::move(out);
    return true;
}

}  // anonymous namespace

// ── Pipeline ─────────────────────────────────────────────────────────────────

bool apply(SheetData& data, const Options& opts, std::string& err) {
    if (!opts.filters.empty() && !apply_filters(data, opts.filters, err)) return false;
    if (!opts.sort_spec.empty() && !apply_sort(data, opts.sort_spec, err)) return false;

    if (opts.head >= 0 && static_cast<size_t>(opts.head) < data.rows.size())
        data.rows.resize(static_cast<size_t>(opts.head));
    if (opts.tail >= 0 && static_cast<size_t>(opts.tail) < data.rows.size())
        data.rows.erase(data.rows.begin(),
                        data.rows.end() - static_cast<long>(opts.tail));

    if (!opts.select.empty() && !apply_select(data, opts.select, err)) return false;
    return true;
}

// ── Arg parsing ──────────────────────────────────────────────────────────────

bool parse_args(int argc, char* argv[], Options& out) {
    bool any_flag = false;
    auto need = [&](int& i) -> const char* {
        return (i + 1 < argc) ? argv[++i] : nullptr;
    };
    // Strict non-negative integer (atoi would turn "--head xyz" into 0 rows).
    auto parse_count = [](const char* v, int& n) {
        double d;
        if (!tuix::parse_number(v, d) || d < 0 || d != static_cast<int>(d)) return false;
        n = static_cast<int>(d);
        return true;
    };

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto flag = [&](const char* name) { return a == name; };

        if (flag("--help") || flag("-h")) {
            out.show_help = true; any_flag = true;
        } else if (flag("--filter") || flag("-f")) {
            const char* v = need(i);
            if (!v) { out.parse_error = a + " needs a predicate"; any_flag = true; break; }
            std::string perr;
            auto p = parse_predicate(v, perr);
            if (!p) { out.parse_error = perr; any_flag = true; break; }
            out.filters.push_back(*p); any_flag = true;
        } else if (flag("--sort") || flag("-s")) {
            const char* v = need(i);
            if (!v) { out.parse_error = a + " needs a column spec"; any_flag = true; break; }
            out.sort_spec += (out.sort_spec.empty() ? "" : ",") + std::string(v);
            any_flag = true;
        } else if (flag("--select")) {
            const char* v = need(i);
            if (!v) { out.parse_error = a + " needs columns"; any_flag = true; break; }
            out.select += (out.select.empty() ? "" : ",") + std::string(v);
            any_flag = true;
        } else if (flag("--head")) {
            const char* v = need(i);
            if (!v || !parse_count(v, out.head)) {
                out.parse_error = "--head needs a non-negative row count";
                any_flag = true; break;
            }
            any_flag = true;
        } else if (flag("--tail")) {
            const char* v = need(i);
            if (!v || !parse_count(v, out.tail)) {
                out.parse_error = "--tail needs a non-negative row count";
                any_flag = true; break;
            }
            any_flag = true;
        } else if (flag("--output") || flag("-o")) {
            const char* v = need(i);
            if (!v) { out.parse_error = a + " needs a path"; any_flag = true; break; }
            out.output = v; any_flag = true;
        } else if (a.size() > 1 && a[0] == '-' && a != "-") {
            out.parse_error = "unknown flag '" + a + "'"; any_flag = true; break;
        } else if (!out.input.empty()) {
            out.parse_error = "multiple input files ('" + out.input + "' and '" + a + "')";
            any_flag = true; break;
        } else {
            out.input = a;   // positional: input path (or "-" for stdin)
        }
    }

    // Headless if any headless flag was seen, or input is being piped in.
    const bool piped = !isatty(STDIN_FILENO);
    out.active = any_flag || (out.input == "-") || (out.input.empty() && piped);
    return out.active;
}

// ── Run ──────────────────────────────────────────────────────────────────────

int run(const Options& opts) {
    if (opts.show_help) { print_help(); return 0; }
    if (!opts.parse_error.empty()) {
        std::cerr << "tuix: " << opts.parse_error << "\n";
        return 2;
    }

    SheetData data;
    try {
        if (opts.input.empty() || opts.input == "-")
            data = CsvLoader::load_stream(std::cin);
        else
            data = CsvLoader::load(opts.input);
    } catch (const std::exception& e) {
        std::cerr << "tuix: cannot read input: " << e.what() << "\n";
        return 1;
    }

    std::string err;
    if (!apply(data, opts, err)) {
        std::cerr << "tuix: " << err << "\n";
        return 2;
    }

    try {
        if (opts.output.empty() || opts.output == "-")
            CsvWriter::save_stream(std::cout, data);
        else
            CsvWriter::save(opts.output, data);
    } catch (const std::exception& e) {
        std::cerr << "tuix: cannot write output: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

void print_help() {
    std::cout <<
        "tuix — terminal spreadsheet\n\n"
        "Interactive:  tuix [FILE]\n"
        "Headless:     tuix [OPTIONS] [FILE]     (also reads stdin when piped)\n\n"
        "Headless options (CSV in → CSV out):\n"
        "  -f, --filter PRED   keep rows matching PRED; repeatable (AND-ed)\n"
        "  -s, --sort SPEC     sort by columns, e.g. 'dept,salary desc'\n"
        "      --select COLS   keep/reorder columns, e.g. 'name,dept'\n"
        "      --head N        keep the first N data rows\n"
        "      --tail N        keep the last N data rows\n"
        "  -o, --output FILE   write here instead of stdout\n"
        "  -h, --help          show this help\n\n"
        "PRED is 'COLUMN OP VALUE'. COLUMN is a header name or letter (A, B...).\n"
        "OP is one of:  ==  !=  <  <=  >  >=  =~ (regex)  contains\n\n"
        "Examples:\n"
        "  tuix --filter 'salary > 50000' --sort dept employees.csv\n"
        "  cat employees.csv | tuix --filter 'name =~ ^A' --select name,salary\n";
}

}  // namespace headless
