// The single translation unit that compiles doctest's implementation and main().
// All other test files include <doctest/doctest.h> without this define.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
