// SPDX-License-Identifier: BSD-2-Clause
#include <catch2/catch_test_macros.hpp>
#include <cdcommons/time/rsfp.hpp>
#include <limits>

// Decimal milliseconds: 1 unit = 1 × (1/1000) s
using MS = cdcommons::time::rsfp<1, 1000>;
// Decimal microseconds
using US = cdcommons::time::rsfp<1, 1000000>;
// Tenths: exact 0.1 representation
using TENTH = cdcommons::time::rsfp<1, 10>;
// Different numerator
using TWO_THIRDS_STEP = cdcommons::time::rsfp<2, 3>;

static const MS ms_inf = std::numeric_limits<MS>::infinity();
static const MS ms_neg_inf = std::numeric_limits<MS>::neg_infinity();

TEST_CASE("rsfp: default construction is zero", "[rsfp]") {
    MS t;
    REQUIRE(t.mantissa() == 0);
}

TEST_CASE("rsfp: explicit construction from mantissa", "[rsfp]") {
    MS t(5);
    REQUIRE(t.mantissa() == 5);
    REQUIRE(t == MS(5));
}

TEST_CASE("rsfp: static type parameters", "[rsfp]") {
    REQUIRE(MS::num == 1);
    REQUIRE(MS::den == 1000);
    REQUIRE(US::num == 1);
    REQUIRE(US::den == 1000000);
}

TEST_CASE("rsfp: addition", "[rsfp]") {
    REQUIRE(MS(3) + MS(4) == MS(7));
    REQUIRE(MS(0) + MS(5) == MS(5));
    REQUIRE(MS(100) + MS(900) == MS(1000));
}

TEST_CASE("rsfp: subtraction", "[rsfp]") {
    REQUIRE(MS(7) - MS(3) == MS(4));
    REQUIRE(MS(5) - MS(5) == MS(0));
    REQUIRE(MS(1000) - MS(1) == MS(999));
}

TEST_CASE("rsfp: comparison (totally ordered)", "[rsfp]") {
    REQUIRE(MS(1) < MS(2));
    REQUIRE(MS(2) > MS(1));
    REQUIRE(MS(3) <= MS(3));
    REQUIRE(MS(3) >= MS(3));
    REQUIRE(MS(0) < MS(1));
    REQUIRE_FALSE(MS(5) < MS(5));
}

TEST_CASE("rsfp: equality", "[rsfp]") {
    REQUIRE(MS(7) == MS(7));
    REQUIRE_FALSE(MS(7) == MS(8));
}

TEST_CASE("rsfp: infinity sentinel from numeric_limits", "[rsfp]") {
    REQUIRE(std::numeric_limits<MS>::has_infinity);
    REQUIRE(ms_inf == std::numeric_limits<MS>::infinity());
    REQUIRE(ms_inf.mantissa() == std::numeric_limits<std::int32_t>::max());
    REQUIRE(ms_neg_inf == std::numeric_limits<MS>::neg_infinity());
    REQUIRE(ms_neg_inf.mantissa() == std::numeric_limits<std::int32_t>::min());
}

TEST_CASE("rsfp: infinity arithmetic propagation", "[rsfp]") {
    MS t(3);
    REQUIRE(ms_inf + t == ms_inf);
    REQUIRE(t + ms_inf == ms_inf);
    REQUIRE(ms_inf - t == ms_inf);
    REQUIRE(ms_inf + ms_inf == ms_inf);
    REQUIRE(ms_neg_inf + t == ms_neg_inf);
    REQUIRE(t + ms_neg_inf == ms_neg_inf);
    REQUIRE(ms_neg_inf - t == ms_neg_inf);
    REQUIRE(ms_neg_inf + ms_neg_inf == ms_neg_inf);
    // x - (-inf) = +inf; x - (+inf) = -inf
    REQUIRE(t - ms_neg_inf == ms_inf);
    REQUIRE(t - ms_inf == ms_neg_inf);
}

TEST_CASE("rsfp: undefined infinity arithmetic throws domain_error", "[rsfp]") {
    REQUIRE_THROWS_AS(ms_inf + ms_neg_inf, std::domain_error);
    REQUIRE_THROWS_AS(ms_neg_inf + ms_inf, std::domain_error);
    REQUIRE_THROWS_AS(ms_inf - ms_inf, std::domain_error);
    REQUIRE_THROWS_AS(ms_neg_inf - ms_neg_inf, std::domain_error);
}

TEST_CASE("rsfp: infinity comparison", "[rsfp]") {
    REQUIRE(MS(1000000000) < ms_inf);
    REQUIRE(ms_inf > MS(0));
    REQUIRE(ms_inf == ms_inf);
}

TEST_CASE("rsfp: exact representation of 0.1 (VDW14 scenario)", "[rsfp]") {
    // With plain double, 0.1 + 0.1 + ... (10×) != 1.0 due to rounding.
    // With rsfp<1,10>, the mantissa is always an integer — no rounding error.
    TENTH acc;
    const TENTH step(1); // represents exactly 1 × (1/10) = 0.1 s
    for (int i = 0; i < 10; ++i)
        acc = acc + step;
    REQUIRE(acc == TENTH(10)); // 10 × (1/10) = 1.0 s, mantissa == 10 exactly

    // Confirm the plain double path DOES accumulate error (motivating RSFP).
    double d = 0.0;
    for (int i = 0; i < 10; ++i)
        d += 0.1;
    REQUIRE(d != 1.0); // floating-point drift
}

TEST_CASE("rsfp: addition overflow saturates to infinity", "[rsfp]") {
    // max() is INT32_MAX - 1; adding 2 would exceed the sentinel, so it saturates.
    MS near_max = std::numeric_limits<MS>::max();
    REQUIRE((near_max + MS(2)) == ms_inf);
    // Adding 1 to max() also hits the sentinel value exactly.
    REQUIRE((near_max + MS(1)) == ms_inf);
    // Adding 0 is fine — stays at max().
    REQUIRE((near_max + MS(0)) == near_max);
}

// Agreement: ms + µs → common scale 1/lcm(1000,1000000) = 1/1000000
TEST_CASE("rsfp_agree: same numerator, different denominator", "[rsfp][agree]") {
    using Agree = cdcommons::time::rsfp_agree<MS, US>;
    static_assert(Agree::common_num == 1);
    static_assert(Agree::common_den == 1000000);

    MS five_ms(5);          // 5 × (1/1000) s = 5000 × (1/1000000) s
    US two_hundred_us(200); // 200 × (1/1000000) s

    auto a = Agree::convert_first(five_ms);
    auto b = Agree::convert_second(two_hundred_us);

    REQUIRE(a.mantissa() == 5000);
    REQUIRE(b.mantissa() == 200);
    REQUIRE((a + b).mantissa() == 5200);
}

// Agreement: rsfp<1,3> + rsfp<2,5>
// common_num = gcd(1,2) = 1, common_den = lcm(3,5) = 15
// scale1 = (1×15)/(3×1) = 5, scale2 = (2×15)/(5×1) = 6
TEST_CASE("rsfp_agree: different numerator and denominator", "[rsfp][agree]") {
    using A = cdcommons::time::rsfp<1, 3>;
    using B = cdcommons::time::rsfp<2, 5>;
    using Agree = cdcommons::time::rsfp_agree<A, B>;
    static_assert(Agree::common_num == 1);
    static_assert(Agree::common_den == 15);
    static_assert(Agree::scale1 == 5);
    static_assert(Agree::scale2 == 6);

    auto ca = Agree::convert_first(A(1));  // 1 × (1/3) → 5 × (1/15)
    auto cb = Agree::convert_second(B(1)); // 1 × (2/5) → 6 × (1/15)

    REQUIRE(ca.mantissa() == 5);
    REQUIRE(cb.mantissa() == 6);
}

TEST_CASE("rsfp_agree: scale factors are correct compile-time constants", "[rsfp][agree]") {
    using Agree = cdcommons::time::rsfp_agree<MS, US>;
    REQUIRE(Agree::scale1 == 1000); // ms → µs: ×1000
    REQUIRE(Agree::scale2 == 1);    // µs → µs: ×1
}

TEST_CASE("rsfp_agree: infinity is preserved under conversion", "[rsfp][agree]") {
    using Agree = cdcommons::time::rsfp_agree<MS, US>;
    auto converted = Agree::convert_first(ms_inf);
    REQUIRE(converted == std::numeric_limits<Agree::type>::infinity());
}

TEST_CASE("rsfp_agree: large finite mantissa saturates to infinity on conversion",
          "[rsfp][agree]") {
    // scale1 = 1000 (ms → µs); near_max × 1000 overflows int32_t — must saturate.
    using Agree = cdcommons::time::rsfp_agree<MS, US>;
    MS near_max = std::numeric_limits<MS>::max();
    auto converted = Agree::convert_first(near_max);
    REQUIRE(converted == std::numeric_limits<Agree::type>::infinity());
}

TEST_CASE("rsfp_agree: very negative mantissa saturates to neg_infinity on conversion",
          "[rsfp][agree]") {
    // scale1 = 1000; INT32_MIN/1000 ≈ -2.1e6; anything below underflows to -inf.
    using Agree = cdcommons::time::rsfp_agree<MS, US>;
    MS very_negative(std::numeric_limits<std::int32_t>::min() / 2);
    auto converted = Agree::convert_first(very_negative);
    REQUIRE(converted == std::numeric_limits<Agree::type>::neg_infinity());
}
