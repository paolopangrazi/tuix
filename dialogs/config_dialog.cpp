#include "config_dialog.hpp"
#include "dialog_shell.hpp"

#include "config/config.hpp"
#include "config/config_schema.hpp"
#include "util/key_events.hpp"

#include <filesystem>
#include <fstream>
#include <toml++/toml.hpp>

using namespace ftxui;
using namespace tuix::key;

namespace {
std::string kstr(const std::vector<char>& v) {
    std::string r;
    for (char c : v) { if (!r.empty()) r += ' '; r += c; }
    return r;
}
toml::array make_key_arr(const std::string& s) {
    toml::array arr;
    for (char c : s) if (c != ' ') arr.push_back(std::string(1, c));
    return arr;
}
}

ConfigDialog::ConfigDialog(const Config& cfg, std::function<void()> on_close)
    : m_cfg(cfg), m_on_close(std::move(on_close)),
      m_tab_names{"Colors", "Keys", "Grid"} {
    m_color_bufs.resize(k_color_slot_count);
    m_key_bufs.resize(k_key_slot_count);
    refresh_from_cfg();

    InputOption opt;
    opt.multiline = false;   // single-line fields; Enter must not insert '\n'
    for (int i = 0; i < k_color_slot_count; ++i)
        m_color_inputs.push_back(Input(&m_color_bufs[i], "", opt));
    for (int i = 0; i < k_key_slot_count; ++i)
        m_key_inputs.push_back(Input(&m_key_bufs[i], "", opt));
    in_cell_width = Input(&gb_cell_width, "", opt);
    in_start_mode = Input(&gb_start_mode, "", opt);

    m_tab_toggle = Menu(&m_tab_names, &m_tab, MenuOption::Horizontal());

    m_content = Container::Tab({
        Container::Vertical(m_color_inputs),
        Container::Vertical(m_key_inputs),
        Container::Vertical({ in_cell_width, in_start_mode }),
    }, &m_tab);

    m_save_btn = Button("  Save  ", [this]{ save_to_file(); },
                        make_dialog_btn_style(m_cfg));

    m_container = Container::Vertical({ m_tab_toggle, m_content, m_save_btn });
}

void ConfigDialog::refresh_from_cfg() {
    // Theme RGB values have no ANSI name, so color_to_name gives "" — shown as
    // a blank field, and save_to_file leaves blank slots untouched in the file.
    for (int i = 0; i < k_color_slot_count; ++i)
        m_color_bufs[i] = Config::color_to_name(m_cfg.colors.*k_color_slots[i].member);
    for (int i = 0; i < k_key_slot_count; ++i)
        m_key_bufs[i] = kstr(m_cfg.keys.*k_key_slots[i].member);
    gb_cell_width = std::to_string(m_cfg.grid.cell_width);
    gb_start_mode = m_cfg.grid.start_insert ? "insert" : "normal";
    m_tab = 0;
    m_status.clear();
}

void ConfigDialog::save_to_file() {
    const auto path = Config::config_file_path();
    if (path.empty()) { m_status = "Cannot save: no config path (HOME unset)"; return; }

    // Read-modify-write: everything this editor doesn't manage — the [theme]
    // block, slots left blank here, hand-written extras — survives a save.
    toml::table tbl;
    try { tbl = toml::parse_file(path.string()); } catch (...) { /* start fresh */ }

    auto section = [&](const char* name) -> toml::table& {
        if (!tbl[name].as_table()) tbl.insert_or_assign(name, toml::table{});
        return *tbl[name].as_table();
    };

    toml::table& ct = section("colors");
    for (int i = 0; i < k_color_slot_count; ++i)
        if (!m_color_bufs[i].empty())   // blank = keep whatever the file has
            ct.insert_or_assign(k_color_slots[i].key, m_color_bufs[i]);

    toml::table& kt = section("keys");
    for (int i = 0; i < k_key_slot_count; ++i)
        if (!m_key_bufs[i].empty())
            kt.insert_or_assign(k_key_slots[i].key, make_key_arr(m_key_bufs[i]));

    toml::table& gt = section("grid");
    try { gt.insert_or_assign("cell_width", std::stoi(gb_cell_width)); } catch (...) {}
    gt.insert_or_assign("start_mode", (gb_start_mode == "insert") ? "insert" : "normal");

    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    out << tbl;
    m_status = "Saved to " + path.string() + "  —  restart to apply";
}

Component ConfigDialog::component() {
    auto renderer = Renderer(m_container, [this] {
        constexpr int k_label_w = 16, k_value_w = 14;   // field-row column widths
        auto crow = [&](const char* label, const Component& inp) -> Element {
            return hbox({
                text("  "),
                text(label) | size(WIDTH, EQUAL, k_label_w) | color(m_cfg.colors.dimmed),
                inp->Render() | size(WIDTH, EQUAL, k_value_w),
            });
        };
        // The color/key lists are long, so lay their rows out in two columns.
        auto two_col = [&](const std::vector<Component>& inputs,
                           auto&& label_of) -> Element {
            const int n = (int)inputs.size(), half = (n + 1) / 2;
            Elements left, right;
            for (int i = 0; i < n; ++i)
                (i < half ? left : right).push_back(crow(label_of(i), inputs[i]));
            return hbox({ vbox(std::move(left)), text("  "), vbox(std::move(right)) });
        };

        Element page;
        switch (m_tab) {
        default:
        case 0: {
            const char* color_hint =
                "ANSI names (green, gray_dark, cyan_light, ...), #rrggbb, or rgb(r,g,b).\n"
                "Blank = keep the current (theme) value.";
            page = vbox({
                two_col(m_color_inputs, [](int i) { return k_color_slots[i].key; }),
                text(""),
                hbox({ text("  "), paragraph(color_hint) | color(m_cfg.colors.dimmed) }),
            });
            break;
        }
        case 1: {
            const char* keys_hint =
                "Space-separated characters (e.g.  i a).  Blank = keep default.";
            page = vbox({
                two_col(m_key_inputs, [](int i) { return k_key_slots[i].key; }),
                text(""),
                hbox({ text("  "), text(keys_hint) | color(m_cfg.colors.dimmed) }),
            });
            break;
        }
        case 2: {
            const char* grid_hint = "start_mode: normal  or  insert";
            page = vbox({
                crow("cell_width",  in_cell_width),
                crow("start_mode",  in_start_mode),
                text(""),
                hbox({ text("  "), text(grid_hint) | color(m_cfg.colors.dimmed) }),
            });
            break;
        }
        }
        auto inner = window(
            hbox({ text(" "), text("Configuration") | bold, text(" ") }),
            vbox({
                m_tab_toggle->Render(),
                separator(),
                page | size(WIDTH, EQUAL, 2 * (k_label_w + k_value_w + 2) + 4),
                separator(),
                hbox({ filler(), m_save_btn->Render(), filler() }),
            })
        ) | center;
        auto bottom = flexbox({
            m_status.empty()
                ? text("")
                : hbox({ text(" "),
                         text(m_status) | color(m_cfg.colors.header),
                         text("  ") }),
            hbox({ text(" "),
                   text("Ctrl+W") | bold | color(m_cfg.colors.header),
                   text("  save  ") | color(m_cfg.colors.dimmed) }),
            hbox({ text("← →") | bold | color(m_cfg.colors.header),
                   text("  switch tab  ") | color(m_cfg.colors.dimmed) }),
            hbox({ text("Tab") | bold | color(m_cfg.colors.header),
                   text("  next field  ") | color(m_cfg.colors.dimmed) }),
            hbox({ text("Esc") | bold | color(m_cfg.colors.header),
                   text("  close  ") | color(m_cfg.colors.dimmed) }),
        }, FlexboxConfig()
            .Set(FlexboxConfig::Wrap::Wrap)
            .Set(FlexboxConfig::JustifyContent::FlexStart)
            .Set(FlexboxConfig::AlignItems::FlexStart)
            .Set(FlexboxConfig::AlignContent::FlexStart));
        return render_dialog_shell(inner, bottom);
    });
    return add_escape_to_close(renderer, m_on_close, [this](Event e) {
        if (e == CtrlW) { save_to_file(); return true; }
        return false;
    });
}
