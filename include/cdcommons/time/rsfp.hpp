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

namespace cdcommons::time {

    // Rational-Scaled Floating Point DEVS time type (VWT19, Section 3.1).
    //
    // Represents values of the form  mantissa × (Num/Den).
    // Num and Den are compile-time integer constants ("static" agreement strategy);
    // mantissa is a signed integer.  This lets values like 0.1 be stored exactly:
    // rsfp<1,10> with mantissa=1, no floating-point drift.
    //
    // The "Floating Point" in the name refers to the scalable exponent mechanism
    // from the VWT19 paper (magnitude × (quantum_n/quantum_d) × 2^exponent); this
    // implementation folds quantum and exponent into the compile-time Num/Den ratio.
    //
    // Infinity sentinel: { std::numeric_limits<Int>::max() }.
    // Addition saturates to the sentinel on overflow.
    //
    // For coupling models with different scale factors use rsfp_agree<A, B>.
    template <int Num, int Den, std::signed_integral Int = long> class rsfp {
        static_assert(Num > 0, "rsfp: Num must be positive");
        static_assert(Den > 0, "rsfp: Den must be positive");

      public:
        using int_type = Int;
        static constexpr int num = Num;
        static constexpr int den = Den;

        constexpr rsfp() noexcept : mantissa_(Int(0)) {}
        constexpr explicit rsfp(Int m) noexcept : mantissa_(m) {}

        constexpr Int mantissa() const noexcept { return mantissa_; }

        constexpr rsfp operator+(const rsfp &rhs) const noexcept {
            if (is_inf(*this) || is_inf(rhs))
                return inf_value();
            // Overflow on non-negative time addition saturates to infinity.
            if (mantissa_ > Int(0) && rhs.mantissa_ > std::numeric_limits<Int>::max() - mantissa_)
                return inf_value();
            return rsfp(mantissa_ + rhs.mantissa_);
        }

        constexpr rsfp operator-(const rsfp &rhs) const noexcept {
            if (is_inf(*this))
                return inf_value();
            return rsfp(mantissa_ - rhs.mantissa_);
        }

        // Num/Den > 0, so value order == mantissa order.
        constexpr auto operator<=>(const rsfp &rhs) const noexcept {
            return mantissa_ <=> rhs.mantissa_;
        }

        constexpr bool operator==(const rsfp &rhs) const noexcept {
            return mantissa_ == rhs.mantissa_;
        }

      private:
        Int mantissa_;

        static constexpr bool is_inf(const rsfp &v) noexcept {
            return v.mantissa_ == std::numeric_limits<Int>::max();
        }

        static constexpr rsfp inf_value() noexcept {
            rsfp r;
            r.mantissa_ = std::numeric_limits<Int>::max();
            return r;
        }

        template <typename A, typename B> friend struct rsfp_agree;
    };

    // Agreement trait for coupling atomic models with different RSFP scale factors.
    //
    // common_num = gcd(Num1, Num2)
    // common_den = lcm(Den1, Den2)
    //
    // This is the smallest rational scale s such that both (N1/D1) and (N2/D2)
    // are integer multiples of s (VWT19, Section 3.4).
    //
    // Scale factors computed in long to avoid intermediate overflow for large
    // denominators (e.g. nanoseconds: Den = 1 000 000 000).  A static_assert
    // fires at instantiation if common_den overflows int.
    template <typename A, typename B> struct rsfp_agree;

    template <int N1, int D1, int N2, int D2, std::signed_integral Int>
    struct rsfp_agree<rsfp<N1, D1, Int>, rsfp<N2, D2, Int>> {
      private:
        // Declared before scale1/scale2 so the initializers find it (GCC requires this).
        static constexpr long cn_ = std::gcd(static_cast<long>(N1), static_cast<long>(N2));
        static constexpr long cd_ = std::lcm(static_cast<long>(D1), static_cast<long>(D2));

        static_assert(cn_ <= static_cast<long>(std::numeric_limits<int>::max()),
                      "rsfp_agree: common_num overflows int — reduce Num values");
        static_assert(cd_ <= static_cast<long>(std::numeric_limits<int>::max()),
                      "rsfp_agree: common_den overflows int — use a smaller Den or wider types");

        // scale for rsfp<N,D> → rsfp<common_num, common_den>:
        //   (N/D) / (common_num/common_den) = (N * common_den) / (D * common_num)
        // Always an exact integer when common_num = gcd(N1,N2), common_den = lcm(D1,D2).
        static constexpr long scale_factor(long N, long D) noexcept {
            return (N * cd_) / (D * cn_);
        }

      public:
        static constexpr int common_num = static_cast<int>(cn_);
        static constexpr int common_den = static_cast<int>(cd_);
        using type = rsfp<common_num, common_den, Int>;

        // Evaluated at compile time — wrong scale factor is a build error, not a runtime surprise.
        static constexpr long scale1 = scale_factor(N1, D1);
        static constexpr long scale2 = scale_factor(N2, D2);

        static_assert(scale1 <= static_cast<long>(std::numeric_limits<Int>::max()),
                      "rsfp_agree: scale1 overflows Int — use a wider Int type");
        static_assert(scale2 <= static_cast<long>(std::numeric_limits<Int>::max()),
                      "rsfp_agree: scale2 overflows Int — use a wider Int type");

        static constexpr type convert_first(rsfp<N1, D1, Int> v) noexcept {
            if (v.mantissa_ == std::numeric_limits<Int>::max())
                return type(std::numeric_limits<Int>::max());
            constexpr Int s = static_cast<Int>(scale1);
            if (s > Int(1) && v.mantissa_ > std::numeric_limits<Int>::max() / s)
                return type(std::numeric_limits<Int>::max());
            if (s > Int(1) && v.mantissa_ < std::numeric_limits<Int>::min() / s)
                return type(std::numeric_limits<Int>::min() + Int(1));
            return type(v.mantissa_ * s);
        }

        static constexpr type convert_second(rsfp<N2, D2, Int> v) noexcept {
            if (v.mantissa_ == std::numeric_limits<Int>::max())
                return type(std::numeric_limits<Int>::max());
            constexpr Int s = static_cast<Int>(scale2);
            if (s > Int(1) && v.mantissa_ > std::numeric_limits<Int>::max() / s)
                return type(std::numeric_limits<Int>::max());
            if (s > Int(1) && v.mantissa_ < std::numeric_limits<Int>::min() / s)
                return type(std::numeric_limits<Int>::min() + Int(1));
            return type(v.mantissa_ * s);
        }
    };

} // namespace cdcommons::time

// Partial specialization of std::numeric_limits for cdcommons::time::rsfp<Num,Den,Int>.
namespace std {
    template <int Num, int Den, std::signed_integral Int>
    struct numeric_limits<cdcommons::time::rsfp<Num, Den, Int>> {
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
        static constexpr int radix = 2;
        static constexpr int digits = 0;
        static constexpr int digits10 = 0;
        static constexpr int max_digits10 = 0;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;

        using T = cdcommons::time::rsfp<Num, Den, Int>;

        static constexpr T infinity() noexcept { return T(numeric_limits<Int>::max()); }
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
