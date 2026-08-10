#pragma once
#include <vector>

namespace tuix {

// One "Did you know?" card: a short headline, the keys it is about (rendered
// as a highlighted chip, may be empty), and one or two sentences of prose.
struct Tip {
    const char* title;
    const char* keys;
    const char* body;
};

// The tips shown by the startup popup (dialogs/tips_dialog.cpp), in order.
// Keep them in sync with the keybindings documented in dialogs/help_dialog.cpp.
const std::vector<Tip>& tips();

}  // namespace tuix
