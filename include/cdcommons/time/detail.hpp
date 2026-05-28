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

#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace cdcommons::time::detail {

    // Integer power: base^exp (exp >= 0).  Throws on overflow — which surfaces as a
    // compile-time failure when called from a static constexpr initializer.
    template <std::signed_integral Int> constexpr Int ipow(Int base, int exp) {
        Int r = Int(1);
        for (int i = 0; i < exp; ++i) {
            if (base > Int(1) && r > std::numeric_limits<Int>::max() / base)
                throw std::overflow_error(
                    "cdcommons::time: scale factor overflow — use a wider Int type");
            r *= base;
        }
        return r;
    }

    // Maps a signed integer type to the next-wider exact-width signed type.
    // The decision chain is sizeof-based so it is portable across platforms where
    // int/long/long long have varying widths (e.g. 32-bit long on Windows).
    //
    // Valid range: int8_t → int16_t → int32_t → int64_t.
    // Int must be at most 32 bits wide; int64_t is used as the common intermediate
    // type so that all scale-factor arithmetic stays within the standard library.
    // Use int32_t (or a narrower type) as the mantissa Int parameter.
    template <std::signed_integral Int> struct next_wider_int {
        using type = std::conditional_t<
            (sizeof(Int) < sizeof(std::int16_t)), std::int16_t,
            std::conditional_t<
                (sizeof(Int) < sizeof(std::int32_t)), std::int32_t,
                std::conditional_t<(sizeof(Int) < sizeof(std::int64_t)), std::int64_t, void>>>;
        static_assert(!std::is_same_v<type, void>,
                      "agree: Int must be at most 32 bits wide (e.g. int32_t); "
                      "int64_t is reserved as the intermediate type for scale-factor "
                      "arithmetic — using int64_t as Int leaves no room for a safe "
                      "wider intermediate");
    };

    template <std::signed_integral Int> using next_wider_int_t = typename next_wider_int<Int>::type;

} // namespace cdcommons::time::detail
