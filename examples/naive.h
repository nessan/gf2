/// Some alternative/naive implementations of various methods that typically work bit-by-bit.
/// **Note:**  Used to check and benchmark the faster methods that are implemented in the library itself.
///
/// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include <gf2/gf2.h>

namespace naive {

/// Returns the index of the first set bit in the bit-vector or `{}` if no bits are set.
template<Unsigned Word>
std::optional<usize>
first_set(const gf2::BitVector<Word>& bv) {
    if (bv.is_empty()) return {};
    for (auto i = 0uz; i < bv.size(); ++i)
        if (bv[i]) return i;
    return {};
}

/// Returns the index of the first unset bit in the bit-vector or `{}` if no bits are set.
template<Unsigned Word>
std::optional<usize>
first_unset(const gf2::BitVector<Word>& bv) {
    if (bv.is_empty()) return {};
    for (auto i = 0uz; i < bv.size(); ++i)
        if (!bv[i]) return i;
    return {};
}

/// Returns the index of the last set bit in the bit-vector or `{}` if no bits are set.
template<Unsigned Word>
std::optional<usize>
last_set(const gf2::BitVector<Word>& bv) {
    if (bv.is_empty()) return {};
    for (auto i = bv.size(); i--;)
        if (bv[i]) return i;
    return {};
}

/// Returns the index of the last unset bit in the bit-vector or `{}` if no bits are set.
template<Unsigned Word>
std::optional<usize>
last_unset(const gf2::BitVector<Word>& bv) {
    if (bv.is_empty()) return {};
    for (auto i = bv.size(); i--;)
        if (!bv[i]) return i;
    return {};
}

/// Returns the index of the next set bit in the bit-vector after `index` or `{}` if no bits are set.
template<Unsigned Word>
std::optional<usize>
next_set(const gf2::BitVector<Word>& bv, usize index) {
    if (bv.is_empty()) return {};
    for (auto i = index + 1; i < bv.size(); ++i)
        if (bv[i]) return i;
    return {};
}
/// Returns the index of the next unset bit in the bit-vector after `index` or `{}` if no bits are set.
template<Unsigned Word>
std::optional<usize>
next_unset(const gf2::BitVector<Word>& bv, usize index) {
    if (bv.is_empty()) return {};
    for (auto i = index + 1; i < bv.size(); ++i)
        if (!bv[i]) return i;
    return {};
}

/// Returns the index of the previous set bit in the bit-vector before `index` or `{}` if no bits are set.
template<Unsigned Word>
std::optional<usize>
previous_set(const gf2::BitVector<Word>& bv, usize index) {
    if (bv.is_empty() || index == 0) return {};
    for (auto i = index; i--;)
        if (bv[i]) return i;
    return {};
}

/// Returns the index of the previous unset bit in the bit-vector before `index` or `{}` if no bits are set.
template<Unsigned Word>
std::optional<usize>
previous_unset(const gf2::BitVector<Word>& bv, usize index) {
    if (bv.is_empty() || index == 0) return {};
    for (auto i = index; i--;)
        if (!bv[i]) return i;
    return {};
}

/// Returns a new bit-vector that is a right-shifted version of the input bit-vector.
template<Unsigned Word>
gf2::BitVector<Word>
shift_right(const gf2::BitVector<Word>& bv, usize shift) {
    // Edge case?
    if (bv.size() == 0 || shift == 0) return bv;

    // Set up the result as a zero bit-vector of the correct length.
    auto result = gf2::BitVector<Word>::zeros(bv.size());

    // Perhaps we have shifted the whole bit-vector out so the result is all zeros.
    if (shift >= bv.size()) return result;

    // Shift the bit-vector by the given amount.
    for (auto i = 0uz; i < bv.size() - shift; ++i) {
        if (bv[i]) result.set(i + shift);
    }
    return result;
}

/// Returns a new bit-vector that is a left-shifted version of the input bit-vector.
template<Unsigned Word>
gf2::BitVector<Word>
shift_left(const gf2::BitVector<Word>& bv, usize shift) {
    // Edge case?
    if (bv.size() == 0 || shift == 0) return bv;

    // Set up the result as a zero bit-vector of the correct length.
    auto result = gf2::BitVector<Word>::zeros(bv.size());

    // Perhaps we have shifted the whole bit-vector out so the result is all zeros.
    if (shift >= bv.size()) return result;

    // Shift the bit-vector by the given amount.
    for (auto i = shift; i < bv.size(); ++i) {
        if (bv[i]) result.set(i - shift);
    }
    return result;
}

/// Returns a "binary" string representation of a bit-vector.
template<Unsigned Word>
std::string
to_binary_string(const gf2::BitVector<Word>& bv) {
    if (bv.is_empty()) return std::string{};
    std::string result;
    result.reserve(bv.size());
    for (auto i = 0uz; i < bv.size(); ++i) {
        char c = bv[i] ? '1' : '0';
        result.push_back(c);
    }
    return result;
}

/// Returns a "hex" string representation of a bit-vector.
template<Unsigned Word>
std::string
to_hex_string(const gf2::BitVector<Word>& bv) {
    if (bv.is_empty()) return std::string{};

    // The number of digits in the output string. Generally hexadecimal but the last may be to a lower base.
    auto digits = (bv.size() + 3) / 4;

    // Preallocate space allowing for a possible two character suffix of the form ".b" where `b` is one of 2, 4 or 8.
    std::string result;
    result.reserve(digits + 2);

    // Iterate through the bit-vector in blocks of 4 elements and convert each block to a hex digit
    auto n_hex_digits = bv.size() / 4;
    for (auto i = 0uz; i < n_hex_digits; ++i) {
        auto index = 4 * i;
        auto num = 0;
        if (bv[index]) num += 8;
        if (bv[index + 1]) num += 4;
        if (bv[index + 2]) num += 2;
        if (bv[index + 3]) num += 1;
        auto num_str = std::format("{:X}", num);
        result.append(num_str);
    }

    auto k = bv.size() % 4;
    if (k != 0) {
        // The bit-vector has a size that is not a multiple of 4 so the last hex digit is encoded to a lower base --
        // 2, 4 or 8.
        auto num = 0;
        for (auto i = 0uz; i < k; ++i) {
            if (bv[bv.size() - 1 - i]) num |= 1 << i;
        }
        auto num_str = std::format("{:X}", num);
        result.append(num_str);

        // Append the appropriate base to the output string so that the last digit can be interpreted properly.
        result.append(std::format(".{}", 1 << k));
    }
    return result;
}

/// Returns the convolution of two bit-vectors computed in the simplest element-by-element manner.
template<Unsigned Word>
auto
convolve(const gf2::BitVector<Word>& a, const gf2::BitVector<Word>& b) {
    usize na = a.size();
    usize nb = b.size();

    // Edge case?
    if (na == 0 || nb == 0) return gf2::BitVector<Word>();

    // Space for the non-singular return value.
    gf2::BitVector<Word> retval(na + nb - 1);

    // Run through all the elements
    for (usize i = 0; i < na; ++i) {
        bool ai = a[i];
        if (ai) {
            for (usize j = 0; j < nb; ++j) retval[i + j] = retval[i + j] ^ (ai & b[j]);
        }
    }
    return retval;
}

/// Computes the polynomial r(x) := x^n mod p(x) for exponent n, where p(x) is a bit-polynomial.
/// This uses the simplest (and slowest) iterative approach.
template<Unsigned Word>
gf2::BitPolynomial<Word>
reduce_x_to_the(usize n, const gf2::BitPolynomial<Word>& P) {
    // Make a copy and drop any high order zero coefficients.
    auto poly = P;
    poly.make_monic();

    // Edge case: the zero polynomial.
    if (poly.is_zero()) return gf2::BitPolynomial<Word>::zero();

    // Edge case: the constant polynomial P(x) := 1. Anything mod 1 is 0.
    if (poly.is_one()) return gf2::BitPolynomial<Word>::zero();

    // Edge case: x^0 = 1 so x^n mod P(x) = 1 for any polynomial P(x) != 1 (we already handled the `P(x) = 1` case).
    if (n == 0) return gf2::BitPolynomial<Word>::one();

    // The polynomial P(x) is non-zero and can be written as P(x) = x^d + p(x) where degree[p] < d.
    auto d = poly.degree();

    // Edge case: P(x) = x + c where c is a constant => x = P(x) + c.
    // Then for any exponent e: x^e = (P(x) + c)^e = terms in powers of P(x) + c^e => x^e mod P(x) = c^e = c.
    if (d == 1) return gf2::BitPolynomial<Word>::constant(poly[0]);

    // We can write poly(x) = p_0 + p_1 x + ... + p_{d-1} x^{d-1}. All that matters are those coefficients.
    auto p = poly.coefficients().sub(0, d);

    // Small powers n < d:
    if (n < d) return gf2::BitPolynomial<Word>::x_to_the(n);

    // Matching power n == d: Remainder is the polynomial itself.
    if (n == d) return gf2::BitPolynomial<Word>{p};

    // Larger powers: Use an iteration which writes x*r(x) mod p(x) in terms of r(x) mod p(x).
    auto r = p;
    for (auto i = d; i < n; ++i) r = r[d - 1] ? p ^ (r >> 1) : (r >> 1);

    // Done
    return gf2::BitPolynomial<Word>{r};
}

} // namespace naive