// SPDX-License-Identifier: LGPL-2.1-or-later

#include <catch2/catch_test_macros.hpp>

TEST_CASE("T1 smoke test builds and runs with C++20", "[smoke]") {
    constexpr int version_major = 0;
    REQUIRE(version_major == 0);
}
