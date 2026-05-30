#include "tab_bar.hpp"

using namespace ftxui;

TabBar::TabBar(const Config& cfg,
               std::function<int()>            active,
               std::function<int()>            count,
               std::function<std::string(int)> name_at,
               std::function<void(int)>        on_select,
               std::function<void()>           on_add,
               std::function<void(int)>        on_rename_request)
    : m_cfg(cfg),
      m_active(std::move(active)),
      m_count(std::move(count)),
      m_name_at(std::move(name_at)),
      m_on_select(std::move(on_select)),
      m_on_add(std::move(on_add)),
      m_on_rename_request(std::move(on_rename_request)) {

    // Per-tab and +-button bounding boxes, refreshed each render via reflect().
    // Stored on a shared component owned by this TabBar so the CatchEvent
    // closure can use them.
    struct State {
        std::vector<Box> tab_boxes;
        Box              add_box{};
    };
    auto state = std::make_shared<State>();

    auto renderer = Renderer([this, state] {
        const int n = m_count();
        state->tab_boxes.assign(n, Box{});

        Elements row;
        row.push_back(text(" "));
        for (int i = 0; i < n; ++i) {
            std::string label = " " + m_name_at(i) + " ";
            auto cell = (i == m_active())
                ? text(label) | bold | color(m_cfg.colors.cursor_bg)
                : text(label) | color(m_cfg.colors.dimmed);
            row.push_back(cell | reflect(state->tab_boxes[i]));
            row.push_back(text("│") | color(m_cfg.colors.dimmed));
        }
        row.push_back(text(" + ") | color(m_cfg.colors.header) | reflect(state->add_box));
        row.push_back(filler());
        return hbox(std::move(row));
    });

    m_container = CatchEvent(renderer, [this, state](Event e) {
        if (!e.is_mouse()) return false;
        if (e.mouse().button != Mouse::Left ||
            e.mouse().motion != Mouse::Pressed) return false;
        const int mx = e.mouse().x, my = e.mouse().y;
        auto inside = [&](const Box& b) {
            return mx >= b.x_min && mx <= b.x_max && my >= b.y_min && my <= b.y_max;
        };
        for (int i = 0; i < (int)state->tab_boxes.size(); ++i) {
            if (inside(state->tab_boxes[i])) {
                if (i == m_active()) m_on_rename_request(i);
                else                 m_on_select(i);
                return true;
            }
        }
        if (inside(state->add_box)) { m_on_add(); return true; }
        return false;
    });
}

Component TabBar::component() { return m_container; }
Element   TabBar::render() const { return m_container->Render(); }
