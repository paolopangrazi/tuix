#pragma once

#include <string>

namespace tuix {

// Outcome of a native "Open File" picker.
//   Ok          — user chose a file; `path` holds its UTF-8 filesystem path.
//   Cancelled   — a native dialog was shown but the user dismissed it.
//   Unavailable — no native dialog exists on this platform, or it couldn't be
//                 created (e.g. no desktop/COM). Callers should fall back to
//                 the in-TUI typed-path dialog.
enum class FileDialogStatus { Ok, Cancelled, Unavailable };

struct OpenFileResult {
    FileDialogStatus status = FileDialogStatus::Unavailable;
    std::string      path;   // valid only when status == Ok
};

// Show the OS-native "Open File" dialog. Implemented on Windows (the
// Explorer-style IFileOpenDialog) and macOS (Finder's Open panel via
// osascript); every other platform returns {Unavailable, ""} so the caller
// keeps its existing typed-path flow.
// `initial_dir` (UTF-8) seeds the starting folder when non-empty.
OpenFileResult native_open_file(const std::string& initial_dir = {});

}  // namespace tuix
