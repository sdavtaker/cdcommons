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
#include <type_traits>

namespace cdcommons::time {

    // Exact rational number type for DEVS simulation time.
    //
    // Always stored in canonical form: denominator > 0, gcd(|num|, denom) = 1.
    // The positive-infinity sentinel is { numeric_limits<Int>::max(), 1 }.
    // Arithmetic propagates infinity: inf + x = inf, inf - x = inf.
    // inf - inf is undefined (DEVS simulators never compute it).
    //
    // Int must be a signed integral type. Use int64_t for long-running simulations
    // to avoid overflow at large time values.
    template <std::signed_integral Int> class rational {
      public:
        using int_type = Int;

        // Default: zero (0/1)
        constexpr rational() noexcept : num_(0), den_(1) {}

        // Construct from integer n/1
        constexpr explicit rational(Int n) noexcept : num_(n), den_(1) {}

        // Construct n/d in canonical form. Throws std::domain_error if d == 0.
        constexpr rational(Int n, Int d) : num_(n), den_(d) {
            if (den_ == Int(0)) {
                throw std::domain_error("cdcommons::time::rational: zero denominator");
            }
            if (den_ < Int(0)) {
                num_ = -num_;
                den_ = -den_;
            }
            Int g = safe_gcd(num_, den_);
            num_ /= g;
            den_ /= g;
        }

        constexpr Int numerator() const noexcept { return num_; }
        constexpr Int denominator() const noexcept { return den_; }

        constexpr rational operator+(const rational &rhs) const {
            if (is_inf(*this) || is_inf(rhs)) {
                return inf_value();
            }
            return rational(num_ * rhs.den_ + rhs.num_ * den_, den_ * rhs.den_);
        }

        constexpr rational operator-(const rational &rhs) const {
            if (is_inf(*this)) {
                return inf_value();
            }
            return rational(num_ * rhs.den_ - rhs.num_ * den_, den_ * rhs.den_);
        }

        constexpr rational operator*(const rational &rhs) const {
            if (is_inf(*this) || is_inf(rhs)) {
                return inf_value();
            }
            return rational(num_ * rhs.num_, den_ * rhs.den_);
        }

        // Cross-multiply comparison (denominators are always positive after canonicalization)
        constexpr auto operator<=>(const rational &rhs) const noexcept {
            return num_ * rhs.den_ <=> rhs.num_ * den_;
        }

        constexpr bool operator==(const rational &rhs) const noexcept {
            return num_ == rhs.num_ && den_ == rhs.den_;
        }

      private:
        Int num_;
        Int den_;

        // The infinity sentinel: { INT_MAX, 1 }
        static constexpr bool is_inf(const rational &r) noexcept {
            return r.num_ == std::numeric_limits<Int>::max() && r.den_ == Int(1);
        }

        static constexpr rational inf_value() noexcept {
            // Bypass canonicalization: already canonical.
            rational r;
            r.num_ = std::numeric_limits<Int>::max();
            r.den_ = Int(1);
            return r;
        }

        // gcd that handles all signed values including INT_MIN without UB.
        // Uses unsigned arithmetic to compute |a| and |b| safely (C++20 two's complement).
        static constexpr Int safe_gcd(Int a, Int b) noexcept {
            using U = std::make_unsigned_t<Int>;
            U ua = static_cast<U>(a);
            U ub = static_cast<U>(b);
            if (a < 0) {
                ua = U(0) - ua; // unsigned wrap == |a| for two's complement
            }
            if (b < 0) {
                ub = U(0) - ub;
            }
            return static_cast<Int>(std::gcd(ua, ub));
        }
    };

} // namespace cdcommons::time

// Partial specialization of std::numeric_limits for cdcommons::time::rational<Int>.
// This specialization is what makes rational<Int> satisfy cdboost::concepts::Time
// and cadmium::concepts::Time (both require has_infinity == true).
namespace std {
    template <std::signed_integral Int> struct numeric_limits<cdcommons::time::rational<Int>> {
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

        static constexpr cdcommons::time::rational<Int> infinity() noexcept {
            return cdcommons::time::rational<Int>{numeric_limits<Int>::max(), Int(1)};
        }
        static constexpr cdcommons::time::rational<Int> max() noexcept {
            return cdcommons::time::rational<Int>{numeric_limits<Int>::max() - Int(1), Int(1)};
        }
        static constexpr cdcommons::time::rational<Int> min() noexcept {
            return cdcommons::time::rational<Int>{Int(1), numeric_limits<Int>::max()};
        }
        static constexpr cdcommons::time::rational<Int> lowest() noexcept {
            return cdcommons::time::rational<Int>{numeric_limits<Int>::min() + Int(1), Int(1)};
        }
        static constexpr cdcommons::time::rational<Int> epsilon() noexcept {
            return cdcommons::time::rational<Int>{Int(1), numeric_limits<Int>::max()};
        }
        static constexpr cdcommons::time::rational<Int> round_error() noexcept {
            return cdcommons::time::rational<Int>{};
        }
        static constexpr cdcommons::time::rational<Int> quiet_NaN() noexcept {
            return cdcommons::time::rational<Int>{};
        }
        static constexpr cdcommons::time::rational<Int> signaling_NaN() noexcept {
            return cdcommons::time::rational<Int>{};
        }
        static constexpr cdcommons::time::rational<Int> denorm_min() noexcept {
            return cdcommons::time::rational<Int>{Int(1), numeric_limits<Int>::max()};
        }
    };
} // namespace std
