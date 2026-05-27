// SPDX-License-Identifier: BSD-2-Clause
#include <catch2/catch_test_macros.hpp>
#include <cdcommons/time/rational.hpp>
#include <limits>

using R = cdcommons::time::rational<int>;
using RL = cdcommons::time::rational<long>;

static const R inf = std::numeric_limits<R>::infinity();

TEST_CASE("rational: default construction is zero", "[rational]") {
    R r;
    REQUIRE(r.numerator() == 0);
    REQUIRE(r.denominator() == 1);
}

TEST_CASE("rational: integer construction", "[rational]") {
    R r(5);
    REQUIRE(r.numerator() == 5);
    REQUIRE(r.denominator() == 1);
}

TEST_CASE("rational: canonical form — gcd reduction", "[rational]") {
    R a(2, 4);
    REQUIRE(a.numerator() == 1);
    REQUIRE(a.denominator() == 2);

    R b(6, 9);
    REQUIRE(b.numerator() == 2);
    REQUIRE(b.denominator() == 3);

    R c(10, 5);
    REQUIRE(c.numerator() == 2);
    REQUIRE(c.denominator() == 1);
}

TEST_CASE("rational: negative denominator is normalized", "[rational]") {
    R a(1, -2);
    REQUIRE(a.numerator() == -1);
    REQUIRE(a.denominator() == 2);

    R b(-3, -4);
    REQUIRE(b.numerator() == 3);
    REQUIRE(b.denominator() == 4);
}

TEST_CASE("rational: zero denominator throws", "[rational]") {
    REQUIRE_THROWS_AS(R(1, 0), std::domain_error);
}

TEST_CASE("rational: equality", "[rational]") {
    REQUIRE(R(1, 2) == R(2, 4));
    REQUIRE(R(0, 3) == R(0, 7));
    REQUIRE_FALSE(R(1, 2) == R(1, 3));
}

TEST_CASE("rational: comparison (spaceship)", "[rational]") {
    REQUIRE(R(1, 3) < R(1, 2));
    REQUIRE(R(1, 2) > R(1, 3));
    REQUIRE(R(1, 2) <= R(1, 2));
    REQUIRE(R(2, 3) >= R(2, 3));
    REQUIRE(R(0, 1) < R(1, 10));
    REQUIRE(R(-1, 3) < R(1, 3));
}

TEST_CASE("rational: addition", "[rational]") {
    REQUIRE(R(1, 3) + R(1, 3) == R(2, 3));
    REQUIRE(R(1, 2) + R(1, 3) == R(5, 6));
    REQUIRE(R(1, 4) + R(3, 4) == R(1, 1));
    REQUIRE(R(0, 1) + R(5, 7) == R(5, 7));
}

TEST_CASE("rational: subtraction", "[rational]") {
    REQUIRE(R(3, 4) - R(1, 4) == R(1, 2));
    REQUIRE(R(1, 2) - R(1, 3) == R(1, 6));
    REQUIRE(R(5, 7) - R(5, 7) == R(0, 1));
    REQUIRE(R(1, 1) - R(1, 2) == R(1, 2));
}

TEST_CASE("rational: multiplication", "[rational]") {
    REQUIRE(R(2, 3) * R(3, 4) == R(1, 2));
    REQUIRE(R(1, 2) * R(2, 1) == R(1, 1));
    REQUIRE(R(0, 1) * R(5, 7) == R(0, 1));
}

TEST_CASE("rational: infinity sentinel", "[rational]") {
    REQUIRE(inf.numerator() == std::numeric_limits<int>::max());
    REQUIRE(inf.denominator() == 1);
    REQUIRE(std::numeric_limits<R>::has_infinity);
    REQUIRE(std::numeric_limits<R>::infinity() == inf);
}

TEST_CASE("rational: infinity arithmetic propagation", "[rational]") {
    R t(3, 2);
    REQUIRE(inf + t == inf);
    REQUIRE(t + inf == inf);
    REQUIRE(inf - t == inf);
    REQUIRE(inf * t == inf);
    REQUIRE(inf + inf == inf);
}

TEST_CASE("rational: infinity comparison", "[rational]") {
    REQUIRE(R(1000000, 1) < inf);
    REQUIRE(inf > R(0, 1));
    REQUIRE(inf == inf);
}

TEST_CASE("rational: long int variant", "[rational]") {
    RL a(1L, 3L);
    RL b(1L, 6L);
    REQUIRE(a + b == RL(1L, 2L));
    auto linf = std::numeric_limits<RL>::infinity();
    REQUIRE(std::numeric_limits<RL>::has_infinity);
    REQUIRE(linf == RL(std::numeric_limits<long>::max(), 1L));
}

TEST_CASE("rational: accumulation (VDW14 scenario)", "[rational]") {
    // Simulate adding 1/10 ten times to get 1 — rational stays exact.
    R acc;
    const R step(1, 10);
    for (int i = 0; i < 10; ++i) {
        acc = acc + step;
    }
    REQUIRE(acc == R(1, 1));
}
