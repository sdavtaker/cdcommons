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
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace cdcommons::time {

    // Fixed-point decimal DEVS time type.
    //
    // Represents values of the form  raw / 10^Scale  where raw is a signed integer.
    // Scale is a compile-time constant; resolution is 10^-Scale seconds.
    // Example: decimal<3> stores milliseconds as raw integers (raw=1 → 0.001 s).
    //
    // Sentinels: +∞ = std::numeric_limits<Raw>::max()
    //            −∞ = std::numeric_limits<Raw>::min()
    // Both sentinels propagate through arithmetic. Addition saturates to +∞ on
    // positive overflow. inf ± inf with opposing signs throws std::domain_error.
    template <unsigned int Scale, std::signed_integral Raw = std::int32_t> struct decimal {
        using raw_type = Raw;
        static constexpr unsigned int scale = Scale;

        constexpr decimal() = default;

        static constexpr decimal from_scaled(Raw raw) noexcept { return decimal(raw); }

        static constexpr decimal from_whole(Raw whole) noexcept {
            Raw acc = whole;
            for (unsigned int i = 0; i < Scale; ++i) {
                if (acc > (std::numeric_limits<Raw>::max() - Raw(1)) / Raw(10))
                    return pos_inf_value();
                if (acc < (std::numeric_limits<Raw>::min() + Raw(1)) / Raw(10))
                    return neg_inf_value();
                acc = static_cast<Raw>(acc * Raw(10));
            }
            return decimal(acc);
        }

        constexpr Raw raw_value() const noexcept { return raw_; }

        friend constexpr decimal operator+(decimal a, decimal b) {
            if (is_pos_inf(a) && is_neg_inf(b))
                throw std::domain_error("decimal: +inf + (-inf) is undefined");
            if (is_neg_inf(a) && is_pos_inf(b))
                throw std::domain_error("decimal: -inf + (+inf) is undefined");
            if (is_pos_inf(a) || is_pos_inf(b))
                return pos_inf_value();
            if (is_neg_inf(a) || is_neg_inf(b))
                return neg_inf_value();
            if (a.raw_ > Raw(0) && b.raw_ > std::numeric_limits<Raw>::max() - a.raw_)
                return pos_inf_value();
            if (a.raw_ < Raw(0) && b.raw_ < std::numeric_limits<Raw>::min() - a.raw_)
                return neg_inf_value();
            return decimal(static_cast<Raw>(a.raw_ + b.raw_));
        }

        friend constexpr decimal operator-(decimal a, decimal b) {
            if (is_pos_inf(a) && is_pos_inf(b))
                throw std::domain_error("decimal: +inf - (+inf) is undefined");
            if (is_neg_inf(a) && is_neg_inf(b))
                throw std::domain_error("decimal: -inf - (-inf) is undefined");
            if (is_pos_inf(a))
                return pos_inf_value();
            if (is_neg_inf(a))
                return neg_inf_value();
            if (is_neg_inf(b))
                return pos_inf_value();
            if (is_pos_inf(b))
                return neg_inf_value();
            // Subtracting a negative can overflow positive; subtracting a positive can overflow
            // negative.
            if (b.raw_ < Raw(0) && a.raw_ > std::numeric_limits<Raw>::max() + b.raw_)
                return pos_inf_value();
            if (b.raw_ > Raw(0) && a.raw_ < std::numeric_limits<Raw>::min() + b.raw_)
                return neg_inf_value();
            return decimal(static_cast<Raw>(a.raw_ - b.raw_));
        }

        friend constexpr auto operator<=>(decimal a, decimal b) noexcept = default;
        friend constexpr bool operator==(decimal a, decimal b) noexcept = default;

      private:
        Raw raw_{0};
        explicit constexpr decimal(Raw raw) noexcept : raw_(raw) {}

        static constexpr bool is_pos_inf(decimal v) noexcept {
            return v.raw_ == std::numeric_limits<Raw>::max();
        }
        static constexpr bool is_neg_inf(decimal v) noexcept {
            return v.raw_ == std::numeric_limits<Raw>::min();
        }
        static constexpr decimal pos_inf_value() noexcept {
            return decimal(std::numeric_limits<Raw>::max());
        }
        static constexpr decimal neg_inf_value() noexcept {
            return decimal(std::numeric_limits<Raw>::min());
        }
    };

} // namespace cdcommons::time

// Partial specialization of std::numeric_limits for cdcommons::time::decimal<Scale,Raw>.
namespace std {
    template <unsigned int Scale, std::signed_integral Raw>
    struct numeric_limits<cdcommons::time::decimal<Scale, Raw>> {
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
        static constexpr int radix = 10;
        static constexpr int digits = 0;
        static constexpr int digits10 = 0;
        static constexpr int max_digits10 = 0;
        static constexpr int min_exponent = 0;
        static constexpr int min_exponent10 = 0;
        static constexpr int max_exponent = 0;
        static constexpr int max_exponent10 = 0;

        using T = cdcommons::time::decimal<Scale, Raw>;

        static constexpr T infinity() noexcept {
            return T::from_scaled(numeric_limits<Raw>::max());
        }
        static constexpr T neg_infinity() noexcept {
            return T::from_scaled(numeric_limits<Raw>::min());
        }
        static constexpr T max() noexcept {
            return T::from_scaled(numeric_limits<Raw>::max() - Raw(1));
        }
        static constexpr T min() noexcept { return T::from_scaled(Raw(1)); }
        static constexpr T lowest() noexcept {
            return T::from_scaled(numeric_limits<Raw>::min() + Raw(1));
        }
        static constexpr T epsilon() noexcept { return T::from_scaled(Raw(1)); }
        static constexpr T round_error() noexcept { return T{}; }
        static constexpr T quiet_NaN() noexcept { return T{}; }
        static constexpr T signaling_NaN() noexcept { return T{}; }
        static constexpr T denorm_min() noexcept { return T::from_scaled(Raw(1)); }
    };
} // namespace std
