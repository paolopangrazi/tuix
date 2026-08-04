#pragma once

// Portable environment lookup. MSVC deprecates getenv (C4996) and wants the
// allocating _dupenv_s form, so the platform split lives here once.

#include <cstdlib>
#include <string>

namespace tuix {

// The variable's value, or "" if unset (an empty variable reads as unset).
inline std::string env_var(const char* name) {
#ifdef _MSC_VER
    char* buf = nullptr;
    size_t len = 0;
    if (_dupenv_s(&buf, &len, name) != 0 || !buf) return "";
    std::string val(buf);
    std::free(buf);
    return val;
#else
    const char* v = std::getenv(name);
    return v ? std::string(v) : std::string();
#endif
}

}  // namespace tuix
