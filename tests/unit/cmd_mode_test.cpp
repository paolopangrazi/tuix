#include <doctest/doctest.h>

#include <string>

#include <ftxui/component/event.hpp>

#include "components/cmd_mode.hpp"

// The `:` command line's dispatch: :q vs :q!, and unknown-command feedback.

namespace {

struct Fired {
    int quit = 0, force_quit = 0, save = 0, save_quit = 0;
    std::string unknown;
};

CmdMode make(Fired& f) {
    return CmdMode({
        /* quit       */ [&] { ++f.quit; },
        /* force_quit */ [&] { ++f.force_quit; },
        /* save       */ [&] { ++f.save; },
        /* save_quit  */ [&] { ++f.save_quit; },
        /* save_as    */ nullptr,
        /* edit       */ nullptr,
        /* goto_cell  */ nullptr,
        /* replace    */ nullptr,
        /* sort       */ nullptr,
        /* unknown    */ [&](const std::string& cmd) { f.unknown = cmd; },
    });
}

// Type `cmd` (without the leading ':') and press Enter.
void run_cmd(CmdMode& m, const std::string& cmd) {
    m.enter();
    for (char c : cmd) m.handle(ftxui::Event::Character(std::string(1, c)));
    m.handle(ftxui::Event::Return);
}

}  // namespace

TEST_CASE(":q confirms, :q! force-quits") {
    Fired f;
    CmdMode m = make(f);
    run_cmd(m, "q");
    CHECK(f.quit == 1);
    CHECK(f.force_quit == 0);
    run_cmd(m, "q!");
    CHECK(f.quit == 1);
    CHECK(f.force_quit == 1);
    CHECK(f.unknown.empty());
}

TEST_CASE("recognized commands don't trip the unknown handler") {
    Fired f;
    CmdMode m = make(f);
    run_cmd(m, "w");
    run_cmd(m, "wq");
    CHECK(f.save == 1);
    CHECK(f.save_quit == 1);
    CHECK(f.unknown.empty());
}

TEST_CASE("unrecognized commands are reported") {
    Fired f;
    CmdMode m = make(f);
    run_cmd(m, "frobnicate");
    CHECK(f.unknown == ":frobnicate");
    // A bare ':' followed by Enter is a no-op, not an error.
    f.unknown.clear();
    run_cmd(m, "");
    CHECK(f.unknown.empty());
}
