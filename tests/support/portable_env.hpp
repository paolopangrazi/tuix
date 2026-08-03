#pragma once

// Cross-platform shims for the couple of POSIX-isms the config tests need.
// The originals (`<unistd.h>`, `getpid`, `setenv`) don't exist under MSVC, so
// these route to the Win32 equivalents there and to the POSIX calls elsewhere.

#include <string>

#ifdef _WIN32
#include <process.h>   // _getpid
#include <cstdlib>     // _putenv_s
#else
#include <cstdlib>     // setenv
#include <unistd.h>    // getpid
#endif

namespace tuix::test {

// A value unique to the running process, for building private temp paths.
inline std::string unique_tag() {
#ifdef _WIN32
    return std::to_string(::_getpid());
#else
    return std::to_string(::getpid());
#endif
}

// Set an environment variable, overwriting any existing value.
inline void set_env(const char* name, const std::string& value) {
#ifdef _WIN32
    _putenv_s(name, value.c_str());
#else
    setenv(name, value.c_str(), /*overwrite=*/1);
#endif
}

}  // namespace tuix::test
