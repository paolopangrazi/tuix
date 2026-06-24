#include "util/clipboard.hpp"

#include <cstdio>
#include <cstdlib>

namespace tuix {

namespace detail {

std::string base64_encode(const std::string& in) {
    static const char tbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string out;
    out.reserve((in.size() + 2) / 3 * 4);

    size_t i = 0;
    for (; i + 3 <= in.size(); i += 3) {
        unsigned n = (unsigned char)in[i] << 16 |
                     (unsigned char)in[i + 1] << 8 |
                     (unsigned char)in[i + 2];
        out += tbl[(n >> 18) & 63];
        out += tbl[(n >> 12) & 63];
        out += tbl[(n >> 6) & 63];
        out += tbl[n & 63];
    }

    if (i + 1 == in.size()) {
        unsigned n = (unsigned char)in[i] << 16;
        out += tbl[(n >> 18) & 63];
        out += tbl[(n >> 12) & 63];
        out += "==";
    } else if (i + 2 == in.size()) {
        unsigned n = (unsigned char)in[i] << 16 | (unsigned char)in[i + 1] << 8;
        out += tbl[(n >> 18) & 63];
        out += tbl[(n >> 12) & 63];
        out += tbl[(n >> 6) & 63];
        out += '=';
    }

    return out;
}

}  // namespace detail

void copy_to_clipboard(const std::string& text) {
    // OSC 52: ESC ] 52 ; c ; <base64> BEL  — "c" targets the clipboard selection.
    std::string seq = "\033]52;c;" + detail::base64_encode(text) + "\a";

    // Inside tmux the sequence must be wrapped in a DCS passthrough, with every
    // inner ESC byte doubled (requires `set -g allow-passthrough on`).
    if (const char* tmux = std::getenv("TMUX"); tmux && *tmux) {
        std::string wrapped = "\033Ptmux;";
        for (char ch : seq) {
            wrapped += ch;
            if (ch == '\033') wrapped += '\033';
        }
        wrapped += "\033\\";
        seq = std::move(wrapped);
    }

    // Write straight to the controlling terminal so we don't fight FTXUI's
    // own stdout rendering. OSC 52 is visually inert, so this is safe to
    // interleave. Fall back to stdout if there is no /dev/tty.
    std::FILE* tty = std::fopen("/dev/tty", "w");
    if (!tty) tty = stdout;
    std::fwrite(seq.data(), 1, seq.size(), tty);
    std::fflush(tty);
    if (tty != stdout) std::fclose(tty);
}

}  // namespace tuix
