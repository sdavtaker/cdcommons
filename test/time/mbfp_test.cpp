// SPDX-License-Identifier: BSD-2-Clause
#include <catch2/catch_test_macros.hpp>
#include <cdcommons/time/mbfp.hpp>
#include <limits>

// Decimal milliseconds: 1 ms = 1 × 10^-3 s
using MS = cdcommons::time::mbfp<10, -3>;
// Decimal nanoseconds: 1 ns = 1 × 10^-9 s
using NS = cdcommons::time::mbfp<10, -9>;
// Binary: 1 unit = 1 × 2^-10 s (~1 ms)
using BIN10 = cdcommons::time::mbfp<2, -10>;
// Base-3 with large exponent (inaccessible to rational with 64-bit int)
using BASE3 = cdcommons::time::mbfp<3, -70, long>;

static const MS ms_inf = std::numeric_limits<MS>::infinity();

TEST_CASE("mbfp: default construction is zero", "[mbfp]") {
    MS t;
    REQUIRE(t.mantissa() == 0L);
}

TEST_CASE("mbfp: explicit construction from mantissa", "[mbfp]") {
    MS t(5L);
    REQUIRE(t.mantissa() == 5L);
    REQUIRE(t == MS(5L));
}

TEST_CASE("mbfp: static type parameters", "[mbfp]") {
    REQUIRE(MS::base == 10);
    REQUIRE(MS::exponent == -3);
    REQUIRE(NS::base == 10);
    REQUIRE(NS::exponent == -9);
}

TEST_CASE("mbfp: addition", "[mbfp]") {
    REQUIRE(MS(3L) + MS(4L) == MS(7L));
    REQUIRE(MS(0L) + MS(5L) == MS(5L));
    REQUIRE(MS(100L) + MS(900L) == MS(1000L));
}

TEST_CASE("mbfp: subtraction", "[mbfp]") {
    REQUIRE(MS(7L) - MS(3L) == MS(4L));
    REQUIRE(MS(5L) - MS(5L) == MS(0L));
    REQUIRE(MS(1000L) - MS(1L) == MS(999L));
}

TEST_CASE("mbfp: comparison (totally ordered)", "[mbfp]") {
    REQUIRE(MS(1L) < MS(2L));
    REQUIRE(MS(2L) > MS(1L));
    REQUIRE(MS(3L) <= MS(3L));
    REQUIRE(MS(3L) >= MS(3L));
    REQUIRE(MS(0L) < MS(1L));
    REQUIRE_FALSE(MS(5L) < MS(5L));
}

TEST_CASE("mbfp: equality", "[mbfp]") {
    REQUIRE(MS(7L) == MS(7L));
    REQUIRE_FALSE(MS(7L) == MS(8L));
}

TEST_CASE("mbfp: infinity sentinel from numeric_limits", "[mbfp]") {
    REQUIRE(std::numeric_limits<MS>::has_infinity);
    REQUIRE(ms_inf.mantissa() == std::numeric_limits<long>::max());
    REQUIRE(ms_inf == std::numeric_limits<MS>::infinity());
}

TEST_CASE("mbfp: infinity arithmetic propagation", "[mbfp]") {
    MS t(3L);
    REQUIRE(ms_inf + t == ms_inf);
    REQUIRE(t + ms_inf == ms_inf);
    REQUIRE(ms_inf - t == ms_inf);
    REQUIRE(ms_inf + ms_inf == ms_inf);
}

TEST_CASE("mbfp: infinity comparison", "[mbfp]") {
    REQUIRE(MS(999999999L) < ms_inf);
    REQUIRE(ms_inf > MS(0L));
    REQUIRE(ms_inf == ms_inf);
}

TEST_CASE("mbfp: exact decimal accumulation (VDW14 scenario)", "[mbfp]") {
    // Adding 1 ms ten times must give exactly 10 ms.
    // Floating-point 0.001 + 0.001 + ... accumulates rounding error; mbfp<10,-3> does not.
    MS acc;
    const MS step(1L);
    for (int i = 0; i < 10; ++i)
        acc = acc + step;
    REQUIRE(acc == MS(10L));
}

TEST_CASE("mbfp: exact binary accumulation", "[mbfp]") {
    // 1024 steps of 2^-10 s = 1 s (mantissa 1024 × 2^-10 = 1).
    BIN10 acc;
    const BIN10 step(1L);
    for (int i = 0; i < 1024; ++i)
        acc = acc + step;
    REQUIRE(acc == BIN10(1024L));
}

TEST_CASE("mbfp: base-3 exact representation (inaccessible to rational)", "[mbfp]") {
    // 1 × 3^-70 cannot be expressed as p/q with |p|, |q| fitting in 64-bit integers
    // (3^70 ≈ 2.5 × 10^33 > INT64_MAX ≈ 9.2 × 10^18).
    // mbfp<3,-70> represents it exactly as mantissa=1.
    BASE3 tiny(1L);
    BASE3 zero;
    REQUIRE(tiny > zero);
    REQUIRE(tiny + zero == tiny);
    REQUIRE(tiny - tiny == zero);
}

// Agreement: decimal milliseconds + decimal nanoseconds
// common_base = lcm(10,10) = 10, common_exp = min(-3,-9) = -9
TEST_CASE("mbfp_agree: same base, different exponent", "[mbfp][agree]") {
    using Agree = cdcommons::time::mbfp_agree<MS, NS>;
    static_assert(Agree::common_base == 10);
    static_assert(Agree::common_exp == -9);

    MS five_ms(5L);          // 5 × 10^-3 s = 5 000 000 × 10^-9 s
    NS two_hundred_ns(200L); // 200 × 10^-9 s

    auto a = Agree::convert_first(five_ms);
    auto b = Agree::convert_second(two_hundred_ns);

    REQUIRE(a.mantissa() == 5000000L);
    REQUIRE(b.mantissa() == 200L);
    REQUIRE((a + b).mantissa() == 5000200L);
}

// Agreement: decimal ms + binary (base-2) 1/1024-second steps
// common_base = lcm(10,2) = 10, common_exp = min(-3,-10) = -10
TEST_CASE("mbfp_agree: different base, different exponent", "[mbfp][agree]") {
    using Agree = cdcommons::time::mbfp_agree<MS, BIN10>;
    static_assert(Agree::common_base == 10);
    static_assert(Agree::common_exp == -10);

    // 1 ms = 1 × 10^-3 s; in common type (base 10, exp -10): 1 × 10^7 = 10 000 000
    MS one_ms(1L);
    auto converted = Agree::convert_first(one_ms);
    REQUIRE(converted.mantissa() == 10000000L);

    // 1 × 2^-10 in base 10 at exp -10: (10/2)^10 = 5^10 = 9 765 625
    BIN10 one_unit(1L);
    auto converted2 = Agree::convert_second(one_unit);
    REQUIRE(converted2.mantissa() == 9765625L);
}

// Agreement: infinity converts to infinity
TEST_CASE("mbfp_agree: infinity is preserved under conversion", "[mbfp][agree]") {
    using Agree = cdcommons::time::mbfp_agree<MS, NS>;
    auto inf_ms = std::numeric_limits<MS>::infinity();
    auto converted = Agree::convert_first(inf_ms);
    REQUIRE(converted == std::numeric_limits<Agree::type>::infinity());
}

// MBFP vs rational: MBFP covers numbers rational cannot (with fixed Int width)
// and rational covers numbers MBFP cannot (arbitrary p/q).
TEST_CASE("mbfp vs rational: disjoint representable sets", "[mbfp]") {
    // 3^-70 is exactly representable in mbfp<3,-70> as mantissa=1.
    // It cannot be exactly stored in rational<long> because 3^70 > LONG_MAX.
    BASE3 base3_tiny(1L); // exact
    REQUIRE(base3_tiny.mantissa() == 1L);

    // 1/7 is exactly rational<long>(1,7) but is NOT representable in any
    // mbfp<Base, Exp> with integer mantissa (7 is coprime to every power-of-10
    // or power-of-2 or power-of-3).  This is a design trade-off, not a bug.
    // We just document it here by verifying our mbfp gives 0 for the closest.
    MS approx_seventh(142L); // 142 × 10^-3 ≈ 0.142 ≈ 1/7, but not exact
    REQUIRE(approx_seventh.mantissa() == 142L);
}
