#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

class CalcCache {
public:
    struct Entry { std::string name, value; };
    using SuggList = std::vector<Entry>;

    // Called from background thread with a snapshot of raw cell values.
    void build(std::vector<std::vector<std::string>> raw_snapshot);

    // Called synchronously on the main thread after a single-cell commit/undo/redo.
    void rebuild_cell(int r, int c, const std::string& raw_value);

    bool     ready()           const noexcept;
    SuggList get(int r, int c) const;
    void     reset();
    void     cancel();   // signals a running build to exit early

private:
    static SuggList compute_for(const std::string& raw);

    std::vector<std::vector<SuggList>> m_data;
    std::atomic<bool>                  m_ready{false};
    std::atomic<bool>                  m_cancel{false};
    mutable std::mutex                 m_mutex;
};
