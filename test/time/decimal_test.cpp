// SPDX-License-Identifier: BSD-2-Clause
#include <catch2/catch_test_macros.hpp>
#include <cdcommons/time/decimal.hpp>
#include <limits>
#include <stdexcept>

// 1 ms resolution: raw=1 → 1 × 10^-3 s
using MS3 = cdcommons::time::decimal<-3>;
// 1 µs resolution: raw=1 → 1 × 10^-6 s
using US6 = cdcommons::time::decimal<-6>;

static const MS3 ms_inf = std::numeric_limits<MS3>::infinity();
static const MS3 ms_neg_inf = std::numeric_limits<MS3>::neg_infinity();

TEST_CASE("decimal: default construction is zero", "[decimal]") {
    MS3 t;
    REQUIRE(t.raw_value() == 0);
}

TEST_CASE("decimal: from_scaled construction", "[decimal]") {
    MS3 t = MS3::from_scaled(5);
    REQUIRE(t.raw_value() == 5);
    REQUIRE(t == MS3::from_scaled(5));
}

TEST_CASE("decimal: from_whole construction", "[decimal]") {
    // from_whole(1) with Scale=3 → raw = 1000
    MS3 t = MS3::from_whole(1);
    REQUIRE(t.raw_value() == 1000);
    MS3 t2 = MS3::from_whole(2);
    REQUIRE(t2.raw_value() == 2000);
}

TEST_CASE("decimal: static type parameters", "[decimal]") {
    REQUIRE(MS3::exponent == -3);
    REQUIRE(US6::exponent == -6);
}

TEST_CASE("decimal: addition", "[decimal]") {
    REQUIRE(MS3::from_scaled(3) + MS3::from_scaled(4) == MS3::from_scaled(7));
    REQUIRE(MS3::from_scaled(0) + MS3::from_scaled(5) == MS3::from_scaled(5));
    REQUIRE(MS3::from_scaled(100) + MS3::from_scaled(900) == MS3::from_scaled(1000));
}

TEST_CASE("decimal: subtraction", "[decimal]") {
    REQUIRE(MS3::from_scaled(7) - MS3::from_scaled(3) == MS3::from_scaled(4));
    REQUIRE(MS3::from_scaled(5) - MS3::from_scaled(5) == MS3::from_scaled(0));
    REQUIRE(MS3::from_scaled(1000) - MS3::from_scaled(1) == MS3::from_scaled(999));
}

TEST_CASE("decimal: comparison (totally ordered)", "[decimal]") {
    REQUIRE(MS3::from_scaled(1) < MS3::from_scaled(2));
    REQUIRE(MS3::from_scaled(2) > MS3::from_scaled(1));
    REQUIRE(MS3::from_scaled(3) <= MS3::from_scaled(3));
    REQUIRE(MS3::from_scaled(3) >= MS3::from_scaled(3));
    REQUIRE(MS3::from_scaled(0) < MS3::from_scaled(1));
    REQUIRE_FALSE(MS3::from_scaled(5) < MS3::from_scaled(5));
}

TEST_CASE("decimal: equality", "[decimal]") {
    REQUIRE(MS3::from_scaled(7) == MS3::from_scaled(7));
    REQUIRE_FALSE(MS3::from_scaled(7) == MS3::from_scaled(8));
}

TEST_CASE("decimal: infinity sentinel from numeric_limits", "[decimal]") {
    REQUIRE(std::numeric_limits<MS3>::has_infinity);
    REQUIRE(ms_inf == std::numeric_limits<MS3>::infinity());
    REQUIRE(ms_inf.raw_value() == std::numeric_limits<std::int32_t>::max());
    REQUIRE(ms_neg_inf == std::numeric_limits<MS3>::neg_infinity());
    REQUIRE(ms_neg_inf.raw_value() == std::numeric_limits<std::int32_t>::min());
}

TEST_CASE("decimal: infinity arithmetic propagation", "[decimal]") {
    MS3 t = MS3::from_scaled(3);
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

TEST_CASE("decimal: undefined infinity arithmetic throws domain_error", "[decimal]") {
    REQUIRE_THROWS_AS(ms_inf + ms_neg_inf, std::domain_error);
    REQUIRE_THROWS_AS(ms_neg_inf + ms_inf, std::domain_error);
    REQUIRE_THROWS_AS(ms_inf - ms_inf, std::domain_error);
    REQUIRE_THROWS_AS(ms_neg_inf - ms_neg_inf, std::domain_error);
}

TEST_CASE("decimal: infinity comparison", "[decimal]") {
    REQUIRE(MS3::from_scaled(999999999) < ms_inf);
    REQUIRE(ms_inf > MS3::from_scaled(0));
    REQUIRE(ms_inf == ms_inf);
}

TEST_CASE("decimal: addition overflow saturates to infinity", "[decimal]") {
    // max() is INT32_MAX - 1; adding 2 exceeds the sentinel → saturates to +inf
    MS3 near_max = std::numeric_limits<MS3>::max();
    REQUIRE((near_max + MS3::from_scaled(2)) == ms_inf);
    REQUIRE((near_max + MS3::from_scaled(1)) == ms_inf);
    REQUIRE((near_max + MS3::from_scaled(0)) == near_max);
}

TEST_CASE("decimal: addition negative overflow saturates to neg_infinity", "[decimal]") {
    MS3 near_lowest = std::numeric_limits<MS3>::lowest();
    // lowest() + (-2) underflows below INT32_MIN → saturate to -inf
    REQUIRE((near_lowest + MS3::from_scaled(-2)) == ms_neg_inf);
    // lowest() + (-1) produces INT32_MIN naturally (the -inf sentinel)
    REQUIRE((near_lowest + MS3::from_scaled(-1)) == ms_neg_inf);
    // lowest() + 0 stays at lowest()
    REQUIRE((near_lowest + MS3::from_scaled(0)) == near_lowest);
}

TEST_CASE("decimal: subtraction overflow saturates", "[decimal]") {
    MS3 near_max = std::numeric_limits<MS3>::max();
    MS3 near_lowest = std::numeric_limits<MS3>::lowest();
    // max() - (-2): subtracting a negative overflows upward → +inf
    REQUIRE((near_max - MS3::from_scaled(-2)) == ms_inf);
    // max() - (-1): produces INT32_MAX naturally (the +inf sentinel)
    REQUIRE((near_max - MS3::from_scaled(-1)) == ms_inf);
    // lowest() - 2: subtracting a positive overflows downward → -inf
    REQUIRE((near_lowest - MS3::from_scaled(2)) == ms_neg_inf);
    // lowest() - 1: produces INT32_MIN naturally (the -inf sentinel)
    REQUIRE((near_lowest - MS3::from_scaled(1)) == ms_neg_inf);
}

TEST_CASE("decimal: from_whole overflow saturates", "[decimal]") {
    // decimal<-30>: 10^30 far exceeds INT32_MAX → saturate to +inf
    using BIG = cdcommons::time::decimal<-30>;
    REQUIRE(BIG::from_whole(1) == std::numeric_limits<BIG>::infinity());
    REQUIRE(BIG::from_whole(-1) == std::numeric_limits<BIG>::neg_infinity());
    // decimal<-3> with reasonable input stays finite
    REQUIRE(MS3::from_whole(1).raw_value() == 1000);
}
