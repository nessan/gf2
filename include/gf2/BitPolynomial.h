#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// Polynomials over GF(2). <br>
/// See the [BitPolynomial](docs/pages/BitPolynomial.md) page for more details.

#include <gf2/BitVector.h>
#include <sstream>

namespace gf2 {

// Forward declarations to avoid recursive inclusion issues when we define polynomial evaluation for bit-matrices.
template<Unsigned Word>
class BitMatrix;

/// A `BitPolynomial` represents a polynomial over GF(2) where we store the polynomial coefficients in a bit-vector.
/// <br> The template parameter `Word` sets the unsigned word type used by the `BitVector` that stores the coefficients.
///
/// # Example
/// ```
/// auto p = BitPolynomial<>::zeros(3);
/// p[0] = true;
/// p[1] = false;
/// p[2] = true;
/// assert_eq(p.to_string(), "1 + x^2");
/// ```
template<Unsigned Word = usize>
class BitPolynomial {
private:
    BitVector<Word> m_coeffs;

public:
    /// The type used to store the bit-polynomial coefficients.
    using coeffs_type = BitVector<Word>;

    /// The underlying unsigned word type used to store the bits.
    using word_type = Word;

    /// @name Constructors:
    /// @{

    /// The default constructor creates an empty bit-polynomial with no coefficients.
    ///
    /// This is one possible form of the zero BitPolynomial.
    ///
    /// # Example
    /// ```
    /// BitPolynomial p;
    /// assert_eq(p.to_string(), "0");
    /// ```
    constexpr BitPolynomial() : m_coeffs{} {}

    /// Constructs a bit-polynomial with the given coefficients by copying them from any bit-store.
    ///
    /// **Note:** The input coefficient bit-store can have any underlying unsigned word type.
    ///
    /// # Example
    /// ```
    /// BitPolynomial p{BitVector<>::ones(10)};
    /// assert_eq(p.to_string(), "1 + x + x^2 + x^3 + x^4 + x^5 + x^6 + x^7 + x^8 + x^9");
    /// ```
    template<BitStore Src>
    constexpr BitPolynomial(Src const& coeffs) : m_coeffs{coeffs.size()} {
        m_coeffs.copy(coeffs);
    }

    /// Constructs a bit-polynomial with the given coefficients by moving them.
    ///
    /// Use `std::move(coeffs)` in the constructor argument to get this version.
    ///
    /// # Note
    /// The input coefficient bit-vector is moved into the bit-polynomial so it is no longer valid after this call.
    ///
    /// # Example
    /// ```
    /// auto coeffs = BitVector<>::ones(10);
    /// BitPolynomial p{std::move(coeffs)};
    /// assert_eq(p.to_string(), "1 + x + x^2 + x^3 + x^4 + x^5 + x^6 + x^7 + x^8 + x^9");
    /// ```
    constexpr BitPolynomial(BitVector<Word>&& coeffs) : m_coeffs{std::move(coeffs)} {}

    /// @}
    /// @name Factory constructors:
    /// @{

    /// Factory method to return the "zero" bit-polynomial p(x) := 0.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::zero();
    /// assert_eq(p.to_string(), "0");
    /// ```
    static constexpr BitPolynomial zero() { return BitPolynomial<Word>{}; }

    /// Factory method to return the "one" bit-polynomial p(x) := 1.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::one();
    /// assert_eq(p.to_string(), "1");
    /// ```
    static constexpr BitPolynomial one() { return BitPolynomial<Word>{std::move(coeffs_type::ones(1))}; }

    /// Factory method to return the constant bit-polynomial p(x) := val where val is a boolean.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::constant(true);
    /// assert_eq(p.to_string(), "1");
    /// ```
    static constexpr BitPolynomial constant(bool val) {
        return BitPolynomial<Word>{std::move(coeffs_type::constant(val, 1))};
    }

    /// Factory method to return a bit-polynomial with `n + 1` coefficients, all initialized to zero.
    ///
    /// This is the BitPolynomial 0*x^n + 0*x^(n-1) + ... + 0*x + 0.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::zeros(3);
    /// assert_eq(p.to_full_string(), "0 + 0x + 0x^2 + 0x^3");
    /// ```
    static constexpr BitPolynomial zeros(usize n) { return BitPolynomial<Word>{std::move(coeffs_type::zeros(n + 1))}; }

    /// Factory method to return a monic bit-polynomial of degree `n` with `n + 1` coefficients, all ones.
    ///
    /// This is the BitPolynomial x^n + x^(n-1) + ... + x + 1.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::ones(4);
    /// assert_eq(p.to_string(), "1 + x + x^2 + x^3 + x^4");
    /// ```
    static constexpr BitPolynomial ones(usize n) { return BitPolynomial<Word>{std::move(coeffs_type::ones(n + 1))}; }

    /// Factory method to return the bit-polynomial p(x) := x^n.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::x_to_the(3);
    /// assert_eq(p.to_string(), "x^3");
    /// ```
    static constexpr BitPolynomial x_to_the(usize n) {
        return BitPolynomial<Word>{std::move(coeffs_type::unit(n + 1, n))};
    }

    /// Factory method to return a new bit-polynomial of *degree* `n` with coefficients set by calling the
    /// function `f` to set each coefficient to either 0 or 1.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::from(10, [](usize i) { return i % 2 == 0; });
    /// assert_eq(p.to_string(), "1 + x^2 + x^4 + x^6 + x^8 + x^10");
    /// ```
    static constexpr BitPolynomial from(usize n, std::invocable<usize> auto f) {
        return BitPolynomial<Word>{std::move(coeffs_type::from(n + 1, f))};
    }

    /// @}
    /// @name Construct polynomials with random coefficients:
    /// @{

    /// Factory method to return a new bit-polynomial of *degree* `n` with `n + 1` coefficients picked uniformly
    /// at random.
    ///
    /// The random coefficients are from independent fair coin flips seeded with entropy.
    ///
    /// **Note:** If `n > 0` then the returned BitPolynomial is monic.
    static constexpr BitPolynomial random(usize n) {
        // BitPolynomial of degree `n` has `n + 1` coefficients.
        auto coeffs = coeffs_type::random(n + 1);

        // If `n > 0` we want the coefficient of `x^n` to be one for sure. If `n == 0` then a random 0/1 is fine.
        if (n > 0) coeffs.set(n);
        return BitPolynomial{std::move(coeffs)};
    }

    /// Factory method to return a new bit-polynomial of *degree* `n` with `n + 1` coefficients picked uniformly
    /// at random. The random coefficients are from independent fair coin flips.
    ///
    /// For reproducibility, the underlying random number generator is seeded with the specified `seed`.
    ///
    /// # Example
    /// ```
    /// std::uint64_t seed = 42;
    /// auto p1 = BitPolynomial<>::seeded_random(3311, seed);
    /// auto p2 = BitPolynomial<>::seeded_random(3311, seed);
    /// assert_eq(p1, p2, "BitPolys with the same seed should be equal");
    /// ```
    static constexpr BitPolynomial seeded_random(usize n, std::uint64_t seed) {
        // BitPolynomial of degree `n` has `n + 1` coefficients.
        auto coeffs = coeffs_type::seeded_random(n + 1, seed);

        // If `n > 0` we want the coefficient of `x^n` to be one for sure. If `n == 0` then a random 0/1 is fine.
        if (n > 0) coeffs.set(n);
        return BitPolynomial{std::move(coeffs)};
    }

    /// @}
    /// @name Basic queries:
    /// @{

    /// Returns the degree of the bit-polynomial.
    ///
    /// The degree of the bit-polynomial is the highest power of `x` with a non-zero coefficient.
    ///
    /// **Note:** We return 0 for the zero polynomial.
    ///
    /// # Example
    /// ```
    /// auto coeffs = BitVector<>::from_string("10101000").value();
    /// BitPolynomial p{coeffs};
    /// assert_eq(p.degree(), 4);
    /// assert_eq(p.size(), 8);
    /// ```
    constexpr usize degree() const { return m_coeffs.last_set().value_or(0); }

    /// Returns the "size" of the bit-polynomial which is its total number of coefficients.
    ///
    /// **Note:** Contrast this to the `degree()` method which ignores trailing zero coefficients.
    ///
    /// # Example
    /// ```
    /// auto coeffs = BitVector<>::from_string("10101000").value();
    /// BitPolynomial p{coeffs};
    /// assert_eq(p.degree(), 4);
    /// assert_eq(p.size(), 8);
    /// ```
    constexpr usize size() const { return m_coeffs.size(); }

    /// Returns `true` if the bit-polynomial is some form of the zero BitPolynomial p(x) := 0
    constexpr bool is_zero() const { return m_coeffs.none(); }

    /// Returns `true` if the bit-polynomial is non-zero.
    constexpr bool is_non_zero() const { return m_coeffs.any(); }

    /// Returns `true` if the bit-polynomial is p(x) := 1
    constexpr bool is_one() const { return degree() == 0 && m_coeffs.size() >= 1 && m_coeffs.get(0); }

    /// Returns `true` if the bit-polynomial is either p(x) := 0 or 1
    constexpr bool is_constant() const { return degree() == 0; }

    /// Returns `true` if the bit-polynomial is *monic*, i.e., no trailing zero coefficients.
    constexpr bool is_monic() const { return m_coeffs.trailing_zeros() == 0; }

    /// Returns `true` if the bit-polynomial is empty, i.e., has no coefficients.
    ///
    /// A bit-polynomial with no coefficients is treated as a form of p(x) := 0.
    constexpr bool is_empty() const { return m_coeffs.is_empty(); }

    /// @}
    /// @name Coefficient access:
    /// @{

    /// Returns the coefficient of $x^i$ in the bit-polynomial as a boolean.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::zeros(3);
    /// assert_eq(p[2], false);
    /// ```
    constexpr auto operator[](usize i) const { return m_coeffs[i]; }

    /// Returns the coefficient of $x^i$ in the bit-polynomial as a `BitRef`.
    ///
    /// This allows us to set the coefficient of $x^i$ in the bit-polynomial via the `=` operator.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::zeros(3);
    /// assert_eq(p.to_string(), "0");
    /// p[2] = true;
    /// assert_eq(p.to_string(), "x^2");
    /// ```
    constexpr auto operator[](usize i) { return m_coeffs[i]; }

    /// Read-only access to all the coefficients of the bit-polynomial.
    ///
    /// This returns the coefficients as an immutable reference to a bit-vector.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::ones(3);
    /// auto c = p.coefficients();
    /// assert_eq(c.to_string(), "1111");
    /// ```
    constexpr const coeffs_type& coefficients() const { return m_coeffs; }

    /// Read-write access to all the coefficients of the bit-polynomial.
    ///
    /// This returns the coefficients as a mutable reference to a bit-vector.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::zeros(3);
    /// assert_eq(p.to_string(), "0");
    /// auto& c = p.coefficients();
    /// c.set_all();
    /// assert_eq(p.to_string(), "1 + x + x^2 + x^3");
    /// ```
    constexpr coeffs_type& coefficients() { return m_coeffs; }

    /// Set the bit-polynomial coefficients by *copying* them from a pre-filled bit-store.
    ///
    /// **Note:** The input coefficient bit-store can have any underlying unsigned word type.
    ///
    /// # Example
    /// ```
    /// BitPolynomial p;
    /// assert_eq(p.to_string(), "0");
    /// auto c = BitVector<u8>::ones(3);
    /// p.copy_coefficients(c);
    /// assert_eq(c.to_string(), "111");
    /// assert_eq(p.to_string(), "1 + x + x^2");
    /// ```
    template<BitStore Src>
    constexpr void copy_coefficients(Src const& coeffs) {
        m_coeffs.resize(coeffs.size());
        m_coeffs.copy(coeffs);
    }

    /// Set the BitPolynomial coefficients by *moving* a bit-vector of coefficients into place.
    ///
    /// Use `std::move(coeffs)` in the argument to get this version of `copy_coefficients`.
    ///
    /// **Note:** After the call the argument bit-vector is no longer valid.
    ///
    /// # Example
    /// ```
    /// BitPolynomial p;
    /// assert_eq(p.to_string(), "0");
    /// auto coeffs = BitVector<>::ones(3);
    /// p.copy_coefficients(std::move(coeffs));
    /// assert_eq(p.to_string(), "1 + x + x^2");
    /// ```
    constexpr void move_coefficients(coeffs_type&& coeffs) { m_coeffs = std::move(coeffs); }

    /// @}
    /// @name Resizing:
    /// @{

    /// Clears the BitPolynomial, i.e., sets it to the zero BitPolynomial & returns `self`.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::x_to_the(3);
    /// assert_eq(p.to_string(), "x^3");
    /// p.clear();
    /// assert_eq(p.to_string(), "0");
    /// ```
    constexpr BitPolynomial& clear() {
        m_coeffs.clear();
        return *this;
    }

    /// Resizes the BitPolynomial to have the `n` coefficients and returns `self`.
    ///
    /// # Note
    /// If `n` > `self.size()` then the BitPolynomial is padded with zero coefficients.
    /// If `n` < `self.size()` then the BitPolynomial is truncated which can change the degree of the BitPolynomial.
    ///
    /// # Example
    /// ```
    /// auto coeffs = BitVector<>::from_string("111010").value();
    /// assert_eq(coeffs.to_string(), "111010");
    /// BitPolynomial p{coeffs};
    /// assert_eq(p.to_string(), "1 + x + x^2 + x^4");
    /// assert_eq(p.to_full_string(), "1 + x + x^2 + 0x^3 + x^4 + 0x^5");
    /// p.resize(2);
    /// assert_eq(p.to_string(), "1 + x");
    /// p.resize(4);
    /// assert_eq(p.to_full_string(), "1 + x + 0x^2 + 0x^3");
    /// ```
    constexpr BitPolynomial& resize(usize n) {
        m_coeffs.resize(n);
        return *this;
    }

    /// Shrinks the BitPolynomial to have the minimum number of coefficients and returns `self`.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::x_to_the(3);
    /// assert_eq(p.to_string(), "x^3");
    /// p.shrink_to_fit();
    /// assert_eq(p.to_string(), "x^3");
    /// ```
    constexpr BitPolynomial& shrink_to_fit() {
        m_coeffs.shrink_to_fit();
        return *this;
    }

    /// Kills any trailing zero coefficients, so e.g. p(x) := 0*x^4 + x^2 + x becomes p(x) := x^2 + x.
    ///
    /// # Note
    /// Does nothing to any form of the zero BitPolynomial.
    ///
    /// # Example
    /// ```
    /// auto coeffs = BitVector<>::from_string("101010").value();
    /// BitPolynomial p{coeffs};
    /// assert_eq(p.is_monic(), false);
    /// p.make_monic();
    /// assert_eq(p.is_monic(), true);
    /// ```
    constexpr BitPolynomial& make_monic() {
        if (is_non_zero()) m_coeffs.resize(degree() + 1);
        return *this;
    }

    /// @}
    /// @name Arithmetic Operations:
    /// @{

    /// In-place addition with another bit-polynomial, returning a reference to the result.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::x_to_the(2);
    /// auto q = BitPolynomial<>::x_to_the(3);
    /// p += q;
    /// assert_eq(p.to_string(), "x^2 + x^3");
    /// ```
    constexpr BitPolynomial& operator+=(BitPolynomial const& rhs) {
        // Edge case.
        if (rhs.is_zero()) return *this;

        // Another edge case.
        if (is_zero()) {
            *this = rhs;
            return *this;
        }

        // Perhaps we need to get bigger to accommodate the rhs? Note that added coefficients are zeros.
        auto rhs_degree = rhs.degree();
        if (m_coeffs.size() < rhs_degree + 1) m_coeffs.resize(rhs_degree + 1);

        // Add in the active rhs words.
        for (auto i = 0uz; i < rhs.monic_word_count(); ++i) {
            m_coeffs.set_word(i, m_coeffs.word(i) ^ rhs.m_coeffs.word(i));
        }
        return *this;
    }

    /// In-place subtraction with another bit-polynomial, returning a reference to the result.
    ///
    /// **Note:** In GF(2) subtraction is the same as addition.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::x_to_the(2);
    /// auto q = BitPolynomial<>::x_to_the(3);
    /// p -= q;
    /// assert_eq(p.to_string(), "x^2 + x^3");
    /// ```
    constexpr BitPolynomial& operator-=(BitPolynomial const& rhs) { return operator+=(rhs); }

    /// In-place multiplication with another bit-polynomial, returning a reference to the result.
    ///
    /// **Note:** This method uses the convolution method for bit-vectors under the hood.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::ones(1);
    /// assert_eq(p.to_string(), "1 + x");
    /// auto q = BitPolynomial<>::ones(2);
    /// assert_eq(q.to_string(), "1 + x + x^2");
    /// p *= q;
    /// assert_eq(p.to_string(), "1 + x^3");
    /// ```
    constexpr BitPolynomial& operator*=(BitPolynomial const& rhs) {
        // Edge cases: zero BitPolys.
        if (rhs.is_zero()) return clear();
        if (is_zero()) return *this;

        // Edge cases: either BitPolynomial is one.
        if (rhs.is_one()) return *this;
        if (is_one()) {
            *this = rhs;
            return *this;
        }

        // Generally we pass the work to the convolution method for bit-vectors.
        *this = BitPolynomial{std::move(convolve(m_coeffs, rhs.m_coeffs))};
        return *this;
    }

    /// Returns the sum of two bit-polynomials.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::x_to_the(3);
    /// auto q = BitPolynomial<>::x_to_the(2);
    /// auto r = p + q;
    /// assert_eq(r.to_string(), "x^2 + x^3");
    /// ```
    constexpr auto operator+(BitPolynomial<Word> const& rhs) const {
        // Avoid unnecessary resizing by adding the smaller degree BitPolynomial to the larger one ...
        if (degree() >= rhs.degree()) {
            BitPolynomial<Word> result{*this};
            result += rhs;
            return result;
        } else {
            BitPolynomial<Word> result{rhs};
            result += *this;
            return result;
        }
    }

    /// Returns the difference of two bit-polynomials.
    ///
    /// **Note:** In GF(2) subtraction is the same as addition.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::x_to_the(3);
    /// auto q = BitPolynomial<>::x_to_the(2);
    /// auto r = p - q;
    /// assert_eq(r.to_string(), "x^2 + x^3");
    /// ```
    constexpr auto operator-(BitPolynomial<Word> const& rhs) const {
        // Subtraction is identical to addition in GF(2).
        return operator+(rhs);
    }

    /// Returns the product of two bit-polynomials.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::x_to_the(3);
    /// auto q = BitPolynomial<>::x_to_the(2);
    /// auto r = p * q;
    /// assert_eq(r.to_string(), "x^5");
    /// ```
    constexpr auto operator*(BitPolynomial<Word> const& rhs) const {
        BitPolynomial<Word> result{*this};
        result *= rhs;
        return result;
    }

    /// @}
    /// @name Specialised Arithmetic Operations:
    /// @{

    /// Fills `dst` with the square of this bit-polynomial.
    ///
    /// **Note:** This is more efficient than multiplying two `BitPolynomial`s for this special case.
    ///
    /// # Example
    /// ```
    /// auto coeffs = BitVector<>::from_string("111").value();
    /// BitPolynomial p{coeffs};
    /// assert_eq(p.to_string(), "1 + x + x^2");
    /// BitPolynomial q;
    /// p.squared(q);
    /// assert_eq(q.to_string(), "1 + x^2 + x^4");
    /// ```
    constexpr void squared(BitPolynomial& dst) const {
        // Edge case: any constant BitPolynomial.
        if (is_constant()) {
            dst = *this;
            return;
        }

        // In GF(2) if p(x) = a + bx + cx^2 + ... then p(x)^2 = a^2 + b^2x^2 + c^2x^4 + ...
        // This identity means we can use the `riffle` method to square the BitPolynomial.
        m_coeffs.riffled(dst.m_coeffs);
    }

    /// Returns a new BitPolynomial that is the square of this BitPolynomial.
    ///
    /// **Note:** This is more efficient than multiplying two `BitPolynomial`s for this special case.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::x_to_the(3);
    /// assert_eq(p.to_string(), "x^3");
    /// auto q = p.squared();
    /// assert_eq(q.to_string(), "x^6");
    /// ```
    constexpr BitPolynomial squared() const {
        BitPolynomial dst;
        squared(dst);
        return dst;
    }

    /// Multiplies the BitPolynomial by `x^n` and returns `self`.
    ///
    /// **Note:** This is faster than general multiplication for this special, common case.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::x_to_the(3);
    /// assert_eq(p.to_string(), "x^3");
    /// p.times_x_to_the(2);
    /// assert_eq(p.to_string(), "x^5");
    /// ```
    constexpr BitPolynomial& times_x_to_the(usize n) {
        auto new_degree = degree() + n;
        auto new_size = new_degree + 1;
        if (m_coeffs.size() < new_size) { m_coeffs.resize(new_size); }
        m_coeffs >>= n;
        return *this;
    }

    /// @}
    /// @name Polynomial Pieces:
    /// @{

    /// Makes the destination bit-polynomial a copy of the low `d + 1` coefficients of this bit-polynomial.
    /// The destination bit-polynomial will have degree at most `d`.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::ones(6);
    /// assert_eq(p.to_string(), "1 + x + x^2 + x^3 + x^4 + x^5 + x^6");
    /// BitPolynomial q;
    /// p.sub(4, q);
    /// assert_eq(q.to_string(), "1 + x + x^2 + x^3 + x^4");
    /// p.sub(6, q);
    /// assert_eq(q.to_string(), "1 + x + x^2 + x^3 + x^4 + x^5 + x^6");
    /// p.sub(16, q);
    /// assert_eq(q.to_string(), "1 + x + x^2 + x^3 + x^4 + x^5 + x^6");
    /// p.sub(0, q);
    /// assert_eq(q.to_string(), "1");
    /// ```
    constexpr void sub(usize d, BitPolynomial& dst) const {
        if (d == 0) {
            dst = BitPolynomial<Word>::constant(m_coeffs.get(0));
        } else if (d + 1 >= m_coeffs.size()) {
            dst = *this;
        } else {
            dst.copy_coefficients(m_coeffs.span(0, d + 1));
        }
    }

    /// Returns a new bit-polynomial that is a copy of the low `d + 1` coefficients of this bit-polynomial.
    /// The returned bit-polynomial will have degree at most `d`.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::ones(6);
    /// assert_eq(p.to_string(), "1 + x + x^2 + x^3 + x^4 + x^5 + x^6");
    /// assert_eq(p.sub(4).to_string(), "1 + x + x^2 + x^3 + x^4");
    /// assert_eq(p.sub(6).to_string(), "1 + x + x^2 + x^3 + x^4 + x^5 + x^6");
    /// assert_eq(p.sub(16).to_string(), "1 + x + x^2 + x^3 + x^4 + x^5 + x^6");
    /// assert_eq(p.sub(0).to_string(), "1");
    /// ```
    constexpr auto sub(usize d) const {
        BitPolynomial dst;
        sub(d, dst);
        return std::move(dst);
    }

    /// Splits the bit-polynomial into a low and high part where the low part has degree at most `d`.
    /// On return `self(x) = low(x) + x^(d+1) * high(x)`.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::ones(6);
    /// assert_eq(p.to_string(), "1 + x + x^2 + x^3 + x^4 + x^5 + x^6");
    /// BitPolynomial low, high;
    /// p.split(4, low, high);
    /// assert_eq(low.to_string(), "1 + x + x^2 + x^3 + x^4");
    /// assert_eq(high.to_string(), "1 + x");
    /// p.split(6, low, high);
    /// assert_eq(low.to_string(), "1 + x + x^2 + x^3 + x^4 + x^5 + x^6");
    /// assert_eq(high.to_string(), "0");
    /// p.split(0, low, high);
    /// assert_eq(low.to_string(), "1");
    /// assert_eq(high.to_string(), "1 + x + x^2 + x^3 + x^4 + x^5");
    /// ```
    constexpr void split(usize d, BitPolynomial& lo, BitPolynomial& hi) const {
        if (d == 0) {
            lo = BitPolynomial<Word>::constant(m_coeffs.get(0));
            hi.copy_coefficients(m_coeffs.span(1, m_coeffs.size()));
        } else if (d + 1 >= m_coeffs.size()) {
            lo = *this;
            hi.clear();
        } else {
            lo.copy_coefficients(m_coeffs.span(0, d + 1));
            hi.copy_coefficients(m_coeffs.span(d + 1, m_coeffs.size()));
        }
    }

    /// Splits the bit-polynomial into a low and high part where the low part has degree at most `d`.
    /// On return `self(x) = low(x) + x^(d+1) * high(x)`.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::ones(7);
    /// assert_eq(p.to_string(), "1 + x + x^2 + x^3 + x^4 + x^5 + x^6 + x^7");
    /// auto [low, high] = p.split(4);
    /// assert_eq(low.to_string(), "1 + x + x^2 + x^3 + x^4");
    /// assert_eq(high.to_string(), "1 + x + x^2");
    /// ```
    constexpr auto split(usize d) const {
        BitPolynomial lo, hi;
        split(d, lo, hi);
        return std::make_pair(lo, hi);
    }

    /// @}
    /// @name Polynomial Evaluation:
    /// @{

    /// Evaluates the BitPolynomial at the boolean scalar point `x` and returns the result.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::x_to_the(3);
    /// assert_eq(p(true), true);
    /// assert_eq(p(false), false);
    /// ```
    constexpr bool operator()(bool x) const {
        // Edge case: the zero BitPolynomial.
        if (is_zero()) { return false; }

        // Edge case: `x = false` which is the same as `x = 0` & we always have p(0) = p_0.
        if (!x) { return m_coeffs.get(0); }

        // We are evaluating the BitPolynomial at `x = true` which is the same as `x = 1`: p(1) = p_0 + p_1 + p_2 + ...
        Word sum = 0;
        for (auto i = 0uz; i < m_coeffs.words(); ++i) sum ^= m_coeffs.word(i);
        return gf2::count_ones(sum) % 2 == 1;
    }

    /// Evaluates the bit-polynomial for a *square* `gf2::BitMatrix` argument `M`.
    ///
    /// Uses Horner's method to evaluate `p(M)` where `M` is a square matrix and returns the result as a new bit-matrix.
    ///
    /// # Panics
    /// This method panics if the matrix is not square.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(6);
    /// BitPolynomial p1{BitVector<>::alternating(12)};
    /// assert_eq(p1(m), BitMatrix<>::zeros(6, 6));
    /// BitPolynomial p2{BitVector<>::alternating(6)};
    /// assert_eq(p2(m), BitMatrix<>::identity(6));
    /// ```
    constexpr auto operator()(BitMatrix<Word> const& M) const {
        // The bit-matrix argument must be square.
        gf2_assert(M.is_square(), "Matrix must be square -- not {} x {}!", M.rows(), M.cols());

        // The returned bit-matrix will be n x n.
        auto n = M.rows();

        // Edge case: If the polynomial is zero then the return value is the n x n zero matrix.
        if (is_zero()) return BitMatrix<Word>{n, n};

        // Otherwise we start with the polynomial sum being the n x n identity matrix.
        auto result = BitMatrix<Word>::identity(n);

        // Work backwards a la Horner ...
        auto d = degree();
        while (d > 0) {

            // Always multiply ...
            result = dot(M, result);

            // Add the identity to the sum if the corresponding polynomial coefficient is 1.
            if (m_coeffs[d - 1]) result.add_identity();

            // And count down ...
            d--;
        }

        return result;
    }

    /// @}
    /// @name Modular Reduction:
    /// @{

    /// If this polynomial is P(x) we return x^e mod P(x) for e = n or e = 2^n depending on the `n_is_log2` argument.
    ///
    /// In general, we can write any polynomial h(x) as h(x) = q(x) * P(x) + r(x) where degree(r) < degree(P).
    /// `q(x)` is the quotient polynomial and `r(x)`the remainder polynomial that is returned by this method.
    ///
    /// Here we consider `h(x) = x^e` where the exponent `e` is either `n` or `2^n` depending on the final argument.
    /// Setting `n_is_log2 = true` allows for enormous powers of `x` which is useful for some applications.
    /// The default is `n_is_log2 = false` so we return `x^n mod P(x)`.
    ///
    /// # Panics
    /// This method panics if the polynomial is the zero polynomial.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::x_to_the(3);
    /// auto r = p.reduce_x_to_the(2);
    /// assert_eq(r.to_string(), "x^2");
    /// ```
    BitPolynomial reduce_x_to_the(usize n, bool n_is_log2 = false) const {
        // Error check: anything mod 0 is not defined.
        if (is_zero()) throw std::invalid_argument("... mod P(x) is not defined for P(x) := 0.");

        // Edge case: anything mod 1 = 0.
        if (is_one()) return BitPolynomial<Word>::zero();

        // Edge case: x^0 = 1 and 1 mod P(x) = 1 for any P(x) != 1 (we already handled the case where P(x) := 1).
        if (n == 0 && !n_is_log2) return BitPolynomial<Word>::one();

        // The BitPolynomial P(x) is non-zero so can be written as P(x) = x^d + p(x) where degree(p) < d.
        auto d = degree();

        // Edge case: P(x) = x + c where c is a constant so x = P(x) + c (subtraction in GF(2) is the same as addition).
        // Then x^e = (P(x) + c)^e = terms in powers of P(x) + c^e. Hence, x^e mod P(x) = c^e = c.
        if (d == 1) return BitPolynomial<Word>::constant(m_coeffs.get(1));

        // We can write p(x) = p_0 + p_1 x + ... + p_{d-1} x^{d-1}. All that matters are those coefficients.
        auto p = m_coeffs.sub(0, d);

        // A lambda to perform: q(x) <- x*q(x) mod P(x) where degree(q) < d.
        // The lambda works on the coefficients of q(x) passed as a bit-vector q.
        auto times_x_step = [&](auto& q) {
            bool add_p = q[d - 1];
            q >>= 1;
            if (add_p) q ^= p;
        };

        // Iteratively precompute x^{d+i} mod P(x) for i = 0, 1, ..., d-1 starting with x^d mod P(x) ~ p.
        // We store all the bit-vectors in a standard `std::vector` of length d.
        std::vector<coeffs_type> power_mod(d, coeffs_type{d});
        power_mod[0] = p;
        for (auto i = 1uz; i < d; ++i) {
            power_mod[i] = power_mod[i - 1];
            times_x_step(power_mod[i]);
        }

        // Some workspace we use/reuse below in order to minimize allocations/deallocations.
        coeffs_type s{2 * d}, h{d};

        // lambda to perform: q(x) <- q(x)^2 mod P(x) where degree(q) < d.
        // The lambda works on the coefficients of q(x) passed as a bit-vector q.
        auto square_step = [&](auto& q) {
            // Square q(x) storing the result in workspace `s`.
            q.riffled(s);

            // Split s(x) = l(x) + x^d * h(x) where l(x) & h(x) are both of degree less than d.
            // We reuse q(x) for l(x).
            s.split_at(d, q, h);

            // s(x) = q(x) + h(x) so s(x) mod P(x) = q(x) + h(x) mod P(x) which we handle term by term.
            // If h(x) != 0 then at most every second term in h(x) is 1 (nature of BitPolynomial squares in GF(2)).
            if (auto h_first = h.first_set()) {
                auto h_last = h.last_set();
                for (auto i = *h_first; i <= *h_last; i += 2)
                    if (h[i]) q ^= power_mod[i];
            }
        };

        // Our return value r(x) := x^e mod P(x) has degree < d: r(x) = r_0 + r_1 x + ... + r_{d-1} x^{d-1}.
        coeffs_type r{d};

        // If `n_is_log2` is `true`, we are reducing x^(2^n) mod P(x) which is done iteratively by squaring.
        if (n_is_log2) {
            // Note that we already handled edge case where P(x) = x + c above.
            // Start with r(x) = x mod P(x) -> x^2 mod P(x) -> x^4 mod P(x) ...
            r[1] = true;
            for (auto i = 0uz; i < n; ++i) square_step(r);
            return BitPolynomial{std::move(r)};
        }

        // Small exponent case: n < d => x^n mod P(x) = x^n.
        if (n < d) return BitPolynomial::x_to_the(n);

        // Matching exponent case: n = d => x^n mod P(x) = x^d mod P(x) = p(x).
        if (n == d) return BitPolynomial{std::move(p)};

        // General case: n > d is handled by a square & multiply algorithm.
        usize n_bit = std::bit_floor(n);

        // Start with r(x) = x mod P(x) which takes care of the most significant binary digit in n.
        // TODO: We could start a bit further along with a higher power r(x) := x^? mod P(x) where ? < n but > 1.
        r[1] = 1;
        n_bit >>= 1;

        // And off we go ...
        while (n_bit) {

            // Always do a square step and then a times_x step if necessary (i.e. if current bit in N is set).
            square_step(r);
            if (n & n_bit) times_x_step(r);

            // On to the next bit position in n.
            n_bit >>= 1;
        }

        // Made it.
        return BitPolynomial{std::move(r)};
    }

    /// @}
    /// @name String Representations:
    /// @{

    /// Returns a readable string representation of the bit-polynomial `p(x) = p0 + p1x + ...`
    ///
    /// You can specify the variable name using the `var` parameter. The default variable name is `x`.
    ///
    /// **Note:** We do not show any terms with zero coefficients.
    ///
    /// # Example
    /// ```
    /// auto coeffs = BitVector<>::from_string("101010").value();
    /// BitPolynomial p{coeffs};
    /// assert_eq(p.to_string("M"), "1 + M^2 + M^4");
    /// ```
    std::string to_string(std::string_view var = "x") const {
        // Edge case: the zero BitPolynomial.
        if (is_zero()) return "0";

        // Otherwise we construct the string ...
        std::ostringstream ss;

        // All terms other than the first one are preceded by a "+"
        bool first_term = true;
        for (auto i = 0uz; i <= degree(); ++i) {
            if (m_coeffs.get(i)) {
                if (i == 0) {
                    ss << "1";
                } else {
                    if (!first_term) ss << " + ";
                    if (i == 1)
                        ss << var;
                    else
                        ss << var << "^" << i;
                }
                first_term = false;
            }
        }
        return ss.str();
    }

    /// Returns a readable *full* string representation of the bit-polynomial.
    ///
    /// You can specify the variable name using the `var` parameter. The default variable name is `x`.
    ///
    ///  **Note:** We show all terms including those with zero coefficients.
    ///
    /// # Example
    /// ```
    /// auto coeffs = BitVector<>::from_string("101010").value();
    /// BitPolynomial p{coeffs};
    /// assert_eq(p.to_full_string("M"), "1 + 0M + M^2 + 0M^3 + M^4 + 0M^5");
    /// ```
    std::string to_full_string(std::string_view var = "x") const {
        // Edge case: the zero BitPolynomial.
        if (is_empty()) return "0";

        // Otherwise we construct the string ...
        std::ostringstream ss;

        // Start with the first term ...
        if (m_coeffs.get(0))
            ss << "1";
        else
            ss << "0";

        // Now all the other terms, each preceded by a "+"
        for (auto i = 1uz; i < size(); ++i) {
            ss << " + ";
            if (m_coeffs.get(i) == false) ss << "0";
            if (i == 1)
                ss << var;
            else
                ss << var << "^" << i;
        }
        return ss.str();
    }

    /// The usual output stream operator for a bit-polynomial
    std::ostream& operator<<(std::ostream& s) const { return s << to_string(); }

    /// @}
    /// @name Equality Operator:
    /// @{

    /// Equality operator checks that any pair of bit-polynomials are equal in content.
    ///
    ///  **Note:** We ignore any high order trailing zero coefficients in either operand so 1 + x == 1 + x + 0x^2.
    ///
    /// # Example
    /// ```
    /// auto p = BitPolynomial<>::x_to_the(3);
    /// auto q = BitPolynomial<>::zeros(1000);
    /// q[3] = true;
    /// assert(p == q);
    /// ```
    friend constexpr bool operator==(BitPolynomial const& lhs, BitPolynomial const& rhs) {
        // Edge case.
        if (&lhs == &rhs) return true;

        // Check the active words for equality.
        auto count = lhs.monic_word_count();
        if (rhs.monic_word_count() != count) return false;
        for (auto i = 0uz; i < count; ++i)
            if (lhs.m_coeffs.word(i) != rhs.m_coeffs.word(i)) return false;

        // Made it
        return true;
    }

    /// @}

private:
    // Returns the number of "active" words underlying the coefficient bit-vector.
    // There may be high order trailing zero coefficients that have no real impact on most bit-polynomial calculations.
    // This method returns the number of "words" that matter in the coefficient bit-vector store of words.
    constexpr usize monic_word_count() const {
        if (auto deg = m_coeffs.last_set()) return word_index<Word>(*deg) + 1;
        return 0;
    }
};

} // namespace gf2

// --------------------------------------------------------------------------------------------------------------------
// Specialises `std::formatter` to handle bit-polynomials ...
// -------------------------------------------------------------------------------------------------------------------

/// Specialise `std::formatter` for our `gf2::BitPolynomial<Word>` type.
///
/// You can use the format specifier to set the variable symbol (the default is "x").
/// For example, `std::format("{:mat}", p)` ->  "p0 + p1*mat + p2*mat^2 + ...".
///
/// # Example
/// ```
/// auto p = BitPolynomial<>::ones(2);
/// auto px = std::format("{}", p);
/// auto py = std::format("{:y}", p);
/// assert_eq(px, "1 + x + x^2");
/// assert_eq(py, "1 + y + y^2");
/// ```
template<gf2::Unsigned Word>
struct std::formatter<gf2::BitPolynomial<Word>> {

    /// Parse a bit-polynomial format specifier for a variable name (default is "x").
    constexpr auto parse(std::format_parse_context const& ctx) {
        auto it = ctx.begin();
        if (*it != '}') m_var.clear();
        while (it != ctx.end() && *it != '}') {
            m_var += *it;
            ++it;
        }
        return it;
    }

    /// Defer the work to the `to_string(...)` method in the class.
    template<class FormatContext>
    auto format(gf2::BitPolynomial<Word> const& rhs, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "{}", rhs.to_string(m_var));
    }

    std::string m_var = "x";
};