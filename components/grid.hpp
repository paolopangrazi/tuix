#pragma once
#include <functional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "actionbox.hpp"
#include "calc_cache.hpp"
#include "cell.hpp"
#include "loader/csv_loader.hpp"
#include "formulas/eval_context.hpp"
#include "formulas/value.hpp"
#include "config/config.hpp"

struct Sheet;

class Grid : public EvalContext {
public:
    // A history step is either a single-cell edit or a whole-row reorder (sort).
    // For Reorder, `order` is the permutation that was applied: new[i] = old[order[i]];
    // undo applies its inverse, and row/col/before/after are unused. The new
    // fields are last (with defaults) so existing `{row,col,before,after}` init
    // keeps working for cell edits.
    enum class HistoryKind { Cell, Reorder };
    struct HistoryEntry {
        int row = 0, col = 0;
        std::string before, after;
        HistoryKind kind = HistoryKind::Cell;
        std::vector<int> order;
    };

    // One sort criterion: column index + ascending/descending.
    struct SortKey { int col; bool descending; };

    // ── Lifecycle, data & persistence (grid.cpp) ─────────────────────────────
    Grid(int rows, int cols, const Config& cfg = {});
    ~Grid();

    int rows() const override;
    int cols() const override;

    Cell&       at(int r, int c);
    const Cell& at(int r, int c) const;

    void    load(const SheetData& data);
    SheetData to_csv_data(char delimiter = ',') const;

    // Swap the grid's full editable state in/out of a Sheet snapshot.
    void save_to  (Sheet& s) const;
    void load_from(const Sheet& s);

    // ── Undo / redo (grid_edit.cpp) ──────────────────────────────────────────
    void undo();
    void redo();
    bool can_undo() const noexcept;
    bool can_redo() const noexcept;

    // ── Search, replace & sort (grid_search.cpp, grid_analytics.cpp) ─────────
    // Move the cursor to an A1 cell address (e.g. "B12"); false if out of range.
    bool goto_ref(const std::string& a1);

    // Replace every occurrence of `find` with `repl` across all data cells
    // (raw values, case-sensitive). Returns the number of replacements made and
    // posts a transient status message. A no-op when `find` is empty.
    int replace_all(const std::string& find, const std::string& repl);

    // Stable-sort the data rows by one or more columns (typed: numeric when both
    // cells are numbers, else case-insensitive text; blanks last). Pushes one
    // undoable reorder step. `sort_spec` parses a command string like
    // "B desc, A" (empty / bare → the current cursor column, ascending).
    void sort_by(const std::vector<SortKey>& keys);
    void sort_spec(const std::string& spec);

    // Toggle a cold→hot heatmap over the current selection (or, with no
    // selection, the cursor's whole column). Off → captures the range's numeric
    // min/max and shades it; on → clears.
    void toggle_heatmap();

    // True while the `/` search prompt is capturing keys.
    bool searching() const noexcept { return m_searching; }

    // ── Mode & status (grid.cpp, grid_render.cpp) ────────────────────────────
    enum class Mode { NORMAL, INSERT };
    Mode        mode()         const noexcept;
    std::string context_hint() const;

    // Post a transient one-shot message (e.g. an I/O error). Cleared on next move.
    void set_status(std::string msg) { m_status_msg = std::move(msg); }

    // ── Column stats & suggestions (grid_analytics.cpp) ──────────────────────
    // Live summary of the column under the cursor (for the column-stats strip).
    // `valid` is false when the cursor is not on a data column (header/gutter).
    struct ColumnStats {
        bool        valid   = false;
        std::string name;
        int         total   = 0;   // data rows in the column
        int         nonnull = 0;   // non-empty cells
        int         nulls   = 0;   // empty cells
        int         numeric = 0;   // cells that coerce to a number
        double      sum     = 0.0;
        double      mean    = 0.0;
        double      median  = 0.0;
        double      min     = 0.0;
        double      max     = 0.0;
    };
    ColumnStats column_stats() const;

    // Pre-computed formula values applicable to the current cell (empty = hide).
    struct Suggestion { std::string name, value; };
    std::vector<Suggestion> cell_suggestions()  const;
    std::vector<Suggestion> range_suggestions() const;  // live, for multi-cell selections

    // ── EvalContext: formula evaluation (grid.cpp) ───────────────────────────
    Value cell_value(int row, int col) const override;
    Value cell_value_in(const std::string& sheet, int row, int col) const override;

    // Install a resolver mapping a sheet name to its snapshot (for cross-sheet
    // references like =Sheet2!A1). Returns nullptr for an unknown sheet.
    void set_sheet_lookup(std::function<const Sheet*(const std::string&)> fn);
    void set_sheet_name(std::string name) { m_sheet_name = std::move(name); }

    void set_calc_ready_cb(std::function<void()> cb);

    // ── FTXUI component & event dispatch (grid_input.cpp) ────────────────────
    ftxui::Component make_component();

private:
    // ── Internal state ───────────────────────────────────────────────────────
    int k_cell_w;
    static constexpr int k_rownum_w = 5;

    int m_rows, m_cols;
    Config m_cfg;
    int m_cursor_row = 0, m_cursor_col = 0;
    int m_offset_row = 0, m_offset_col = 0;
    int  m_sel_row = 0, m_sel_col = 0;
    bool m_has_selection = false;

    std::string                    m_sheet_name;   // active sheet's name (for self-qualified refs)
    std::function<const Sheet*(const std::string&)> m_sheet_lookup;  // cross-sheet resolver

    std::vector<std::vector<Cell>> m_cells;
    std::vector<std::string>       m_col_names;
    std::vector<int>               m_col_widths;
    // Columns the user has resized by hand: their width is pinned and no longer
    // tracks content (auto-fit via compute_col_width is skipped — see refit_col).
    std::vector<bool>              m_col_manual;
    // Per-row height in terminal lines (default 1). Only changed by the user;
    // there is no auto-fit for height, so no "manual" flag is needed.
    std::vector<int>               m_row_heights;
    mutable ftxui::Box             m_box              = {};
    std::vector<ActionBox>         m_action_boxes;
    std::vector<ActionBox>         m_col_action_boxes;
    int                            m_pending_delete_row = -1;
    int                            m_pending_delete_col = -1;

    // Mouse column-resize drag. m_resize_col is the column being dragged (-1 when
    // idle); m_resize_hover is the boundary under the cursor, used only to
    // highlight the grabbable separator. m_resize_start_* anchor the press.
    int                            m_resize_col     = -1;
    int                            m_resize_hover   = -1;
    int                            m_resize_start_x = 0;
    int                            m_resize_start_w = 0;
    // Mouse row-resize drag, mirroring the column fields above: m_resize_row is
    // the row being dragged, m_resize_hrow the bottom border under the cursor.
    int                            m_resize_row     = -1;
    int                            m_resize_hrow    = -1;
    int                            m_resize_start_y = 0;
    int                            m_resize_start_h = 0;
    // True while a left-drag is painting a multi-cell selection; the anchor is
    // m_sel_row/m_sel_col and the moving end is the cursor.
    bool                           m_mouse_selecting = false;

    // Last sort applied, for the ▲/▼ header indicator (-1 = none).
    int                            m_sort_col  = -1;
    bool                           m_sort_desc = false;
    // Header column under the mouse (-1 = none); shows a clickable sort hint.
    int                            m_header_hover = -1;

    // Heatmap: when active, numeric cells in the rectangle [r0..r1]×[c0..c1] are
    // shaded along a cold→hot gradient scaled to the captured min/max.
    bool                           m_heat_active = false;
    int                            m_heat_r0 = 0, m_heat_c0 = 0, m_heat_r1 = 0, m_heat_c1 = 0;
    double                         m_heat_min = 0.0, m_heat_max = 0.0;

    std::vector<HistoryEntry> m_undo_stack;
    std::vector<HistoryEntry> m_redo_stack;
    void apply_history(std::vector<HistoryEntry>& from,
                       std::vector<HistoryEntry>& to, bool use_after);
    // Reorder the data rows (cells + heights + action boxes): new[i] = old[order[i]].
    void apply_row_permutation(const std::vector<int>& order);

    std::vector<std::vector<std::string>> m_yank_data;  // [rows][cols] raw values
    int m_yank_row = -1;
    int m_yank_col = -1;
    void yank_selection();
    void paste_yanked();

    CalcCache              m_calc_cache;
    std::thread            m_bg_thread;
    std::function<void()>  m_on_calc_ready;

    void launch_build();

    bool        m_pending_g      = false;
    bool        m_editing        = false;
    bool        m_insert_sticky  = false;
    std::string m_edit_buf;
    std::string m_edit_orig;
    int         m_edit_cursor    = 0;
    bool        m_edit_typed     = false;  // true once user has pressed a character key

    // `/` incremental search. m_search_query is the active term that drives
    // n/N stepping and match highlighting; m_search_hits is its row-major matches.
    bool        m_searching      = false;
    std::string m_status_msg;            // transient one-shot message (e.g. replace count)
    std::string m_search_buf;            // live text while the prompt is open
    std::string m_search_query;          // committed/active term (empty = no highlight)
    int         m_search_origin_row = 0; // cursor before search, restored on cancel
    int         m_search_origin_col = 0;
    std::vector<std::pair<int, int>> m_search_hits;

    // ── Internal operations (defined across the grid_*.cpp files) ────────────
    void start_search();
    void update_search();        // recompute hits from m_search_buf, preview-jump
    void commit_search();
    void cancel_search();
    void search_step(int dir);   // n (+1) / N (-1)
    void recompute_hits(const std::string& q);

    void move(int dr, int dc);
    void move_home();
    void move_end();
    void page_up();
    void page_down();
    void add_row();
    void insert_row(int r);
    void add_col();
    void insert_col(int c);
    void delete_row(int r);
    void try_delete_row(int r);
    void delete_col(int c);
    void try_delete_col(int c);
    // Shared tail for every row/column insert or delete: leave edit mode, fix
    // the viewport, and kick off a background recalc.
    void after_structural_change();
    bool select_at_mouse(int mx, int my);
    // Map a mouse position to a data column / row, or -1 if outside the cells.
    // (content-relative rx = mx - x_min - 1, ry = my - y_min - 1.)
    int  col_at_x(int rx) const;
    int  row_at_y(int ry) const;
    // Extend the active mouse drag-selection to the cell under the cursor,
    // clamping a drag that wanders into the gutter/header/outside.
    void drag_select_to(int mx, int my);
    void scroll_to_mouse_y(int my);
    void start_edit(bool clear);
    void commit_edit();
    void extend_selection(int dr, int dc);
    void commit_and_step(int dr, int dc);

    // Cursor target is the header row when m_cursor_row < 0, else a grid cell.
    std::string value_at(int r, int c) const;
    void        set_value_at(int r, int c, const std::string& v);
    // Clear a single cell's value, recording one undo entry (no-op if empty).
    void        clear_cell(int r, int c);

    // Event dispatch (called from CatchEvent in make_component)
    bool handle_pending_delete (ftxui::Event e);
    bool handle_cell_editing   (ftxui::Event e);
    bool handle_search         (ftxui::Event e);
    bool handle_normal_nav     (ftxui::Event e);
    bool handle_mouse          (ftxui::Event e);

    bool           in_selection(int r, int c) const noexcept;
    std::string    cursor_label()  const;
    std::string    cell_display(int r, int c) const;
    ftxui::Element formula_bar()   const;
    ftxui::Element render()        const;
    ftxui::Element vscrollbar()    const;

    int  vis_rows() const;
    int  vis_cols() const;
    void adjust_viewport();
    int         compute_col_width(int c) const;
    // Re-auto-fit column c to its content, unless the user has pinned its width.
    void        refit_col(int c);
    // Widen (delta > 0) or narrow (delta < 0) column c by hand, pinning its width.
    void        resize_col(int c, int delta);
    // Set column c's width directly (clamped), pinning it. Shared by keyboard
    // resize and mouse drag.
    void        set_col_width(int c, int w);
    // Column whose right-edge separator sits at/near content-x rx (else -1),
    // for mouse resize hit-testing. rx is mouse-x relative to the grid content.
    int         border_hit(int rx) const;
    // Row-height analogs of the column helpers above.
    void        resize_row(int r, int delta);
    void        set_row_height(int r, int h);
    // Row whose bottom-edge separator sits on content-line ry (else -1).
    int         row_border_hit(int ry) const;
    // How many rows, starting at offset `off`, fit in the current viewport.
    int         rows_fitting_from(int off) const;
    std::string unique_col_name(const std::string& name, int skip_col) const;

    // formula autocomplete
    int              m_ac_sel = 0;
    std::vector<int> ac_matches() const;
    ftxui::Element   render_ac_popup() const;
};
