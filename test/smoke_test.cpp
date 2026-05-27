// SPDX-License-Identifier: BSD-2-Clause
#include <catch2/catch_test_macros.hpp>
#include <cdcommons/cdcommons.hpp>

TEST_CASE("cdcommons header is includable", "[smoke]") {
    SUCCEED("cdcommons/cdcommons.hpp compiled successfully");
}
