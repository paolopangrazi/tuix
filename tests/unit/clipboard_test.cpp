#include <doctest/doctest.h>

#include "util/clipboard.hpp"

using tuix::detail::base64_encode;

TEST_CASE("base64_encode handles padding lengths") {
    // Classic RFC 4648 test vectors.
    CHECK(base64_encode("") == "");
    CHECK(base64_encode("f") == "Zg==");
    CHECK(base64_encode("fo") == "Zm8=");
    CHECK(base64_encode("foo") == "Zm9v");
    CHECK(base64_encode("foob") == "Zm9vYg==");
    CHECK(base64_encode("fooba") == "Zm9vYmE=");
    CHECK(base64_encode("foobar") == "Zm9vYmFy");
}

TEST_CASE("base64_encode round-trips spreadsheet-style TSV") {
    CHECK(base64_encode("a\tb\nc\td") == "YQliCmMJZA==");
}

TEST_CASE("base64_encode preserves high bytes") {
    std::string bytes;
    bytes.push_back((char)0xFF);
    bytes.push_back((char)0x00);
    bytes.push_back((char)0xAA);
    CHECK(base64_encode(bytes) == "/wCq");
}
