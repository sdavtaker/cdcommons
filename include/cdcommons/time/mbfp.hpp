// SPDX-License-Identifier: BSD-2-Clause
/**
 * Copyright (c) 2026-present, Damian Vicino
 * Carleton University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <compare>
#include <concepts>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace cdcommons::time {

    namespace detail {
        template <std::signed_integral Int> constexpr Int ipow(Int base, int exp) {
            Int r = Int(1);
            for (int i = 0; i < exp; ++i) {
                if (base > Int(1) && r > std::numeric_limits<Int>::max() / base)
                    throw std::overflow_error(
                        "cdcommons::time::mbfp: scale factor overflow — use a wider Int type");
                r *= base;
            }
            return r;
        }
    } // namespace detail

    // Multi-Base Floating Point DEVS time type (VWT19, Section 3.2).
    //
    // Represents values of the form  mantissa × Base^Exp.
    // Base and Exp are compile-time constants ("static" agreement strategy).
    // Two values of the same type share the same scale-factor and can be
    // directly compared and added without conversion.
    //
    // Sentinels: +∞ = std::numeric_limits<Int>::max()
    //            −∞ = std::numeric_limits<Int>::min()
    // Both sentinels propagate through arithmetic.  Addition saturates to +∞ on
    // positive overflow.  inf ± inf with opposing signs throws std::domain_error.
    //
    // To combine values from two atomic models with different scale-factors,
    // use mbfp_agree<A, B>. Only non-positive exponents are supported for
    // exact integer conversion (the typical case for DES time fractions).
    template <int Base, int Exp, std::signed_integral Int = long> class mbfp {
        static_assert(Base >= 2, "mbfp: base must be >= 2");

      public:
        using int_type = Int;
        static constexpr int base = Base;
        static constexpr int exponent = Exp;

        constexpr mbfp() noexcept : mantissa_(Int(0)) {}
        constexpr explicit mbfp(Int m) noexcept : mantissa_(m) {}

        constexpr Int mantissa() const noexcept { return mantissa_; }

        constexpr mbfp operator+(const mbfp &rhs) const {
            if (is_pos_inf(*this) && is_neg_inf(rhs))
                throw std::domain_error("mbfp: +inf + (-inf) is undefined");
            if (is_neg_inf(*this) && is_pos_inf(rhs))
                throw std::domain_error("mbfp: -inf + (+inf) is undefined");
            if (is_pos_inf(*this) || is_pos_inf(rhs))
                return pos_inf_value();
            if (is_neg_inf(*this) || is_neg_inf(rhs))
                return neg_inf_value();
            // Positive overflow saturates to +∞.
            if (mantissa_ > Int(0) && rhs.mantissa_ > std::numeric_limits<Int>::max() - mantissa_)
                return pos_inf_value();
            return mbfp(mantissa_ + rhs.mantissa_);
        }

        constexpr mbfp operator-(const mbfp &rhs) const {
            if (is_pos_inf(*this) && is_pos_inf(rhs))
                throw std::domain_error("mbfp: +inf - (+inf) is undefined");
            if (is_neg_inf(*this) && is_neg_inf(rhs))
                throw std::domain_error("mbfp: -inf - (-inf) is undefined");
            if (is_pos_inf(*this))
                return pos_inf_value();
            if (is_neg_inf(*this))
                return neg_inf_value();
            if (is_neg_inf(rhs))
                return pos_inf_value();
            if (is_pos_inf(rhs))
                return neg_inf_value();
            return mbfp(mantissa_ - rhs.mantissa_);
        }

        // Base >= 2 guarantees Base^Exp > 0, so value order == mantissa order.
        constexpr auto operator<=>(const mbfp &rhs) const noexcept {
            return mantissa_ <=> rhs.mantissa_;
        }

        constexpr bool operator==(const mbfp &rhs) const noexcept {
            return mantissa_ == rhs.mantissa_;
        }

      private:
        Int mantissa_;

        static constexpr bool is_pos_inf(const mbfp &v) noexcept {
            return v.mantissa_ == std::numeric_limits<Int>::max();
        }
        static constexpr bool is_neg_inf(const mbfp &v) noexcept {
            return v.mantissa_ == std::numeric_limits<Int>::min();
        }

        static constexpr mbfp pos_inf_value() noexcept {
            mbfp r;
            r.mantissa_ = std::numeric_limits<Int>::max();
            return r;
        }
        static constexpr mbfp neg_inf_value() noexcept {
            mbfp r;
            r.mantissa_ = std::numeric_limits<Int>::min();
            return r;
        }

        template <typename A, typename B> friend struct mbfp_agree;
    };

    // Agreement trait for coupling atomic models with different MBFP scale-factors.
    //
    // common_base = lcm(Base1, Base2)
    // common_exp  = min(Exp1, Exp2)
    //
    // Scale-factor conversion (VWT19, Section 3.4):
    //   new_mantissa = old_mantissa × (common_base / old_base)^|old_exp|
    //                               × common_base^(|common_exp| − |old_exp|)
    // Valid when Exp1 ≤ 0 and Exp2 ≤ 0.
    template <typename A, typename B> struct mbfp_agree;

    template <int B1, int E1, int B2, int E2, std::signed_integral Int>
    struct mbfp_agree<mbfp<B1, E1, Int>, mbfp<B2, E2, Int>> {
        static_assert(E1 <= 0 && E2 <= 0,
                      "mbfp_agree: both exponents must be <= 0 for exact conversion");

        static constexpr int common_base = std::lcm(B1, B2);
        static constexpr int common_exp = (E1 < E2) ? E1 : E2;
        using type = mbfp<common_base, common_exp, Int>;

      private:
        // Declared before scale1/scale2 so the initializers find it (GCC requires this).
        // Computes: (common_base/B)^|E| × common_base^(|common_exp|−|E|)
        // Throws std::overflow_error if the result exceeds Int — surfaced at compile time
        // because scale1/scale2 below are static constexpr.
        static constexpr Int scale_factor(int B, int E) {
            const int abs_e = -E;
            const int abs_ce = -common_exp;
            const Int k = Int(common_base) / Int(B);
            const Int a = detail::ipow(k, abs_e);
            const Int b = detail::ipow(Int(common_base), abs_ce - abs_e);
            if (a > Int(1) && b > std::numeric_limits<Int>::max() / a)
                throw std::overflow_error(
                    "cdcommons::time::mbfp: scale factor overflow — use a wider Int type");
            return a * b;
        }

      public:
        // Evaluated at compile time. If the scale factor overflows Int, the
        // instantiation fails here — not mid-simulation.
        static constexpr Int scale1 = scale_factor(B1, E1);
        static constexpr Int scale2 = scale_factor(B2, E2);

        static constexpr type convert_first(mbfp<B1, E1, Int> v) noexcept {
            if (v.mantissa_ == std::numeric_limits<Int>::max())
                return type(std::numeric_limits<Int>::max());
            if (v.mantissa_ == std::numeric_limits<Int>::min())
                return type(std::numeric_limits<Int>::min());
            if (scale1 > Int(1) && v.mantissa_ > std::numeric_limits<Int>::max() / scale1)
                return type(std::numeric_limits<Int>::max());
            if (scale1 > Int(1) && v.mantissa_ < std::numeric_limits<Int>::min() / scale1)
                return type(std::numeric_limits<Int>::min());
            return type(v.mantissa_ * scale1);
        }

        static constexpr type convert_second(mbfp<B2, E2, Int> v) noexcept {
            if (v.mantissa_ == std::numeric_limits<Int>::max())
                return type(std::numeric_limits<Int>::max());
            if (v.mantissa_ == std::numeric_limits<Int>::min())
                return type(std::numeric_limits<Int>::min());
            if (scale2 > Int(1) && v.mantissa_ > std::numeric_limits<Int>::max() / scale2)
                return type(std::numeric_limits<Int>::max());
            if (scale2 > Int(1) && v.mantissa_ < std::numeric_limits<Int>::min() / scale2)
                return type(std::numeric_limits<Int>::min());
            return type(v.mantissa_ * scale2);
        }
    };

} // namespace cdcommons::time

// Partial specialization of std::numeric_limits for cdcommons::time::mbfp<Base,Exp,Int>.
namespace std {
    template <int Base, int Exp, std::signed_integral Int>
    struct numeric_limits<cdcommons::time::mbfp<Base, Exp, Int>> {
        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = true;
        static constexpr bool is_integer = false;
        static constexpr bool is_exact = true;
        static constexpr bool has_infinity = true;
        static constexpr bool has_quiet_NaN = false;
        static constexpr bool has_signaling_NaN = false;
        static constexpr bool is_bounded = true;
        static constexpr bool is_modulo = false;
        static constexpr bool is_iec559 = false;
        static constexpr bool traps = false;
        static constexpr bool tinyness_before = false;
        static constexpr int radix = Base;
        static constexpr int digits = 0;
        static constexpr int digits10 = 0;
        static constexpr int max_digits10 = 0;
        static constexpr int min_exponent = Exp;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = Exp;
        static constexpr int max_exponent10 = 0;

        using T = cdcommons::time::mbfp<Base, Exp, Int>;

        static constexpr T infinity() noexcept { return T(numeric_limits<Int>::max()); }
        static constexpr T neg_infinity() noexcept { return T(numeric_limits<Int>::min()); }
        static constexpr T max() noexcept { return T(numeric_limits<Int>::max() - Int(1)); }
        static constexpr T min() noexcept { return T(Int(1)); }
        static constexpr T lowest() noexcept { return T(numeric_limits<Int>::min() + Int(1)); }
        static constexpr T epsilon() noexcept { return T(Int(1)); }
        static constexpr T round_error() noexcept { return T{}; }
        static constexpr T quiet_NaN() noexcept { return T{}; }
        static constexpr T signaling_NaN() noexcept { return T{}; }
        static constexpr T denorm_min() noexcept { return T(Int(1)); }
    };
} // namespace std
