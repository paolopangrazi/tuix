#pragma once
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "actionbox.hpp"
#include "cell.hpp"
#include "loader/csv_loader.hpp"
#include "formulas/eval_context.hpp"
#include "formulas/value.hpp"
#include "config/config.hpp"

class Grid : public EvalContext {
public:
    Grid(int rows, int cols, const Config& cfg = {});

    int rows() const override;
    int cols() const override;

    Cell&       at(int r, int c);
    const Cell& at(int r, int c) const;

    void    load(const CsvData& data);
    CsvData to_csv_data(char delimiter = ',') const;

    void undo();
    void redo();
    bool can_undo() const noexcept;
    bool can_redo() const noexcept;

    enum class Mode { NORMAL, INSERT };
    Mode        mode()         const noexcept;
    std::string context_hint() const;

    Value cell_value(int row, int col) const override;

    ftxui::Component make_component();

private:
    int k_cell_w;
    static constexpr int k_rownum_w = 5;

    int m_rows, m_cols;
    Config m_cfg;
    int m_cursor_row = 0, m_cursor_col = 0;
    int m_offset_row = 0, m_offset_col = 0;
    int  m_sel_row = 0, m_sel_col = 0;
    bool m_has_selection = false;

    std::vector<std::vector<Cell>> m_cells;
    std::vector<std::string>       m_col_names;
    std::vector<int>               m_col_widths;
    mutable ftxui::Box             m_box              = {};
    std::vector<ActionBox>         m_action_boxes;
    std::vector<ActionBox>         m_col_action_boxes;
    int                            m_pending_delete_row = -1;
    int                            m_pending_delete_col = -1;

    struct HistoryEntry { int row, col; std::string before, after; };
    std::vector<HistoryEntry> m_undo_stack;
    std::vector<HistoryEntry> m_redo_stack;

    std::vector<std::vector<std::string>> m_yank_data;  // [rows][cols] raw values
    int m_yank_row = -1;
    int m_yank_col = -1;

    bool        m_pending_g      = false;
    bool        m_editing        = false;
    bool        m_insert_sticky  = false;
    bool        m_in_header      = false;
    bool        m_header_editing = false;
    std::string m_edit_buf;
    std::string m_edit_orig;
    std::string m_header_edit_buf;
    std::string m_header_edit_orig;

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
    bool select_at_mouse(int mx, int my);
    void scroll_to_mouse_y(int my);
    void start_edit(bool clear);
    void commit_edit();
    void commit_header_edit();
    void extend_selection(int dr, int dc);
    void commit_and_step(int dr, int dc);

    // Event dispatch (called from CatchEvent in make_component)
    bool handle_pending_delete (ftxui::Event e);
    bool handle_header_editing (ftxui::Event e);
    bool handle_header_nav     (ftxui::Event e);
    bool handle_cell_editing   (ftxui::Event e);
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
    std::string col_letter(int c) const;
    std::string unique_col_name(const std::string& name, int skip_col) const;

    // formula autocomplete
    int              m_ac_sel = 0;
    std::vector<int> ac_matches() const;
    ftxui::Element   render_ac_popup() const;
};
