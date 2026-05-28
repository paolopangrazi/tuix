#include "calc_cache.hpp"

#include "formulas/eval_context.hpp"
#include "formulas/evaluator.hpp"
#include "formulas/value.hpp"

namespace {

struct NullCtx : EvalContext {
    Value cell_value(int, int) const override { return Value::empty(); }
    int   rows()               const override { return 0; }
    int   cols()               const override { return 0; }
};

}  // namespace

CalcCache::SuggList CalcCache::compute_for(const std::string& raw) {
    SuggList out;
    if (raw.empty() || raw[0] == '=') return out;

    bool numeric = false;
    try {
        std::size_t pos = 0;
        std::stod(raw, &pos);
        numeric = (pos == raw.size());
    } catch (...) {}

    auto quote = [](const std::string& s) {
        std::string q = "\"";
        for (char c : s) { if (c == '"') q += "\"\""; else q += c; }
        return q + "\"";
    };

    NullCtx ctx;
    auto add = [&](const char* name, const std::string& formula) {
        Value v = Evaluator::evaluate_formula(formula, ctx);
        if (!v.is_error()) out.push_back({ name, v.to_display() });
    };

    if (numeric) {
        add("ABS",   "=ABS("   + raw + ")");
        add("INT",   "=INT("   + raw + ")");
        add("SQRT",  "=SQRT("  + raw + ")");
        add("ROUND", "=ROUND(" + raw + ", 2)");
        const std::string q = quote(raw);
        add("LEN",   "=LEN("   + q + ")");
        add("UPPER", "=UPPER(" + q + ")");
        add("TRIM",  "=TRIM("  + q + ")");
    } else {
        const std::string q = quote(raw);
        add("LEN",   "=LEN("   + q + ")");
        add("UPPER", "=UPPER(" + q + ")");
        add("LOWER", "=LOWER(" + q + ")");
        add("TRIM",  "=TRIM("  + q + ")");
    }
    return out;
}

void CalcCache::build(std::vector<std::vector<std::string>> raw) {
    const int rows = (int)raw.size();
    const int cols = rows > 0 ? (int)raw[0].size() : 0;

    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_data.assign(rows, std::vector<SuggList>(cols));
    }

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (m_cancel.load(std::memory_order_acquire)) return;
            SuggList sl = compute_for(raw[r][c]);
            std::lock_guard<std::mutex> lk(m_mutex);
            m_data[r][c] = std::move(sl);
        }
    }
    m_ready.store(true, std::memory_order_release);
}

void CalcCache::rebuild_cell(int r, int c, const std::string& raw_value) {
    SuggList sl = compute_for(raw_value);
    std::lock_guard<std::mutex> lk(m_mutex);
    if (r >= 0 && r < (int)m_data.size() &&
        c >= 0 && c < (int)m_data[r].size())
        m_data[r][c] = std::move(sl);
}

bool CalcCache::ready() const noexcept {
    return m_ready.load(std::memory_order_acquire);
}

CalcCache::SuggList CalcCache::get(int r, int c) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (r < 0 || r >= (int)m_data.size()) return {};
    if (c < 0 || c >= (int)m_data[r].size()) return {};
    return m_data[r][c];
}

void CalcCache::reset() {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_data.clear();
    m_ready.store(false, std::memory_order_release);
    m_cancel.store(false, std::memory_order_release);
}

void CalcCache::cancel() {
    m_cancel.store(true, std::memory_order_release);
}
