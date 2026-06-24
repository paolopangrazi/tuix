#pragma once

#include <string>

namespace tuix {

// Copy `text` onto the host system clipboard using the OSC 52 terminal escape.
// This works over SSH and inside tmux (via passthrough), since the copy is
// performed by the terminal emulator rather than a local clipboard daemon.
// Best-effort: silently does nothing if no terminal is available.
void copy_to_clipboard(const std::string& text);

namespace detail {
// Standard base64 (RFC 4648) with '=' padding. Exposed for testing.
std::string base64_encode(const std::string& in);
}  // namespace detail

}  // namespace tuix
