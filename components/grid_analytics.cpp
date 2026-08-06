// grid_analytics.cpp — column sorting, the heatmap overlay, and the
// column-stats / formula suggestions that summarise data. (Part of the Grid
// class; see grid.hpp.)
#include "grid.hpp"

#include <algorithm>
#include <numeric>
#include <sstream>
#include <string>

#include "util/col_label.hpp"
#include "formulas/evaluator.hpp"
#include "util/numbers.hpp"
#include "util/strings.hpp"

// ── Sorting ──────────────────────────────────────────────────────────────────

// Order two evaluated cell values: numbers numerically (and before text),
// otherwise case-insensitive text. (Empties are handled by the caller.)
static int cmp_values(const Value& a, const Value& b) {
    double da, db;
    const bool an = a.to_number(da), bn = b.to_number(db);
    return tuix::cmp_mixed(an, da, a.to_display(), bn, db, b.to_display());
}

void Grid::sort_by(const std::vector<SortKey>& keys) {
    if (m_rows <= 1 || keys.empty()) return;
    for (const auto& k : keys)
        if (k.col < 0 || k.col >= m_cols) return;   // reject bad columns wholesale

    // Evaluate each key column once per row so the comparator never re-evaluates
    // formulas during the O(n log n) sort.
    std::vector<std::vector<Value>> kv(keys.size(), std::vector<Value>(m_rows));
    for (size_t k = 0; k < keys.size(); ++k)
        for (int r = 0; r < m_rows; ++r)
            kv[k][r] = cell_value(r, keys[k].col);

    std::vector<int> order(m_rows);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        for (size_t k = 0; k < keys.size(); ++k) {
            const Value& va = kv[k][a];
            const Value& vb = kv[k][b];
            const bool ea = va.is_empty(), eb = vb.is_empty();
            if (ea || eb) {
                if (ea && eb) continue;                 // tie on this key → next key
                return eb;                              // blanks sort last, both directions
            }
            const int c = cmp_values(va, vb);
            if (c != 0) return keys[k].descending ? c > 0 : c < 0;
        }
        return false;                                   // stable: equal keys keep order
    });

    m_sort_col  = keys[0].col;
    m_sort_desc = keys[0].descending;

    bool identity = true;
    for (int i = 0; i < m_rows; ++i)
        if (order[i] != i) { identity = false; break; }
    if (!identity) {
        HistoryEntry h;
        h.kind  = HistoryKind::Reorder;
        h.order = order;
        m_undo_stack.push_back(std::move(h));
        m_redo_stack.clear();
        apply_row_permutation(order);
        adjust_viewport();
        launch_build();
    }

    const std::string label = m_col_names[keys[0].col] + (keys[0].descending ? " ▼" : " ▲");
    set_status(has_formulas() ? "Sorted by " + label + "  (formula refs not adjusted)"
                              : "Sorted by " + label);
}

void Grid::sort_spec(const std::string& spec) {
    std::vector<SortKey> keys;
    auto add_key = [&](const std::string& token) {
        std::istringstream iss(token);
        std::string w1, w2;
        iss >> w1 >> w2;
        int  col  = m_cursor_col;     // default: the current column
        bool desc = false;
        if (!w1.empty()) {
            if (auto c = parse_col_label(w1)) {        // "B [dir]"
                col = *c;
                if (!w2.empty()) desc = tuix::is_desc_token(w2);
            } else {                                   // bare direction on current column
                desc = tuix::is_desc_token(w1);
            }
        }
        if (col >= 0 && col < m_cols) keys.push_back({col, desc});
    };

    if (spec.find_first_not_of(" \t") == std::string::npos) {
        if (m_cursor_col >= 0 && m_cursor_col < m_cols) keys.push_back({m_cursor_col, false});
    } else {
        for (size_t pos = 0; pos <= spec.size(); ) {
            const size_t comma = spec.find(',', pos);
            add_key(spec.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos));
            if (comma == std::string::npos) break;
            pos = comma + 1;
        }
    }

    if (keys.empty()) { set_status("sort: no valid column"); return; }
    sort_by(keys);
}

// ── Heatmap ──────────────────────────────────────────────────────────────────

std::optional<Grid::CellRect> Grid::active_range() const noexcept {
    if (m_has_selection && m_cursor_row >= 0 && m_cursor_col >= 0) return selection_bounds();
    if (m_cursor_col >= 0)                    // no selection → the whole current column
        return CellRect{0, m_cursor_col, m_rows - 1, m_cursor_col};
    return std::nullopt;                      // gutter — nothing to act on
}

void Grid::toggle_heatmap() {
    if (m_heat.active) { m_heat.active = false; set_status("Heatmap off"); return; }

    const auto range = active_range();
    if (!range) {
        set_status("Heatmap: put the cursor on a column first");
        return;
    }
    const auto [r0, c0, r1, c1] = *range;

    bool any = false;
    double mn = 0, mx = 0;
    for (int r = r0; r <= r1; ++r)
        for (int c = c0; c <= c1; ++c) {
            double v;
            if (cell_value(r, c).to_number(v)) {
                if (!any) { mn = mx = v; any = true; }
                else      { mn = std::min(mn, v); mx = std::max(mx, v); }
            }
        }
    if (!any) { set_status("Heatmap: no numeric values in range"); return; }

    m_heat.r0 = r0; m_heat.r1 = r1; m_heat.c0 = c0; m_heat.c1 = c1;
    m_heat.min = mn; m_heat.max = mx;
    m_heat.active = true;
    m_has_selection = false;                 // reveal the colors (selection would mask them)
    set_status("Heatmap on");
}

// ── Chart panel ────────────────────────────────────────────────────────────

void Grid::cycle_chart() {
    const char* name = "bar";
    if (!m_chart_active) {
        m_chart_active = true;
        m_chart_type   = ChartType::Bar;
    } else switch (m_chart_type) {
        case ChartType::Bar:       m_chart_type = ChartType::Line;      name = "line";      break;
        case ChartType::Line:      m_chart_type = ChartType::Histogram; name = "histogram"; break;
        case ChartType::Histogram: m_chart_active = false;             name = nullptr;     break;
    }
    set_status(name ? std::string("Chart: ") + name : "Chart off");
}

Grid::ChartData Grid::chart_data() const {
    ChartData cd;
    if (!m_chart_active) return cd;
    cd.active = true;
    cd.type   = m_chart_type;

    const auto range = active_range();
    if (!range) return cd;                   // gutter — nothing to chart
    const auto [r0, c0, r1, c1] = *range;

    cd.label = (c0 == c1 && c0 < static_cast<int>(m_col_names.size()))
                   ? m_col_names[c0] : "selection";

    // Re-gather (evaluating formulas) only when the data or range changed —
    // this runs on every render frame.
    if (m_chart.gen != m_data_gen || m_chart.r0 != r0 || m_chart.r1 != r1 ||
        m_chart.c0 != c0 || m_chart.c1 != c1) {
        m_chart.values.clear();
        m_chart.skipped = 0;
        for (int r = r0; r <= r1; ++r)
            for (int c = c0; c <= c1; ++c) {
                double v;
                if (cell_value(r, c).to_number(v)) m_chart.values.push_back(v);
                else                               ++m_chart.skipped;
            }
        m_chart.gen = m_data_gen;
        m_chart.r0 = r0; m_chart.r1 = r1; m_chart.c0 = c0; m_chart.c1 = c1;
    }
    cd.values  = m_chart.values;
    cd.skipped = m_chart.skipped;
    return cd;
}

// ── Column stats & formula suggestions ───────────────────────────────────────

Grid::ColumnStats Grid::column_stats() const {
    ColumnStats st;
    // Only summarize while the cursor sits on a column header (row -1).
    if (m_cursor_row >= 0) return st;
    if (m_cursor_col < 0 || m_cursor_col >= m_cols) return st;  // gutter — nothing to summarize
    // Cached per (data generation, column): this runs every render frame, and
    // the median sort + full-column evaluation are too heavy to redo per frame.
    if (m_stats.gen == m_data_gen && m_stats.col == m_cursor_col) return m_stats.value;
    const int c = m_cursor_col;
    st.valid = true;
    st.name  = m_col_names[c];
    st.total = m_rows;

    std::vector<double> nums;
    nums.reserve(m_rows);
    for (int r = 0; r < m_rows; ++r) {
        Value v = cell_value(r, c);
        if (v.is_empty()) { ++st.nulls; continue; }
        ++st.nonnull;
        double d;
        if (v.to_number(d)) {
            ++st.numeric;
            nums.push_back(d);
        }
    }

    if (!nums.empty()) {
        st.sum = std::accumulate(nums.begin(), nums.end(), 0.0);
        st.mean = st.sum / static_cast<double>(nums.size());
        st.min = *std::min_element(nums.begin(), nums.end());
        st.max = *std::max_element(nums.begin(), nums.end());
        std::sort(nums.begin(), nums.end());
        const size_t n = nums.size();
        st.median = (n % 2) ? nums[n / 2]
                            : (nums[n / 2 - 1] + nums[n / 2]) / 2.0;
    }
    m_stats.value = st;
    m_stats.gen   = m_data_gen;
    m_stats.col   = c;
    return st;
}

std::vector<Grid::Suggestion> Grid::cell_suggestions() const {
    if (m_cursor_row < 0 || m_cursor_col < 0) return {};
    if (m_has_selection) return {};
    if (!m_calc_cache.ready()) return {};
    auto entries = m_calc_cache.get(m_cursor_row, m_cursor_col);
    std::vector<Suggestion> out;
    out.reserve(entries.size());
    for (auto& e : entries) out.push_back({e.name, e.value});
    return out;
}

std::vector<Grid::Suggestion> Grid::range_suggestions() const {
    if (!m_has_selection || m_cursor_row < 0 || m_cursor_col < 0) return {};

    const auto [r0, c0, r1, c1] = selection_bounds();
    if (r0 == r1 && c0 == c1) return {};  // single cell — cell_suggestions handles it

    const std::string range = col_letter(c0) + std::to_string(r0 + 1)
                            + ":"
                            + col_letter(c1) + std::to_string(r1 + 1);

    std::vector<Suggestion> out;
    auto add = [&](const char* name, const std::string& formula) {
        Value v = Evaluator::evaluate_formula(formula, *this);
        if (!v.is_error()) out.push_back({name, v.to_display()});
    };

    add("SUM",    "=SUM("     + range + ")");
    add("AVG",    "=AVERAGE(" + range + ")");
    add("COUNT",  "=COUNT("   + range + ")");
    add("COUNTA", "=COUNTA("  + range + ")");
    add("MIN",    "=MIN("     + range + ")");
    add("MAX",    "=MAX("     + range + ")");

    return out;
}
