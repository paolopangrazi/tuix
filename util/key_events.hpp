#pragma once
#include <ftxui/component/event.hpp>

// Named ftxui::Event matchers for terminal sequences FTXUI has no built-in
// name for (Ctrl+<letter> control characters, Shift/Ctrl+Arrow and
// Ctrl+PageUp/PageDown CSI sequences). FTXUI's own named events
// (Event::ArrowUp, Event::Return, Event::F1, ...) already cover the rest —
// only the raw "\x.." cases some code needed are centralized here, so each
// sequence has one name and one place to fix if a terminal encodes it
// differently.
namespace tuix::key {

// Ctrl+<letter> control characters (Ctrl+A=0x01 ... Ctrl+Z=0x1A).
inline const ftxui::Event CtrlE = ftxui::Event::Special("\x05");  // toggle exit confirm
inline const ftxui::Event CtrlR = ftxui::Event::Special("\x12");  // redo
inline const ftxui::Event CtrlT = ftxui::Event::Special("\x14");  // new sheet tab
inline const ftxui::Event CtrlW = ftxui::Event::Special("\x17");  // save config

// Shift+Arrow (CSI sequences FTXUI doesn't name itself) — extend selection.
inline const ftxui::Event ShiftArrowUp    = ftxui::Event::Special("\x1B[1;2A");
inline const ftxui::Event ShiftArrowDown  = ftxui::Event::Special("\x1B[1;2B");
inline const ftxui::Event ShiftArrowRight = ftxui::Event::Special("\x1B[1;2C");
inline const ftxui::Event ShiftArrowLeft  = ftxui::Event::Special("\x1B[1;2D");

// Ctrl+PageUp / Ctrl+PageDown — cycle sheet tabs (previous / next).
inline const ftxui::Event CtrlPageUp   = ftxui::Event::Special("\x1B[5;5~");
inline const ftxui::Event CtrlPageDown = ftxui::Event::Special("\x1B[6;5~");

}  // namespace tuix::key
