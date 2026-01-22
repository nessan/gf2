#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// Matrices over over GF(2) -- *bit-matrices*. <br>
/// See the [BitMatrix](docs/pages/BitMatrix.md) page for more details.

#include <gf2/BitPolynomial.h>
#include <gf2/BitVector.h>
#include <gf2/RNG.h>

#include <string>
#include <optional>
#include <regex>

namespace gf2 {

// Forward declarations to avoid recursive inclusion issues
// clang-format off
template<Unsigned Word> class BitGauss;
template<Unsigned Word> class BitLU;
// clang-format on

/// A dynamically-sized matrix over GF(2) stored as a vector of bit-vectors representing the rows of the matrix. The row
/// elements are compactly stored in standard vectors of primitive unsigned words whose type is given by the template
/// parameter `Word`.
///
/// @note These matrices are stored by row, so it is always more efficient to arrange computations to operate on rows
/// instead of columns. The high-level methods in this library take care of this for you.
template<Unsigned Word = usize>
class BitMatrix {
private:
    /// We store each *row* in a bit-matrix as a bit-vector.
    std::vector<BitVector<Word>> m_rows;

public:
    // The row type is a `BitVector` ...
    using row_type = BitVector<Word>;

    /// The underlying unsigned word type used to store the bits.
    using word_type = Word;

    /// @name Constructors
    /// @{

    /// The default constructor creates an empty bit-matrix with no rows or columns.
    ///
    /// # Example
    /// ```
    /// BitMatrix m;
    /// assert_eq(m.to_compact_binary_string(), "");
    /// ```
    constexpr BitMatrix() : m_rows{} {}

    /// Constructs the `n x n` square bit-matrix with all the elements set to 0.
    ///
    /// If `n` is zero, we create an empty bit-matrix with no rows or columns.
    ///
    /// # Example
    /// ```
    /// BitMatrix m{3};
    /// assert_eq(m.to_compact_binary_string(), "000 000 000");
    /// ```
    constexpr BitMatrix(usize n) : m_rows{} {
        if (n > 0) resize(n, n);
    }

    /// Constructs the `m x n` bit-matrix with all the elements set to 0.
    ///
    /// If either `m` or `n` is zero, we create an empty bit-matrix with no rows or columns.
    ///
    /// # Example
    /// ```
    /// BitMatrix m{3, 4};
    /// assert_eq(m.to_compact_binary_string(), "0000 0000 0000");
    /// ```
    constexpr BitMatrix(usize m, usize n) : m_rows{} {
        if (m > 0 && n > 0) resize(m, n);
    }

    /// Construct a bit-matrix by *copying* a given set of rows which can be any `BitStore` subclass.
    ///
    /// @note We check that all the rows have the same size unless `NDEBUG` is defined.
    ///
    /// # Example
    /// ```
    /// auto rows = std::vector{BitVector<>::zeros(3), BitVector<>::ones(3)};
    /// BitMatrix m{rows};
    /// assert_eq(m.to_compact_binary_string(), "000 111");
    /// ```
    constexpr BitMatrix(std::vector<row_type> const& rows) : m_rows{rows} {
        gf2_assert(check_rows(m_rows), "Not all rows have the same size!");
    }

    /// Construct a bit-matrix by *moving* the given rows.
    ///
    /// Use `std::move(rows)` in the constructor argument to get this version.
    /// The input rows are moved into the bit-matrix so they are no longer valid after this constructor.
    ///
    /// @note We check that all the rows have the same size unless `NDEBUG` is defined.
    ///
    /// # Example
    /// ```
    /// auto rows = std::vector{BitVector<>::zeros(3), BitVector<>::ones(3)};
    /// BitMatrix m{std::move(rows)};
    /// assert_eq(m.to_compact_binary_string(), "000 111");
    /// ```
    constexpr BitMatrix(std::vector<BitVector<Word>>&& rows) : m_rows{std::move(rows)} {
        gf2_assert(check_rows(m_rows), "Not all rows have the same size!");
    }

    /// @}
    /// @name Factory Constructors
    /// @{

    /// Factory method to create the `m x n` zero bit-matrix with all the elements set to 0.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zeros(3, 4);
    /// assert_eq(m.to_compact_binary_string(), "0000 0000 0000");
    /// ```
    static constexpr BitMatrix zeros(usize m, usize n) { return BitMatrix{m, n}; }

    /// Factory method to create the `m x m` square bit-matrix with all the elements set to 0.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zeros(3);
    /// assert_eq(m.to_compact_binary_string(), "000 000 000");
    /// ```
    static constexpr BitMatrix zeros(usize m) { return BitMatrix{m, m}; }

    /// Factory method to create the `m x n` bit-matrix with all the elements set to 1.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::ones(3, 4);
    /// assert_eq(m.to_compact_binary_string(), "1111 1111 1111");
    /// ```
    static constexpr BitMatrix ones(usize m, usize n) {
        auto rows = std::vector{m, BitVector<Word>::ones(n)};
        return BitMatrix{std::move(rows)};
    }

    /// Factory method to create the `m x m` square bit-matrix with all the elements set to 1.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::ones(3);
    /// assert_eq(m.to_compact_binary_string(), "111 111 111");
    /// ```
    static constexpr BitMatrix ones(usize m) { return BitMatrix::ones(m, m); }

    /// Factory method to create the `m x n` bit-matrix with alternating elements.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::alternating(3, 4);
    /// assert_eq(m.to_compact_binary_string(), "1010 0101 1010");
    /// ```
    static constexpr BitMatrix alternating(usize m, usize n) {
        auto rows = std::vector{m, BitVector<Word>::alternating(n)};
        // Flip every other row.
        for (auto i = 1uz; i < m; i += 2) rows[i].flip_all();
        return BitMatrix{std::move(rows)};
    }

    /// Factory method to create the `m x m` square bit-matrix with alternating elements.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::alternating(3);
    /// assert_eq(m.to_compact_binary_string(), "101 010 101");
    /// ```
    static constexpr BitMatrix alternating(usize m) { return BitMatrix::alternating(m, m); }

    /// Factory method to create the `m x n` bit-matrix from the outer product of the given bit-stores.
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<>::from_string("101").value();
    /// auto v = BitVector<>::from_string("110").value();
    /// auto m = BitMatrix<>::from_outer_product(u, v);
    /// assert_eq(m.to_compact_binary_string(), "110 000 110");
    /// ```
    template<BitStore Lhs, BitStore Rhs>
        requires std::same_as<typename Lhs::word_type, Word> && std::same_as<typename Rhs::word_type, Word>
    static constexpr BitMatrix from_outer_product(Lhs const& u, Rhs const& v) {
        BitMatrix result;
        for (auto i = 0uz; i < u.size(); ++i) {
            row_type row = u.get(i) ? row_type(v) : row_type(v.size());
            result.m_rows.push_back(std::move(row));
        }
        return result;
    }

    /// Factory method to create the `m x n` bit-matrix from the outer sum of the given bit-stores.
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<>::from_string("101").value();
    /// auto v = BitVector<>::from_string("110").value();
    /// auto m = BitMatrix<>::from_outer_sum(u, v);
    /// assert_eq(m.to_compact_binary_string(), "001 110 001");
    /// ```
    template<BitStore Lhs, BitStore Rhs>
        requires std::same_as<typename Lhs::word_type, Word> && std::same_as<typename Rhs::word_type, Word>
    static constexpr BitMatrix from_outer_sum(Lhs const& u, Rhs const& v) {
        BitMatrix result;
        for (auto i = 0uz; i < u.size(); ++i) {
            result.m_rows.push_back(v);
            if (u.get(i)) result.m_rows.back().flip_all();
        }
        return result;
    }

    /// Factory method to construct an $m \times n$ bit-matrix by repeatedly calling `f(i, j)` for each index pair.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::from(3, 2, [](usize i, usize) { return i % 2 == 0; });
    /// assert_eq(m.to_compact_binary_string(), "11 00 11");
    /// ```
    static constexpr BitMatrix from(usize m, usize n, std::invocable<usize, usize> auto f) {
        BitMatrix result{m, n};
        for (auto i = 0uz; i < m; ++i)
            for (auto j = 0uz; j < n; ++j)
                if (f(i, j)) result.set(i, j, true);
        return result;
    }

    /// @}
    /// @name Randomly Populated Constructors
    /// @{

    /// Factory method to generate a bit-matrix of size `m x n` where the elements are picked at random.
    ///
    /// This is the workhorse method for generating random bit-matrices that allows one to specify the probability of
    /// each bit being 1, and also a seed for the random number generator for reproducibility. If you set the seed
    /// to 0 then computer entropy is used to seed the RNG.
    ///
    /// The default call `BitMatrix<>::random(m, n)` produces a random bit-matrix with each bit being 1 with probability
    /// 0.5 and where the RNG is seeded from entropy.
    ///
    /// @param m The number of rows in the bit-matrix to generate.
    /// @param n The number of columns in the bit-matrix to generate.
    /// @param p The probability of the elements being 1 (defaults to a fair coin, i.e. 50-50).
    /// @param seed The seed to use for the random number generator (defaults to 0, which means use entropy).
    ///
    /// @note If `p < 0` then the bit-matrix is all zeros, if `p > 1` then the bit-matrix is all ones.
    ///
    /// # Example
    /// ```
    /// std::uint64_t seed = 1234567890;
    /// auto u = BitMatrix<>::random(3, 2, 0.5, seed);
    /// auto v = BitMatrix<>::random(3, 2, 0.5, seed);
    /// assert(u == v);
    /// ```
    static BitMatrix random(usize m, usize n, double p = 0.5, std::uint64_t seed = 0) {
        // Keep a single static RNG per thread for all calls to this method, seeded with entropy on the first call.
        thread_local RNG rng;

        // Edge case handling ...
        if (p < 0) return BitMatrix::zeros(m, n);

        // Scale p by 2^64 to remove floating point arithmetic from the main loop below.
        // If we determine p rounds to 1 then we can just set all elements to 1 and return early.
        p = p * 0x1p64 + 0.5;
        if (p >= 0x1p64) return BitMatrix::ones(m, n);

        // p does not round to 1 so we use a 64-bit URNG and check each draw against the 64-bit scaled p.
        auto scaled_p = static_cast<std::uint64_t>(p);

        // If a seed was provided, set the RNG's seed to it. Otherwise, we carry on from where we left off.
        std::uint64_t old_seed = rng.seed();
        if (seed != 0) rng.set_seed(seed);

        auto result = BitMatrix::zeros(m, n);
        for (auto i = 0uz; i < m; ++i) {
            for (auto j = 0uz; j < n; ++j) {
                if (rng.u64() < scaled_p) result.set(i, j, true);
            }
        }

        // Restore the old seed if necessary.
        if (seed != 0) rng.set_seed(old_seed);

        return result;
    }

    /// Factory method to generate a bit-matrix of size `m x n` where the elements are from independent fair
    /// coin flips generated from an RNG seeded with the given `seed`.
    ///
    /// This allows one to have reproducible random bit-vectors, which is useful for testing and debugging.
    ///
    /// @param m The number of rows in the bit-matrix to generate.
    /// @param n The number of columns in the bit-matrix to generate.
    /// @param seed The seed to use for the random number generator (if you set this to 0 then entropy is used).
    ///
    /// @note If `p < 0` then the bit-matrix is all zeros, if `p > 1` then the bit-matrix is all ones.
    ///
    /// # Example
    /// ```
    /// std::uint64_t seed = 1234567890;
    /// auto u = BitMatrix<>::seeded_random(3, 2, seed);
    /// auto v = BitMatrix<>::seeded_random(3, 2, seed);
    /// assert(u == v);
    /// ```
    static BitMatrix seeded_random(usize m, usize n, std::uint64_t seed) { return random(m, n, 0.5, seed); }

    /// Factory method to generate a square bit-matrix of size `m x m` where the elements are from independent
    /// fair coin flips generated from an RNG seeded with the given `seed`.
    ///
    /// This allows one to have reproducible random square bit-matrices, which is useful for testing and debugging.
    ///
    /// @param m The number of rows & columns in the bit-matrix to generate.
    /// @param seed The seed to use for the random number generator (if you set this to 0 then entropy is used).
    ///
    /// @note If `p < 0` then the bit-matrix is all zeros, if `p > 1` then the bit-matrix is all ones.
    ///
    /// # Example
    /// ```
    /// std::uint64_t seed = 1234567890;
    /// auto u = BitMatrix<>::seeded_random(3, seed);
    /// auto v = BitMatrix<>::seeded_random(3, seed);
    /// assert(u == v);
    /// ```
    static BitMatrix seeded_random(usize m, std::uint64_t seed) { return random(m, m, 0.5, seed); }

    /// Factory method to generate a bit-matrix of size `m x n` where the elements are from independent fair
    /// coin flips and where each bit is 1 with probability `p`.
    ///
    /// @param m The number of rows in the bit-matrix to generate.
    /// @param n The number of columns in the bit-matrix to generate.
    /// @param p The probability of the elements being 1.
    ///
    /// # Example
    /// ```
    /// auto u = BitMatrix<>::biased_random(10, 7, 0.3);
    /// auto v = BitMatrix<>::biased_random(10, 7, 0.3);
    /// assert_eq(u.size(), v.size());
    /// ```
    static BitMatrix biased_random(usize m, usize n, double p) { return random(m, n, p, 0); }

    /// Factory method to generate a square bit-matrix of size `m x m` where the elements are from independent
    /// fair coin flips and where each bit is 1 with probability `p`.
    ///
    /// @param m The number of rows in the bit-matrix to generate.
    /// @param p The probability of the elements being 1.
    ///
    /// # Example
    /// ```
    /// auto u = BitMatrix<>::biased_random(10, 0.3);
    /// assert_eq(u.size(), 100);
    /// ```
    static BitMatrix biased_random(usize m, double p) { return random(m, m, p, 0); }

    /// @}
    /// @name Constructors for Special Matrices
    /// @{

    /// Factory method to create the `m x m` square "zero" bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// assert_eq(m.to_compact_binary_string(), "000 000 000");
    /// ```
    static constexpr BitMatrix zero(usize m) { return BitMatrix{m, m}; }

    /// Factory method to create the `m x m` identity bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// assert_eq(m.to_compact_binary_string(), "100 010 001");
    /// ```
    static constexpr BitMatrix identity(usize m) {
        BitMatrix result{m, m};
        for (auto i = 0uz; i < m; ++i) result.set(i, i, true);
        return result;
    }

    /// Constructs a square *companion BitMatrix* with a copy of the given top row and a sub-diagonal of `1`s.
    ///
    /// The top row should be passed as a bit-store and is copied to the first row of the BitMatrix.
    /// The rest of the BitMatrix is initialized to zero and the sub-diagonal is set to `1`s.
    ///
    /// # Example
    /// ```
    /// auto top_row = BitVector<>::ones(5);
    /// auto m = BitMatrix<>::companion(top_row);
    /// assert_eq(m.to_compact_binary_string(), "11111 10000 01000 00100 00010");
    /// ```
    template<BitStore Store>
        requires std::same_as<typename Store::word_type, Word>
    static constexpr BitMatrix companion(Store const& top_row) {
        // Edge case:
        if (top_row.size() == 0) return BitMatrix{};
        auto result = BitMatrix::zero(top_row.size());
        result.m_rows[0].copy(top_row);
        result.set_sub_diagonal(1);
        return result;
    }

    /// Constructs the `n x n` shift-left by `p` places BitMatrix.
    ///
    /// If the bit-matrix is multiplied by a bit-vector, the result is the bit-vector shifted left by `p` places.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::left_shift(5, 2);
    /// auto v = BitVector<>::ones(5);
    /// assert_eq(dot(m, v).to_string(), "11100");
    /// ```
    static constexpr BitMatrix left_shift(usize n, usize p) {
        auto result = BitMatrix::zeros(n, n);
        result.set_super_diagonal(p);
        return result;
    }

    /// Constructs the `n x n` shift-right by `p` places BitMatrix.
    ///
    /// If the bit-matrix is multiplied by a bit-vector, the result is the bit-vector shifted right by `p` places.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::right_shift(5, 2);
    /// auto v = BitVector<>::ones(5);
    /// assert_eq(dot(m, v).to_string(), "00111");
    /// ```
    static BitMatrix right_shift(usize n, usize p) {
        auto result = BitMatrix::zeros(n, n);
        result.set_sub_diagonal(p);
        return result;
    }

    /// Constructs the `n x n` rotate-left by `p` places matrix.
    ///
    /// If the bit-matrix is multiplied by a bit-vector, the result is the bit-vector rotated right by `p` places.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::left_rotation(5, 2);
    /// auto v = BitVector<>::from_binary_string("11100").value();
    /// assert_eq(dot(m, v).to_string(), "00111");
    /// ```
    static BitMatrix left_rotation(usize n, usize p) {
        auto result = BitMatrix::zeros(n, n);
        for (auto i = 0uz; i < n; ++i) {
            auto j = (i + n - p) % n;
            result.set(i, j, true);
        }
        return result;
    }

    /// Constructs the `n x n` rotate-right by `p` places matrix.
    ///
    /// If the bit-matrix is multiplied by a bit-vector, the result is the bit-vector rotated right by `p` places.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::right_rotation(5, 2);
    /// auto v = BitVector<>::from_binary_string("11100").value();
    /// assert_eq(dot(m, v).to_string(), "10011");
    /// ```
    static BitMatrix right_rotation(usize n, usize p) {
        auto result = BitMatrix::zeros(n, n);
        for (auto i = 0uz; i < n; ++i) {
            auto j = (i + p) % n;
            result.set(i, j, true);
        }
        return result;
    }

    /// @}
    /// @name Construction by reshaping bit-stores.
    /// @{

    /// Factory method to reshape a bit-vector `v` that is assumed to a sequence of `r` rows into a bit-matrix.
    //
    /// # Example
    /// ```
    /// auto v = BitVector<>::ones(15);
    /// auto m1 = BitMatrix<>::from_row_store(v, 3).value();
    /// assert_eq(m1.to_compact_binary_string(), "11111 11111 11111");
    /// auto m2 = BitMatrix<>::from_row_store(v, 5).value();
    /// assert_eq(m2.to_compact_binary_string(), "111 111 111 111 111");
    /// auto m3 = BitMatrix<>::from_row_store(v, 15).value();
    /// assert_eq(m3.to_compact_binary_string(), "1 1 1 1 1 1 1 1 1 1 1 1 1 1 1");
    /// ```
    template<BitStore Store>
        requires std::same_as<typename Store::word_type, Word>
    static std::optional<BitMatrix> from_row_store(Store const& v, usize r) {
        // Edge case?
        if (v.size() == 0) return BitMatrix{};

        // Error handling ...
        if (r == 0 || v.size() % r != 0) return std::nullopt;

        // Number of columns (for sure this is an even division)
        usize c = v.size() / r;

        // Create the bit-matrix and copy the rows.
        BitMatrix result{r, c};
        for (auto i = 0uz; i < r; ++i) {
            auto begin = i * c;
            auto end = begin + c;
            result.m_rows[i].copy(v.span(begin, end));
        }
        return result;
    }

    /// Factory method to reshape a bit-vector that is assumed to a sequence of `c` columns into a bit-matrix.
    ///
    /// @param v The bit-vector to reshape.
    /// @param c The number of columns.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<>::ones(15);
    /// auto m1 = BitMatrix<>::from_col_store(v, 3).value();
    /// assert_eq(m1.to_compact_binary_string(), "111 111 111 111 111");
    /// auto m2 = BitMatrix<>::from_col_store(v, 5).value();
    /// assert_eq(m2.to_compact_binary_string(), "11111 11111 11111");
    /// auto m3 = BitMatrix<>::from_col_store(v, 15).value();
    /// assert_eq(m3.to_compact_binary_string(), "111111111111111");
    /// ```
    template<BitStore Store>
        requires std::same_as<typename Store::word_type, Word>
    static std::optional<BitMatrix> from_col_store(Store const& v, usize c) {
        // Edge case?
        if (v.size() == 0) return BitMatrix{};

        // Error handling ...
        if (c == 0 || v.size() % c != 0) return std::nullopt;

        // Number of columns (for sure this is an even division)
        usize r = v.size() / c;

        // Create the bit-matrix and copy the rows.
        BitMatrix result{r, c};
        usize     iv = 0;
        for (auto j = 0uz; j < c; ++j) {
            for (auto i = 0uz; i < r; ++i) {
                if (v.get(iv)) result.m_rows[i].set(j);
                iv++;
            }
        }
        return result;
    }

    /// @}
    /// @name Construction from strings
    /// @{

    /// Factory method to construct a bit-matrix from a string that is assumed to be a sequence of rows.
    ///
    /// The rows can be separated by newlines, white space, commas, single quotes, or semicolons.
    /// Each row should be a binary or hex string representation of a bit-vector.
    /// The rows can have an optional prefix of "0b", "0x" or "0X" to indicate that the string is binary or hex.
    ///
    /// A hex string can have a suffix of ".2", ".4", or ".8" to indicate the base of the last digit/character.
    /// This allows for rows of any length as opposed to just a multiple of 4.
    ///
    /// After parsing, the rows must all have the same length.
    ///
    /// # Example
    /// ```
    /// auto m1 = BitMatrix<>::from_string("111   111\n111").value();
    /// assert_eq(m1.to_compact_binary_string(), "111 111 111");
    /// auto m2 = BitMatrix<>::from_string("0XAA; 0b1111_0000").value();
    /// assert_eq(m2.to_compact_binary_string(), "10101010 11110000");
    /// auto m3 = BitMatrix<>::from_string("0x7.8 000").value();
    /// assert_eq(m3.to_compact_binary_string(), "111 000");
    /// ```
    static std::optional<BitMatrix> from_string(std::string_view s) {
        // Edge case
        if (s.empty()) return BitMatrix{};

        // We split the string into tokens using the standard regex library.
        std::string                src(s);
        auto                       delims = std::regex(R"([\s|,|;]+)");
        std::sregex_token_iterator iter{src.cbegin(), src.cend(), delims, -1};
        std::vector<std::string>   tokens{iter, {}};

        // Zap any empty tokens & check there is something to do
        tokens.erase(std::remove_if(tokens.begin(), tokens.end(), [](std::string_view x) { return x.empty(); }),
                     tokens.end());
        if (tokens.empty()) return BitMatrix{};

        // We hope to fill a BitMatrix.
        BitMatrix result;

        // Iterate through the possible rows
        usize n_rows = tokens.size();
        usize n_cols = 0;
        for (auto i = 0uz; i < n_rows; ++i) {
            // Attempt to parse the current token as a row for the bit-matrix.
            auto r = row_type::from_string(tokens[i]);

            // Parse failure?
            if (!r) return std::nullopt;

            // We've read a potentially valid BitMatrix row.
            if (i == 0) {
                // First row sets the number of columns
                n_cols = r->size();
                result.resize(n_rows, n_cols);
            } else {
                // Subsequent rows better have the same number of elements!
                if (r->size() != n_cols) return std::nullopt;
            }
            result.m_rows[i] = *r;
        }
        return result;
    }

    /// @}
    /// @name Basic queries
    /// @{

    /// Returns the number of rows in the bit-matrix.
    constexpr usize rows() const { return m_rows.size(); }

    /// Returns the number of columns in the bit-matrix.
    constexpr usize cols() const { return m_rows.size() > 0 ? m_rows[0].size() : 0; }

    /// Returns the totalnumber of elements in the bit-matrix.
    constexpr usize size() const { return rows() * cols(); }

    /// Is this an empty bit-matrix?
    constexpr bool is_empty() const { return rows() == 0; }

    /// @}
    /// @name Checks for Special Bit-Matrices
    /// @{

    /// Returns `true` if this a square bit-matrix? Note that empty bit-matrices are NOT considered square.
    ///
    /// # Example
    /// ```
    /// BitMatrix m;
    /// assert_eq(m.is_square(), false);
    /// m.resize(3, 3);
    /// assert_eq(m.is_square(), true);
    /// m.resize(3, 4);
    /// assert_eq(m.is_square(), false);
    /// ```
    constexpr bool is_square() const { return rows() != 0 && rows() == cols(); }

    /// Returns `true` if this a square *zero* bit-matrix?
    ///
    /// # Example
    /// ```
    /// BitMatrix m;
    /// assert_eq(m.is_zero(), false);
    /// m.resize(3, 3);
    /// assert_eq(m.is_zero(), true);
    /// m.resize(3, 4);
    /// assert_eq(m.is_zero(), false);
    /// ```
    constexpr bool is_zero() const { return is_square() && none(); }

    /// Returns `true` if this is the identity bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// assert_eq(m.is_identity(), true);
    /// assert_eq(m.to_compact_binary_string(), "100 010 001");
    /// ```
    constexpr bool is_identity() const {
        if (!is_square()) return false;
        for (auto i = 0uz; i < rows(); ++i) {
            row_type r = m_rows[i];
            r.flip(i);
            if (r.any()) return false;
        }
        return true;
    }

    /// Returns `true` if this is a symmetric square bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// assert_eq(m.is_symmetric(), true);
    /// m.row(0).set_all();
    /// assert_eq(m.is_symmetric(), false);
    /// ```
    constexpr bool is_symmetric() const {
        if (!is_square()) return false;
        for (auto i = 0uz; i < rows(); ++i) {
            for (auto j = 0uz; j < cols(); ++j) {
                if (get(i, j) != get(j, i)) return false;
            }
        }
        return true;
    }

    /// @}
    /// @name Bit Counts
    /// @{

    /// Returns the number of one elements in the bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// assert_eq(m.count_ones(), 0);
    /// m.set_all();
    /// assert_eq(m.count_ones(), 9);
    /// ```
    constexpr usize count_ones() const {
        usize count = 0;
        for (const auto& row : m_rows) count += row.count_ones();
        return count;
    }

    /// Returns the number of zero elements in the bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// assert_eq(m.count_zeros(), 6);
    /// ```
    constexpr usize count_zeros() const { return size() - count_ones(); }

    /// Returns the number of ones on the main diagonal of the bit-matrix.
    ///
    /// @note In debug mode, this method will panic if the bit-matrix is not square.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// assert_eq(m.count_ones_on_diagonal(), 3);
    /// ```
    constexpr usize count_ones_on_diagonal() const {
        gf2_debug_assert(is_square(), "BitMatrix is {} x {} but it should be square!", rows(), cols());
        usize result = 0;
        for (auto i = 0uz; i < rows(); ++i)
            if (get(i, i)) ++result;
        return result;
    }

    /// Returns the "sum" of the main diagonal elements of the bit-matrix.
    ///
    /// Returns the "sum" of the main diagonal elements of the bit-matrix.
    ///
    /// @note In debug mode, this method will panic if the bit-matrix is not square.
    ///
    /// # Example
    /// ```
    /// auto m1 = BitMatrix<>::identity(3);
    /// assert_eq(m1.trace(), true);
    /// auto m2 = BitMatrix<>::zero(4);
    /// assert_eq(m2.trace(), false);
    /// ```
    constexpr bool trace() const { return count_ones_on_diagonal() % 2 == 1; }

    /// @}
    /// @name Overall State Queries
    /// @{

    /// Returns `true` if any element of the bit-matrix is set.
    ///
    /// Empty matrices are considered to have no set bits.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// assert_eq(m.any(), false);
    /// m.set(0, 0);
    /// assert_eq(m.any(), true);
    /// m.clear();
    /// assert_eq(m.any(), false);
    /// ```
    constexpr bool any() const {
        for (const auto& row : m_rows)
            if (row.any()) return true;
        return false;
    }

    /// Returns `true` if all elements of the bit-matrix are set.
    ///
    /// Empty matrices are considered to have all bits set.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// assert_eq(m.all(), false);
    /// m.set_all();
    /// assert_eq(m.all(), true);
    /// m.clear();
    /// assert_eq(m.all(), true);
    /// ```
    constexpr bool all() const {
        for (const auto& row : m_rows)
            if (!row.all()) return false;
        return true;
    }

    /// Returns `true` if no elements of the bit-matrix are set.
    ///
    /// Empty matrices are considered to have no set bits.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// assert_eq(m.none(), true);
    /// m.set_all();
    /// assert_eq(m.none(), false);
    /// m.clear();
    /// assert_eq(m.none(), true);
    /// ```
    constexpr bool none() const {
        for (const auto& row : m_rows)
            if (!row.none()) return false;
        return true;
    }

    /// @}
    /// @name Individual Element Access
    /// @{

    /// Returns `true` if the element at row `r` and column `c` is set.
    ///
    /// @note In debug mode, this method will panic if `r` or `c` is out of bounds.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// assert_eq(m.get(0, 0), false);
    /// m.set(0, 0);
    /// assert_eq(m.get(0, 0), true);
    /// ```
    constexpr bool get(usize r, usize c) const {
        gf2_debug_assert(r < rows(), "Row index {} out of bounds [0,{})", r, rows());
        gf2_debug_assert(c < cols(), "Column index {} out of bounds [0,{})", c, cols());
        return m_rows[r].get(c);
    }

    /// Returns the value of the bit at row `r` and column `c` as a `bool`.
    ///
    /// @note In debug mode, this method will panic if `r` or `c` is out of bounds.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// assert_eq(m(0, 0), false);
    /// m.set(0, 0);
    /// assert_eq(m(0, 0), true);
    /// ```
    constexpr bool operator()(usize r, usize c) const {
        gf2_debug_assert(r < rows(), "Row index {} out of bounds [0,{})", r, rows());
        gf2_debug_assert(c < cols(), "Column index {} out of bounds [0,{})", c, cols());
        return m_rows[r].get(c);
    }

    /// Sets the bit at row `r` and column `c` to the bool value `val`.
    /// The default is to set the bit to `true`.
    ///
    /// @note In debug mode, this method will panic if `r` or `c` is out of bounds.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// m.set(0, 0);
    /// assert_eq(m.get(0, 0), true);
    /// ```
    constexpr void set(usize r, usize c, bool val = true) {
        gf2_debug_assert(r < rows(), "Row index {} out of bounds [0,{})", r, rows());
        gf2_debug_assert(c < cols(), "Column index {} out of bounds [0,{})", c, cols());
        m_rows[r].set(c, val);
    }

    /// Returns the bit at row `r` and column `c` as a `BitRef` reference which can be used to set the bit.
    ///
    /// @note In debug mode, this method will panic if `r` or `c` is out of bounds.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// assert_eq(m(0,0), false);
    /// m(0,0) = true;
    /// assert_eq(m(0,0), true);
    /// ```
    constexpr BitRef<row_type> operator()(usize r, usize c) {
        gf2_debug_assert(r < rows(), "Row index {} out of bounds [0,{})", r, rows());
        gf2_debug_assert(c < cols(), "Column index {} out of bounds [0,{})", c, cols());
        return m_rows[r][c];
    }

    /// Flips the bit at row `r` and column `c`.
    ///
    /// @note In debug mode, this method will panic if `r` or `c` is out of bounds.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// m.flip(0, 0);
    /// assert_eq(m.get(0, 0), true);
    /// ```
    constexpr void flip(usize r, usize c) {
        gf2_debug_assert(r < rows(), "Row index {} out of bounds [0,{})", r, rows());
        gf2_debug_assert(c < cols(), "Column index {} out of bounds [0,{})", c, cols());
        m_rows[r].flip(c);
    }

    /// @}
    /// @name Row Access
    /// @{

    /// Returns a read-only reference to the row at index `r`.
    ///
    /// @note In debug mode, this method will panic if `r` is out of bounds.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// assert_eq(m.row(0).to_binary_string(), "100");
    /// assert_eq(m.row(1).to_binary_string(), "010");
    /// assert_eq(m.row(2).to_binary_string(), "001");
    /// ```
    constexpr const row_type& row(usize r) const {
        gf2_debug_assert(r < rows(), "Row index {} out of bounds [0,{})", r, rows());
        return m_rows[r];
    }

    /// Returns a read-write reference to the row at index `r`.
    ///
    /// @note In debug mode, this method will panic if `r` is out of bounds.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// m.row(0).set(1);
    /// assert_eq(m.row(0).to_binary_string(), "110");
    /// assert_eq(m.row(1).to_binary_string(), "010");
    /// assert_eq(m.row(2).to_binary_string(), "001");
    /// ```
    constexpr row_type& row(usize r) {
        gf2_debug_assert(r < rows(), "Row index {} out of bounds [0,{})", r, rows());
        return m_rows[r];
    }

    /// Returns a read-only reference to the row at index `r`.
    ///
    /// @note In debug mode, this method will panic if `r` is out of bounds.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// assert_eq(m[0].to_binary_string(), "100");
    /// assert_eq(m[1].to_binary_string(), "010");
    /// assert_eq(m[2].to_binary_string(), "001");
    /// ```
    constexpr const row_type& operator[](usize r) const {
        gf2_debug_assert(r < rows(), "Row index {} out of bounds [0,{})", r, rows());
        return m_rows[r];
    }

    /// Returns a read-write reference to the row at index `r`.
    ///
    /// @note In debug mode, this method will panic if `r` is out of bounds.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// m[0].set(1);
    /// assert_eq(m[0].to_binary_string(), "110");
    /// assert_eq(m[1].to_binary_string(), "010");
    /// assert_eq(m[2].to_binary_string(), "001");
    /// ```
    constexpr row_type& operator[](usize r) {
        gf2_debug_assert(r < rows(), "Row index {} out of bounds [0,{})", r, rows());
        return m_rows[r];
    }

    /// @}
    /// @name Column Access
    /// @{

    /// Returns a **clone** of the elements in column `c` from the bit-matrix as an independent bit-vector.
    ///
    /// Matrices are stored by rows and there is no cheap reference style access to the BitMatrix columns!
    ///
    /// @note In debug mode, this method will panic if `c` is out of bounds.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// auto col = m.col(1);
    /// assert_eq(col.to_string(), "010");
    /// col.set(0);
    /// col.set(2);
    /// assert_eq(col.to_string(), "111");
    /// assert_eq(m.to_compact_binary_string(), "100 010 001");
    /// ```
    constexpr BitVector<Word> col(usize c) const {
        gf2_debug_assert(c < cols(), "Column {} out of bounds [0, {})", c, cols());
        auto result = BitVector<Word>::zeros(rows());
        for (auto r = 0uz; r < rows(); ++r) {
            if (get(r, c)) { result.set(r); }
        }
        return result;
    }

    /// @}
    /// @name Whole Matrix Mutators
    /// @{

    /// Sets all the elements of the bit-matrix to the specified boolean `value`.
    ///
    /// By default, all bits are set to `true`.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// m.set_all();
    /// assert_eq(m.to_compact_binary_string(), "111 111 111");
    /// ```
    constexpr void set_all(bool value = true) {
        for (auto& row : m_rows) row.set_all(value);
    }

    /// Flips all the elements of the bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// m.flip_all();
    /// assert_eq(m.to_compact_binary_string(), "111 111 111");
    /// ```
    constexpr void flip_all() {
        for (auto& row : m_rows) row.flip_all();
    }

    /// @}
    /// @name Diagonal Mutators
    /// @{

    /// Sets the main diagonal of a square bit-matrix to the boolean value `val`.
    ///
    /// @note In debug mode, this method will panic if the bit-matrix is not square.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// m.set_diagonal();
    /// assert_eq(m.to_compact_binary_string(), "100 010 001");
    /// ```
    constexpr void set_diagonal(bool val = true) {
        gf2_debug_assert(is_square(), "BitMatrix is {} x {} but it should be square!", rows(), cols());
        for (auto i = 0uz; i < rows(); ++i) { set(i, i, val); }
    }

    /// Flips all the elements on the main diagonal of a square bit-matrix.
    ///
    /// @note In debug mode, this method will panic if the bit-matrix is not square.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// m.flip_diagonal();
    /// assert_eq(m.to_compact_binary_string(), "100 010 001");
    /// ```
    constexpr void flip_diagonal() {
        gf2_debug_assert(is_square(), "BitMatrix is {} x {} but it should be square!", rows(), cols());
        for (auto i = 0uz; i < rows(); ++i) { flip(i, i); }
    }

    /// Sets the elements on super-diagonal `d` of a square bit-matrix to the boolean value `val`.
    ///
    /// Here `d = 0` is the main diagonal and `d = 1` is the first super-diagonal etc.
    ///
    /// @note In debug mode, this method will panic if the bit-matrix is not square.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(5);
    /// m.set_super_diagonal(1);
    /// assert_eq(m.to_compact_binary_string(), "01000 00100 00010 00001 00000");
    /// ```
    constexpr void set_super_diagonal(usize d, bool val = true) {
        gf2_debug_assert(is_square(), "BitMatrix is {} x {} but it should be square!", rows(), cols());
        for (auto i = 0uz; i < rows() - d; ++i) { set(i, i + d, val); }
    }

    /// Flips all the elements on the super-diagonal `d` of a square bit-matrix.
    ///
    /// Here `d = 0` is the main diagonal and `d = 1` is the first super-diagonal etc.
    ///
    /// @note In debug mode, this method will panic if the bit-matrix is not square.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(5);
    /// m.flip_super_diagonal(1);
    /// assert_eq(m.to_compact_binary_string(), "01000 00100 00010 00001 00000");
    /// ```
    constexpr void flip_super_diagonal(usize d) {
        gf2_debug_assert(is_square(), "BitMatrix is {} x {} but it should be square!", rows(), cols());
        for (auto i = 0uz; i < rows() - d; ++i) { flip(i, i + d); }
    }

    /// Sets the elements on sub-diagonal `d` of a square bit-matrix to the boolean value `val`.
    ///
    /// Here `d = 0` is the main diagonal and `d = 1` is the first sub-diagonal etc.
    ///
    /// @note In debug mode, this method will panic if the bit-matrix is not square.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(5);
    /// m.set_sub_diagonal(1);
    /// assert_eq(m.to_compact_binary_string(), "00000 10000 01000 00100 00010");
    /// ```
    constexpr void set_sub_diagonal(usize d, bool val = true) {
        gf2_debug_assert(is_square(), "BitMatrix is {} x {} but it should be square!", rows(), cols());
        for (auto i = 0uz; i < rows() - d; ++i) { set(i + d, i, val); }
    }

    /// Flips all the elements on the sub-diagonal `d` of a square bit-matrix.
    ///
    /// Here `d = 0` is the main diagonal and `d = 1` is the first sub-diagonal etc.
    ///
    /// @note In debug mode, this method will panic if the bit-matrix is not square.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(5);
    /// m.flip_sub_diagonal(1);
    /// assert_eq(m.to_compact_binary_string(), "00000 10000 01000 00100 00010");
    /// ```
    constexpr void flip_sub_diagonal(usize d) {
        gf2_debug_assert(is_square(), "BitMatrix is {} x {} but it should be square!", rows(), cols());
        for (auto i = 0uz; i < rows() - d; ++i) { flip(i + d, i); }
    }

    /// @}
    /// @name Resizing
    /// @{

    /// Resize the bit-matrix to `r` rows and `c` columns.
    ///
    /// @param r The new number of rows -- if r < rows() then we lose the excess rows.
    /// @param c The new number of columns -- if c < cols() then we lose the excess columns.
    ///
    /// Any added elements are set to 0.
    ///
    /// # Example
    /// ```
    /// BitMatrix m;
    /// m.resize(10, 10);
    /// assert_eq(m.rows(), 10);
    /// assert_eq(m.cols(), 10);
    /// m.resize(3, 7);
    /// assert_eq(m.rows(), 3);
    /// assert_eq(m.cols(), 7);
    /// m.resize(0, 10);
    /// assert_eq(m.rows(), 0);
    /// assert_eq(m.cols(), 0);
    /// ```
    constexpr void resize(usize r, usize c) {
        // Edge case ...
        if (rows() == r && cols() == c) return;

        // Resizes to zero in either dimension is taken as a zap the lot instruction
        if (r == 0 || c == 0) r = c = 0;

        // Rows, then columns
        m_rows.resize(r);
        for (auto& row : m_rows) row.resize(c);
    }

    /// Removes all the elements from the bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// m.clear();
    /// assert_eq(m.rows(), 0);
    /// assert_eq(m.cols(), 0);
    /// ```
    constexpr void clear() { resize(0, 0); }

    /// Makes an arbitrary rectangular bit-matrix into a square `BitMatrix`.
    ///
    /// Existing elements are preserved. Any added elements are initialized to zero.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::from_string("111 111 111 111").value();
    /// m.make_square(3);
    /// assert_eq(m.to_compact_binary_string(), "111 111 111");
    /// ```
    constexpr void make_square(usize n) { resize(n, n); }

    /// @}
    /// @name Appending Rows and Columns
    /// @{

    /// Appends a single row onto the end of the bit-matrix by copying it.
    ///
    /// Returns a reference to the current object for chaining.
    /// @note The row must have the same number of elements as the bit-matrix has columns.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// auto row = BitVector<>::ones(3);
    /// m.append_row(row);
    /// assert_eq(m.to_compact_binary_string(), "000 000 000 111");
    /// ```
    template<BitStore Store>
        requires std::same_as<typename Store::word_type, Word>
    constexpr BitMatrix& append_row(Store const& row) {
        gf2_assert_eq(row.size(), cols(), "Row has {} elements but bit-matrix has {} columns!", row.size(), cols());
        m_rows.push_back(row);
        return *this;
    }

    /// Appends a single row onto the end of the bit-matrix by moving it.
    ///
    /// Use `std::move(row)` in the method argument to guarantee this version. The input row is moved into the
    /// matrix and is no longer valid after this call.
    ///
    /// Returns a reference to the current object for chaining.
    /// @note The row must have the same number of elements as the bit-matrix has columns.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// m.append_row(BitVector<>::ones(3));
    /// assert_eq(m.to_compact_binary_string(), "000 000 000 111");
    /// ```
    template<BitStore Store>
        requires std::same_as<typename Store::word_type, Word>
    constexpr BitMatrix& append_row(Store&& row) {
        gf2_assert_eq(row.size(), cols(), "Row has {} elements but bit-matrix has {} columns!", row.size(), cols());
        m_rows.push_back(std::move(row));
        return *this;
    }

    /// Appends all the rows from the `src` bit-matrix onto the end of this bit-matrix by copying them.
    ///
    /// Returns a reference to the current object for chaining.
    /// @note The source bit-matrix must have the same number of columns as this bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// auto src = BitMatrix<>::ones(3, 3);
    /// m.append_rows(src);
    /// assert_eq(m.to_compact_binary_string(), "000 000 000 111 111 111");
    /// ```
    constexpr BitMatrix& append_rows(BitMatrix<Word> const& src) {
        gf2_assert_eq(src.cols(), cols(), "Source has {} columns but bit-matrix has {} columns!", src.cols(), cols());
        m_rows.insert(m_rows.end(), src.m_rows.begin(), src.m_rows.end());
        return *this;
    }

    /// Appends all the rows from the `src` bit-matrix onto the end of this bit-matrix by moving them.
    ///
    /// Use `std::move(src)` in the method argument to guarantee this version. The source bit-matrix is moved into
    /// this matrix and is no longer valid after this call.
    ///
    /// Returns a reference to the current object for chaining.
    /// @note The source bit-matrix must have the same number of columns as this bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// m.append_rows(BitMatrix<>::ones(3, 3));
    /// assert_eq(m.to_compact_binary_string(), "000 000 000 111 111 111");
    /// ```
    constexpr BitMatrix& append_rows(BitMatrix<Word>&& src) {
        gf2_assert_eq(src.cols(), cols(), "Source has {} columns but bit-matrix has {} columns!", src.cols(), cols());
        m_rows.insert(m_rows.end(), std::make_move_iterator(src.m_rows.begin()),
                      std::make_move_iterator(src.m_rows.end()));
        return *this;
    }

    /// Appends a single column `col` onto the right of the bit-matrix so `M` -> `M|col`.
    ///
    /// Returns a reference to the current object for chaining.
    /// @note The column must have the same number of elements as the bit-matrix has rows.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// auto col = BitVector<>::ones(3);
    /// m.append_col(col);
    /// assert_eq(m.to_compact_binary_string(), "0001 0001 0001");
    /// ```
    template<BitStore Store>
        requires std::same_as<typename Store::word_type, Word>
    constexpr BitMatrix& append_col(Store const& col) {
        gf2_assert_eq(col.size(), rows(), "Column has {} elements but bit-matrix has {} rows!", col.size(), rows());
        for (auto i = 0uz; i < rows(); ++i) m_rows[i].push(col.get(i));
        return *this;
    }

    /// Appends all the columns from the `src` bit-matrix onto the right of this bit-matrix so `M` -> `M|src`.
    ///
    /// Returns a reference to the current object for chaining.
    /// @note The source bit-matrix must have the same number of rows as this bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// auto src = BitMatrix<>::ones(3, 2);
    /// m.append_cols(src);
    /// assert_eq(m.to_compact_binary_string(), "00011 00011 00011");
    /// ```
    constexpr BitMatrix& append_cols(BitMatrix<Word> const& src) {
        gf2_assert_eq(src.rows(), rows(), "Source has {} rows but bit-matrix has {} rows!", src.rows(), rows());
        for (auto i = 0uz; i < rows(); ++i) m_rows[i].append(src.m_rows[i]);
        return *this;
    }

    /// @}
    /// @name Removing Rows and Columns
    /// @{

    /// Removes a row off the end of the bit-matrix and returns it or `std::nullopt` if the bit-matrix is empty.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::ones(3, 3);
    /// auto row = m.remove_row();
    /// assert_eq(row->to_string(), "111");
    /// assert_eq(m.to_compact_binary_string(), "111 111");
    /// ```
    constexpr std::optional<BitVector<Word>> remove_row() {
        if (rows() == 0) return std::nullopt;
        auto row = std::move(m_rows.back());
        m_rows.pop_back();
        return row;
    }

    /// Removes `k` rows off the end of the bit-matrix and returns them as a new bit-matrix or `std::nullopt` if the
    /// bit-matrix has fewer than `k` rows.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::ones(3, 3);
    /// auto popped = m.remove_rows(2);
    /// assert_eq(popped->to_compact_binary_string(), "111 111");
    /// assert_eq(m.to_compact_binary_string(), "111");
    /// ```
    constexpr std::optional<BitMatrix<Word>> remove_rows(usize k) {
        if (rows() < k) return std::nullopt;
        auto begin = m_rows.end() - static_cast<std::ptrdiff_t>(k);
        auto end = m_rows.end();
        auto result = BitMatrix<Word>(std::vector<row_type>(begin, end));
        m_rows.erase(begin, end);
        return result;
    }

    /// Removes a column off the right of the bit-matrix and returns it or `std::nullopt` if the bit-matrix is
    /// empty.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::ones(3, 3);
    /// auto col = m.remove_col();
    /// assert_eq(col->to_string(), "111");
    /// assert_eq(m.to_compact_binary_string(), "11 11 11");
    /// ```
    constexpr std::optional<BitVector<Word>> remove_col() {
        if (cols() == 0) return std::nullopt;
        auto result = col(cols() - 1);
        for (auto i = 0uz; i < rows(); ++i) m_rows[i].pop();
        return result;
    }

    /// @}
    /// @name Sub-matrices
    /// @{

    /// Returns an independent *clone* of the sub-matrix delimited by the given row and column ranges.
    ///
    /// The ranges are half open `[r_start, r_end) X [c_start, c_end)` where `r` is for rows and `c` for columns.
    ///
    /// @note Panics if the bit-matrix has incompatible dimensions with the requested sub-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(5);
    /// auto sub1 = m.sub_matrix(1, 4, 1, 4);
    /// assert_eq(sub1.to_compact_binary_string(), "100 010 001");
    /// auto sub2 = m.sub_matrix(1, 1, 1, 1);
    /// assert_eq(sub2.to_compact_binary_string(), "");
    /// ```
    constexpr BitMatrix sub_matrix(usize r_start, usize r_end, usize c_start, usize c_end) const {

        // Check that the row range is valid.
        gf2_assert(r_start <= r_end, "Invalid row range");
        gf2_assert(r_end <= rows(), "Row range extends beyond the end of the bit-matrix");

        // Check that the column range is valid.
        gf2_assert(c_start <= c_end, "Invalid column range");
        gf2_assert(c_end <= cols(), "Column range extends beyond the right edge of the bit-matrix");

        // Get the number of rows and columns in the sub-matrix.
        auto r = r_end - r_start;
        auto c = c_end - c_start;

        // Create the sub-matrix.
        BitMatrix result{r, c};
        for (auto i = 0uz; i < r; ++i) result.m_rows[i].copy(m_rows[i + r_start].span(c_start, c_end));

        return result;
    }

    /// Replaces the sub-matrix starting at row `top` and column `left` with a copy of the sub-matrix `src`.
    ///
    /// @note Panics if `src` does not fit within this bit-matrix starting at row `top` and column `left`.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(5);
    /// m.replace_sub_matrix(1, 1, BitMatrix<>::ones(3, 3));
    /// assert_eq(m.to_compact_binary_string(), "10000 01110 01110 01110 00001");
    /// ```
    constexpr void replace_sub_matrix(usize top, usize left, BitMatrix<Word> const& src) {
        auto r = src.rows();
        auto c = src.cols();
        gf2_assert(top + r <= rows(), "Too many rows for the replacement sub-matrix to fit");
        gf2_assert(left + c <= cols(), "Too many columns for the replacement sub-matrix to fit");
        for (auto i = 0uz; i < r; ++i) m_rows[top + i].span(left, left + c).copy(src.m_rows[i]);
    }

    /// @}
    /// @name Triangular Sub-Matrices
    /// @{

    /// Returns an independent *clone* of the lower triangular part of the bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::ones(3, 3);
    /// auto sub_m = m.lower();
    /// assert_eq(sub_m.to_compact_binary_string(), "100 110 111");
    /// ```
    constexpr BitMatrix lower() const {
        // Edge case:
        if (is_empty()) return BitMatrix{};

        // Start with a copy of the bit-matrix.
        BitMatrix result = *this;

        // Set the upper triangular part to zero.
        auto nc = cols();
        for (auto i = 0uz; i < rows(); ++i) {
            auto first = i + 1;
            if (first < nc) result.m_rows[i].span(first, nc).set_all(false);
        }
        return result;
    }

    /// Returns an independent *clone* of the upper triangular part of the bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::ones(3, 3);
    /// auto sub_m = m.upper();
    /// assert_eq(sub_m.to_compact_binary_string(), "111 011 001");
    /// ```
    constexpr BitMatrix upper() const {
        // Edge case:
        if (is_empty()) return BitMatrix{};

        // Start with a copy of the bit-matrix.
        auto result = *this;

        // Set the upper triangular part to zero.
        auto c = cols();
        for (auto i = 0uz; i < rows(); ++i) {
            auto len = std::min(i, c);
            if (len > 0) result.m_rows[i].span(0, len).set_all(false);
        }
        return result;
    }

    /// Returns an independent *clone* of the strictly lower triangular part of the bit-matrix.
    ///
    /// This is the same as `lower()` but with the diagonal reset to zero.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::ones(3, 3);
    /// auto sub_m = m.strictly_lower();
    /// assert_eq(sub_m.to_compact_binary_string(), "000 100 110");
    /// ```
    constexpr BitMatrix strictly_lower() const {
        auto result = lower();
        result.set_diagonal(false);
        return result;
    }

    /// Returns an independent *clone* of the strictly upper triangular part of the bit-matrix.
    ///
    /// This is the same as `lower()` but with the diagonal reset to zero.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::ones(3, 3);
    /// auto sub_m = m.strictly_upper();
    /// assert_eq(sub_m.to_compact_binary_string(), "011 001 000");
    /// ```
    constexpr BitMatrix strictly_upper() const {
        auto result = upper();
        result.set_diagonal(false);
        return result;
    }

    /// Returns an independent *clone* of the unit lower triangular part of the bit-matrix.
    ///
    /// This is the same as `lower()` but with the diagonal set to one.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zeros(3, 3);
    /// auto sub_m = m.unit_lower();
    /// assert_eq(sub_m.to_compact_binary_string(), "100 010 001");
    /// ```
    constexpr BitMatrix unit_lower() const {
        auto result = lower();
        result.set_diagonal(true);
        return result;
    }

    /// Returns an independent *clone* of the unit upper triangular part of the bit-matrix.
    ///
    /// This is the same as `upper()` but with the diagonal set to one.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zeros(3, 3);
    /// auto sub_m = m.unit_upper();
    /// assert_eq(sub_m.to_compact_binary_string(), "100 010 001");
    /// ```
    constexpr BitMatrix unit_upper() const {
        auto result = upper();
        result.set_diagonal(true);
        return result;
    }

    /// @}
    /// @name Elementary Row and Column Operations
    /// @{

    /// Swaps rows `i` and `j` of the bit-matrix.
    //
    /// @note In debug mode, this method will panic if either of the indices is out of bounds.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// m.swap_rows(0, 1);
    /// assert_eq(m.to_compact_binary_string(), "010 100 001");
    /// ```
    constexpr void swap_rows(usize i, usize j) {
        gf2_debug_assert(i < rows(), "Row index {} out of bounds [0,{})", i, rows());
        gf2_debug_assert(j < rows(), "Row index {} out of bounds [0,{})", j, rows());
        std::swap(m_rows[i], m_rows[j]);
    }

    /// Swaps columns `i` and `j` of the bit-matrix.
    ///
    /// @note In debug mode, this method will panic if either of the indices is out of bounds.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// m.swap_cols(0, 1);
    /// assert_eq(m.to_compact_binary_string(), "010 100 001");
    /// ```
    constexpr void swap_cols(usize i, usize j) {
        gf2_debug_assert(i < cols(), "Column index {} out of bounds [0,{})", i, cols());
        gf2_debug_assert(j < cols(), "Column index {} out of bounds [0,{})", j, cols());
        for (auto k = 0uz; k < rows(); ++k) m_rows[k].swap(i, j);
    }

    /// Adds the identity bit-matrix to the bit-matrix.
    ///
    /// @note In debug mode, this panics if the bit-matrix is not square.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// m.add_identity();
    /// assert_eq(m.to_compact_binary_string(), "100 010 001");
    /// ```
    constexpr void add_identity() {
        gf2_debug_assert(is_square(), "This bit-matrix is {} x {} but it should be square!", rows(), cols());
        for (auto i = 0uz; i < rows(); ++i) flip(i, i);
    }

    /// @}
    /// @name Transposing
    /// @{

    /// Returns a new bit-matrix that is the transpose of this one.
    ///
    /// @note This method isn't particularly efficient as it works by iterating over all elements of the bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zeros(3, 2);
    /// m.row(0).set_all();
    /// assert_eq(m.to_compact_binary_string(), "11 00 00");
    /// auto n = m.transposed();
    /// assert_eq(n.to_compact_binary_string(), "100 100");
    /// ```
    constexpr BitMatrix transposed() const {
        auto      r = rows();
        auto      c = cols();
        BitMatrix result{c, r};
        for (auto i = 0uz; i < r; ++i) {
            for (auto j = 0uz; j < c; ++j) {
                if (get(i, j)) { result.set(j, i); }
            }
        }
        return result;
    }

    /// Transposes a *square* bit-matrix in place.
    ///
    /// @note This method panics if the bit-matrix is not square.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::zero(3);
    /// m.row(0).set_all();
    /// assert_eq(m.to_compact_binary_string(), "111 000 000");
    /// m.transpose();
    /// assert_eq(m.to_compact_binary_string(), "100 100 100");
    /// ```
    constexpr void transpose() {
        gf2_assert(is_square(), "`transpose()` requires a square matrix");
        for (auto i = 0uz; i < rows(); ++i) {
            for (auto j = 0uz; j < i; ++j) {
                if (get(i, j) != get(j, i)) {
                    flip(i, j);
                    flip(j, i);
                }
            }
        }
    }

    /// @}
    /// @name Exponentiation
    /// @{

    /// Returns a new bit-matrix that is this one raised to some power `n` or `2^n`.
    ///
    /// We efficiently compute `M^e` by using a square and multiply algorithm where by default `e = n`.
    /// If the second argument `n_is_log2 = true` then we consider `e = 2^n` instead.
    ///
    /// @note This method panics if the bit-matrix is not square.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::random(100, 100);
    /// auto p1 = m.to_the(3);
    /// auto o1 = m * m * m;
    /// assert_eq(p1, o1);
    /// auto p2 = m.to_the(2, true);
    /// auto o2 = m * o1;
    /// assert_eq(p2, o2);
    /// ```
    constexpr BitMatrix to_the(usize n, bool n_is_log2 = false) const {
        gf2_assert(is_square(), "Bit-matrix is {} x {} but it should be square!", rows(), cols());

        // Perhaps we just need lots of square steps?  Note that 2^0 = 1 so M^(2^0) = M.
        if (n_is_log2) {
            auto result = *this;
            for (auto i = 0uz; i < n; ++i) result = dot(result, result);
            return result;
        }

        // Otherwise we need square & multiply steps but we first handle the edge case:
        if (n == 0) return BitMatrix::identity(rows());

        // OK n != 0: Note that if e.g. n = 0b00010111 then std::bit_floor(n) = 0b00010000.
        usize n_bit = std::bit_floor(n);

        // Start with a copy of this bit-matrix which takes care of the most significant binary digit in n.
        auto result = *this;
        n_bit >>= 1;

        // More to go?
        while (n_bit) {
            // Always square and then multiply if necessary (i.e. if current bit in n is set).
            result = dot(result, result);
            if (n & n_bit) result = dot(result, *this);

            // On to the next bit position in n.
            n_bit >>= 1;
        }
        return result;
    }

    /// @}
    /// @name Row-echelon forms
    /// @{

    /// Transforms an arbitrary shaped, non-empty, bit-matrix to row-echelon form (in-place).
    ///
    /// The method returns a bit-vector that shows which columns have a "pivot" (a non-zero on or below the diagonal).
    /// The matrix *rank* is the number of set bits in that pivot bit-vector.
    ///
    /// A bit-matrix is in echelon form if the first 1 in any row is to the right of the first 1 in the preceding row.
    /// It is a generalization of an upper triangular form -- the result is a matrix with a "staircase" shape.
    ///
    /// The transformation is Gaussian elimination.
    /// Any all zero rows are moved to the bottom of the matrix.
    /// The echelon form is not unique.
    ///
    /// @note This method panics if the bit-matrix is empty.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// m.set(2, 1, false);
    /// auto has_pivot = m.to_echelon_form();
    /// assert_eq(has_pivot.to_string(), "111");
    /// assert_eq(m.to_compact_binary_string(), "100 010 001");
    /// ```
    BitVector<Word> to_echelon_form() {
        gf2_assert(!is_empty(), "Bit-matrix must not be empty");

        // We return a bit-vector that shows which columns have a pivot -- start by assuming none.
        auto has_pivot = BitVector<Word>::zeros(cols());

        // The current row of the echelon form we are working on.
        auto r = 0uz;
        auto nr = rows();

        // Iterate over each column.
        for (auto j = 0uz; j < cols(); ++j) {
            // Find a non-zero entry in this column below the diagonal (a "pivot").
            auto p = r;
            while (p < nr && !get(p, j)) p++;

            // Did we find a pivot in this column?
            if (p < nr) {
                // Mark this column as having a pivot.
                has_pivot.set(j);

                // If necessary, swap the current row with the row that has the pivot.
                if (p != r) swap_rows(p, r);

                // Below the working row make sure column j is zero by elimination if necessary.
                auto row_r = m_rows[r];
                for (auto i = r + 1; i < nr; ++i)
                    if (get(i, j)) m_rows[i] ^= row_r;

                // Move to the next row. If we've reached the end of the matrix, we're done.
                r += 1;
                if (r == nr) break;
            }
        }

        // Return the bit-vector that shows which columns have a pivot.
        return has_pivot;
    }

    /// Transforms the bit-matrix to reduced row-echelon form (in-place).
    ///
    /// The method returns a bit-vector that shows which columns have a "pivot" (a non-zero on or below the diagonal).
    /// The matrix *rank* is the number of set bits in that bit-vector.
    ///
    /// A bit-matrix is in reduced echelon form if it is in echelon form with at most one 1 in each column.
    /// The reduced echelon form is unique.
    ///
    /// @note This method panics if the bit-matrix is empty.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// m.set(2, 1, false);
    /// auto pivots = m.to_reduced_echelon_form();
    /// assert_eq(pivots.to_string(), "111");
    /// assert_eq(m.to_compact_binary_string(), "100 010 001");
    /// ```
    BitVector<Word> to_reduced_echelon_form() {
        // Start with the echelon form.
        auto has_pivot = to_echelon_form();

        // Iterate over each row from the bottom up - Gauss Jordan elimination.
        for (auto r = rows(); r--;) {
            // Find the first set bit in the current row if there is one.
            if (auto p = m_rows[r].first_set()) {
                // Clear out everything in column p *above* row r (already cleared out below the pivot).
                auto row_r = m_rows[r];
                for (auto i = 0uz; i < r; ++i)
                    if (get(i, *p)) m_rows[i] ^= row_r;
            }
        }

        // Return the bit-vector that shows which columns have a pivot.
        return has_pivot;
    }

    /// @}
    /// @name Inversion
    /// @{

    /// Returns the inverse of a square bit-matrix or `std::nullopt` if the matrix is singular.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// assert_eq(m.inverse().value().to_compact_binary_string(), "100 010 001");
    /// ```
    std::optional<BitMatrix> inverse() const {
        // The bit-matrix must be square & non-empty.
        if (is_empty() || !is_square()) return std::nullopt;

        // Create a copy of the bit-matrix & augment it with the identity matrix on the right.
        auto matrix = *this;
        auto nr = rows();
        auto nc = cols();
        matrix.append_cols(BitMatrix::identity(nr));

        // Transform the augmented matrix to reduced row-echelon form.
        matrix.to_reduced_echelon_form();

        // If all went well the left half is the identity matrix and the right half is the inverse.
        if (matrix.sub_matrix(0, nr, 0, nc).is_identity()) return matrix.sub_matrix(0, nr, nc, nc * 2);

        return std::nullopt;
    }

    /// Class method that returns the probability that a square `n x n` bit-matrix is invertible if each element
    /// is chosen independently and uniformly at random by flips of a fair coin.
    ///
    /// For large `n`, the value is roughly 29% and that holds for n as low as 10.
    ///
    /// @note This method panics if `n` is 0 -- based on the assumption that querying the probability of a 0 x 0
    /// bit-matrix being invertible is an upstream error somewhere.
    ///
    /// # Example
    /// ```
    /// auto p = BitMatrix<>::probability_invertible(10);
    /// assert(abs(p - 0.289) < 1e-3);
    /// ```
    static constexpr double probability_invertible(usize n) {
        // A 0x0 matrix is not well defined throw an error!
        if (n == 0) throw std::invalid_argument("Matrix should not be 0x0--likely an error somewhere upstream!");

        // Formula is p(n) = \prod_{k = 1}^{n} (1 - 2^{-k}) which runs out of juice once n hits any size at all!
        usize n_prod = std::min(n, static_cast<size_t>(std::numeric_limits<double>::digits));

        // Compute the product over the range that matters
        double product = 1;
        double pow2 = 1;
        while (n_prod--) {
            pow2 *= 0.5;
            product *= 1 - pow2;
        }
        return product;
    }

    /// Class method that returns the probability that a square `n x n` bit-matrix is singular if each element
    /// is chosen independently and uniformly at random by flips of a fair coin.
    ///
    /// For large `n`, the value is 71% and that holds for n as low as 10.
    ///
    /// @note This method panics if `n` is 0 -- based on the assumption that querying the probability of a 0 x 0
    /// bit-matrix being invertible is an upstream error somewhere.
    ///
    /// # Example
    /// ```
    /// auto p = BitMatrix<>::probability_singular(10);
    /// assert(abs(p - 0.711) < 1e-3);
    /// ```
    static constexpr double probability_singular(usize n) { return 1 - probability_invertible(n); }

    /// @}
    /// @name LU Decomposition
    /// @{

    /// Returns the LU decomposition of the bit-matrix.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// auto lu = m.LU();
    /// assert_eq(lu.LU().to_compact_binary_string(), "100 010 001");
    /// ```
    BitLU<Word> LU() const { return BitLU<Word>{*this}; }

    /// @}
    /// @name Solving systems of linear equations
    /// @{

    /// Returns the Gaussian elimination solver for this bit-matrix and the passed r.h.s. vector `b`.
    ///
    /// # Example
    /// ```
    /// auto A = BitMatrix<>::ones(3, 3);
    /// auto b = BitVector<>::ones(3);
    /// auto solver = A.solver_for(b);
    /// assert_eq(solver.rank(), 1);
    /// assert_eq(solver.free_count(), 2);
    /// assert_eq(solver.solution_count(), 4);
    /// assert_eq(solver.is_underdetermined(), true);
    /// assert_eq(solver.is_consistent(), true);
    /// ```
    template<BitStore Rhs>
        requires std::same_as<typename Rhs::word_type, Word>
    auto solver_for(Rhs const& b) const {
        return BitGauss<Word>{*this, b};
    }

    /// Returns a solution to the system of linear equations `A.x_ = b` or `std::nullopt` if there are none.
    ///
    /// # Example
    /// ```
    /// auto A = BitMatrix<>::identity(3);
    /// auto b = BitVector<>::ones(3);
    /// auto x = A.x_for(b).value();
    /// assert_eq(x.to_string(), "111");
    /// ```
    template<BitStore Rhs>
        requires std::same_as<typename Rhs::word_type, Word>
    auto x_for(Rhs const& b) const {
        return solver_for(b)();
    }

    /// @}
    /// @name Characteristic Polynomial
    /// @{

    /// Returns the characteristic polynomial of any square bit-matrix as a `gf2::BitPolynomial`.
    ///
    /// The method uses similarity transformations to convert the bit-matrix to *Frobenius form* which has a readily
    /// computable characteristic polynomial. Similarity transformations preserve eigen-structure, and in particular
    /// they preserve the characteristic polynomial.
    ///
    /// @note This method panics if the bit-matrix is not square.
    ///
    /// # Example
    /// ```
    /// auto m2 = BitMatrix<>::identity(2);
    /// assert_eq(m2.characteristic_polynomial().to_string(), "1 + x^2");
    /// auto m3 = BitMatrix<>::identity(3);
    /// assert_eq(m3.characteristic_polynomial().to_string(), "1 + x + x^2 + x^3");
    /// auto m100 = BitMatrix<>::random(100, 100);
    /// auto p = m100.characteristic_polynomial();
    /// assert_eq(p(m100).is_zero(), true);
    /// ```
    BitPolynomial<Word> characteristic_polynomial() const {
        gf2_assert(is_square(), "Bit-matrix must be square not {} x {}", rows(), cols());
        return frobenius_matrix_characteristic_polynomial(frobenius_form());
    }

    /// Class method that returns the characteristic polynomial of a *Frobenius matrix* as a `gf2::BitPolynomial`.
    ///
    /// A Frobenius matrix is a square matrix that consists of blocks of *companion matrices* along the diagonal.
    /// Each companion matrix is a square matrix that is all zeros except for an arbitrary top row and a principal
    /// sub-diagonal of all ones. Companion matrices can be compactly represented by their top rows only.
    ///
    /// This associated function expects to be passed the top rows of the companion matrices as an array of bit-vectors.
    /// The characteristic polynomial of a Frobenius matrix is the product of the characteristic polynomials of its
    /// block companion matrices which are readily computed.
    static BitPolynomial<Word>
    frobenius_matrix_characteristic_polynomial(std::vector<BitVector<Word>> const& top_rows) {
        auto n_companions = top_rows.size();
        if (n_companions == 0) return BitPolynomial<Word>::zero();

        // Compute the product of the characteristic polynomials of the companion matrices.
        auto result = companion_matrix_characteristic_polynomial(top_rows[0]);
        for (auto i = 1uz; i < n_companions; ++i) { result *= companion_matrix_characteristic_polynomial(top_rows[i]); }
        return result;
    }

    /// Class method to return the characteristic polynomial of a *companion matrix* as a `gf2::BitPolynomial`.
    ///
    /// The function expects to be passed the top row of the companion matrix as a bit-vector.
    ///
    /// A companion matrix is a square matrix that is all zeros except for an arbitrary top row and a principal
    /// sub-diagonal of all ones. Companion matrices can be compactly represented by their top rows only.
    ///
    /// The characteristic polynomial of a companion matrix can be computed from its top row.
    ///
    /// # Example
    /// ```
    /// auto top_row = BitVector<>::from_binary_string("101").value();
    /// assert_eq(BitMatrix<>::companion_matrix_characteristic_polynomial(top_row).to_string(), "1 + x^2 + x^3");
    /// ```
    static BitPolynomial<Word> companion_matrix_characteristic_polynomial(BitVector<Word> const& top_row) {
        auto n = top_row.size();

        // The characteristic polynomial is degree n with n + 1 coefficients (leading coefficient is 1).
        auto coeffs = BitVector<Word>::ones(n + 1);

        // The lower order coefficients are the top row of the companion matrix in reverse order.
        for (auto j = 0uz; j < n; ++j) { coeffs.set(n - j - 1, top_row.get(j)); }
        return BitPolynomial<Word>{std::move(coeffs)};
    }

    /// Returns the *Frobenius form* of this bit-matrix in compact top-row only form.
    ///
    /// A Frobenius matrix is a square matrix that consists of one or more blocks of *companion matrices* along the
    /// diagonal. The companion matrices are square matrices that are all zeros except for an arbitrary top row and
    /// a principal sub-diagonal of all ones. Companion matrices can be compactly represented by their top rows
    /// only.
    ///
    /// We can convert any bit-matrix to Frobenius form via a sequence of similarity transformations that preserve the
    /// eigen-structure of the original matrix.
    ///
    /// We return the Frobenius companion matrices in a compact form as a `Vec` of their top rows as bit-vectors.
    ///
    /// @note This method panics if the bit-matrix is not square.
    std::vector<BitVector<Word>> frobenius_form() const {
        // The bit-matrix must be square.
        gf2_assert(is_square(), "Bit-matrix must be square not {} x {}", rows(), cols());

        // Space for the top rows of the companion matrices which we will return.
        auto nr = rows();
        auto top_rows = std::vector<BitVector<Word>>{};
        top_rows.reserve(nr);

        // Make a working copy of the bit-matrix to work through using Danilevsky's algorithm.
        auto copy = *this;
        while (nr > 0) {
            auto companion = copy.danilevsky_step(nr);
            nr -= companion.size();
            top_rows.push_back(companion);
        }
        return top_rows;
    }

    /// @}
    /// @name Bitwise Operations
    /// @{

    /// In-place `XOR` with a bit-matrix `rhs`.
    ///
    /// @note This methods panics if the dimensions do not match.
    ///
    /// # Example
    /// ```
    /// auto lhs = BitMatrix<>::identity(3);
    /// auto rhs = BitMatrix<>::identity(3);
    /// rhs.flip_all();
    /// lhs ^= rhs;
    /// assert_eq(lhs.to_compact_binary_string(), "111 111 111");
    /// ```
    constexpr void operator^=(BitMatrix<Word> const& rhs) {
        gf2_assert(rows() == rhs.rows(), "Row dimensions do not match: {} != {}.", rows(), rhs.rows());
        gf2_assert(cols() == rhs.cols(), "Column dimensions do not match: {} != {}.", cols(), rhs.cols());
        for (auto i = 0uz; i < rows(); ++i) row(i) ^= rhs.row(i);
    }

    /// In-place `AND` with a bit-matrix `rhs`.
    ///
    /// @note This methods panics if the dimensions do not match.
    ///
    /// # Example
    /// ```
    /// auto lhs = BitMatrix<>::identity(3);
    /// auto rhs = BitMatrix<>::identity(3);
    /// rhs.flip_all();
    /// lhs &= rhs;
    /// assert_eq(lhs.to_compact_binary_string(), "000 000 000");
    /// ```
    constexpr void operator&=(BitMatrix<Word> const& rhs) {
        gf2_assert(rows() == rhs.rows(), "Row dimensions do not match: {} != {}.", rows(), rhs.rows());
        gf2_assert(cols() == rhs.cols(), "Column dimensions do not match: {} != {}.", cols(), rhs.cols());
        for (auto i = 0uz; i < rows(); ++i) row(i) &= rhs.row(i);
    }

    /// In-place `OR` with a bit-matrix `rhs`.
    ///
    /// @note This methods panics if the dimensions do not match.
    ///
    /// # Example
    /// ```
    /// auto lhs = BitMatrix<>::identity(3);
    /// auto rhs = BitMatrix<>::identity(3);
    /// rhs.flip_all();
    /// lhs |= rhs;
    /// assert_eq(lhs.to_compact_binary_string(), "111 111 111");
    /// ```
    constexpr void operator|=(BitMatrix<Word> const& rhs) {
        gf2_assert(rows() == rhs.rows(), "Row dimensions do not match: {} != {}.", rows(), rhs.rows());
        gf2_assert(cols() == rhs.cols(), "Column dimensions do not match: {} != {}.", cols(), rhs.cols());
        for (auto i = 0uz; i < rows(); ++i) row(i) |= rhs.row(i);
    }

    /// The`XOR` of this with a bit-matrix `rhs` returning a new bit-matrix.
    ///
    /// @note This methods panics if the dimensions do not match.
    ///
    /// # Example
    /// ```
    /// auto lhs = BitMatrix<>::identity(3);
    /// auto rhs = BitMatrix<>::identity(3);
    /// rhs.flip_all();
    /// auto result = lhs ^ rhs;
    /// assert_eq(result.to_compact_binary_string(), "111 111 111");
    /// ```
    constexpr auto operator^(BitMatrix<Word> const& rhs) const {
        gf2_assert(rows() == rhs.rows(), "Row dimensions do not match: {} != {}.", rows(), rhs.rows());
        gf2_assert(cols() == rhs.cols(), "Column dimensions do not match: {} != {}.", cols(), rhs.cols());
        auto result = *this;
        result ^= rhs;
        return result;
    }

    /// The`AND` of this with a bit-matrix `rhs` returning a new bit-matrix.
    ///
    /// @note This methods panics if the dimensions do not match.
    ///
    /// # Example
    /// ```
    /// auto lhs = BitMatrix<>::identity(3);
    /// auto rhs = BitMatrix<>::identity(3);
    /// rhs.flip_all();
    /// auto result = lhs & rhs;
    /// assert_eq(result.to_compact_binary_string(), "000 000 000");
    /// ```
    constexpr auto operator&(BitMatrix<Word> const& rhs) const {
        gf2_assert(rows() == rhs.rows(), "Row dimensions do not match: {} != {}.", rows(), rhs.rows());
        gf2_assert(cols() == rhs.cols(), "Column dimensions do not match: {} != {}.", cols(), rhs.cols());
        auto result = *this;
        result &= rhs;
        return result;
    }

    /// The`OR` of this with a bit-matrix `rhs` returning a new bit-matrix.
    ///
    /// @note This methods panics if the dimensions do not match.
    ///
    /// # Example
    /// ```
    /// auto lhs = BitMatrix<>::identity(3);
    /// auto rhs = BitMatrix<>::identity(3);
    /// rhs.flip_all();
    /// auto result = lhs | rhs;
    /// assert_eq(result.to_compact_binary_string(), "111 111 111");
    /// ```
    constexpr auto operator|(BitMatrix<Word> const& rhs) const {
        gf2_assert(rows() == rhs.rows(), "Row dimensions do not match: {} != {}.", rows(), rhs.rows());
        gf2_assert(cols() == rhs.cols(), "Column dimensions do not match: {} != {}.", cols(), rhs.cols());
        auto result = *this;
        result |= rhs;
        return result;
    }

    /// Returns a copy of this bit-matrix with all the elements flipped.
    ///
    /// # Example
    /// ```
    /// auto m = BitMatrix<>::identity(3);
    /// auto result = ~m;
    /// assert_eq(result.to_compact_binary_string(), "011 101 110");
    /// ```
    constexpr auto operator~() {
        auto result = *this;
        result.flip_all();
        return result;
    }

    /// @}
    /// @name Arithmetic Operations
    /// @{

    /// In-place addition with a bit-matrix `rhs` -- in GF(2) addition is the same as `XOR`.
    ///
    /// @note This methods panics if the dimensions of do not match.
    ///
    /// # Example
    /// ```
    /// auto lhs = BitMatrix<>::identity(3);
    /// auto rhs = BitMatrix<>::identity(3);
    /// rhs.flip_all();
    /// lhs += rhs;
    /// assert_eq(lhs.to_compact_binary_string(), "111 111 111");
    /// ```
    constexpr void operator+=(BitMatrix<Word> const& rhs) { operator^=(rhs); }

    /// In-place difference with a bit-matrix `rhs` -- in GF(2) subtraction is the same as `XOR`.
    ///
    /// @note This methods panics if the dimensions do not match.
    ///
    /// # Example
    /// ```
    /// auto lhs = BitMatrix<>::identity(3);
    /// auto rhs = BitMatrix<>::identity(3);
    /// rhs.flip_all();
    /// lhs -= rhs;
    /// assert_eq(lhs.to_compact_binary_string(), "111 111 111");
    /// ```
    constexpr void operator-=(BitMatrix<Word> const& rhs) { operator^=(rhs); }

    /// Returns a new bit-matrix that is `*this + rhs` which is `*this ^ rhs` in GF(2).
    ///
    /// @note This methods panics if the dimensions do not match.
    ///
    /// # Example
    /// ```
    /// auto lhs = BitMatrix<>::identity(3);
    /// auto rhs = BitMatrix<>::identity(3);
    /// rhs.flip_all();
    /// auto result = lhs - rhs;
    /// assert_eq(result.to_compact_binary_string(), "111 111 111");
    /// ```
    constexpr auto operator+(BitMatrix<Word> const& rhs) const {
        auto result = *this;
        result += rhs;
        return result;
    }

    /// Returns a new bit-matrix that is `*this - rhs` which is `*this ^ rhs` in GF(2).
    ///
    /// @note This methods panics if the dimensions so not match.
    ///
    /// # Example
    /// ```
    /// auto lhs = BitMatrix<>::identity(3);
    /// auto rhs = BitMatrix<>::identity(3);
    /// rhs.flip_all();
    /// auto result = lhs + rhs;
    /// assert_eq(result.to_compact_binary_string(), "111 111 111");
    /// ```
    constexpr auto operator-(BitMatrix<Word> const& rhs) const {
        auto result = *this;
        result -= rhs;
        return result;
    }

    /// @}
    /// @name String Representations
    /// @{

    /// Returns a configurable binary string representation for this bit-matrix.
    ///
    /// @param row_sep What we use to separate the rows (defaults to a "\n").
    /// @param bit_sep What we use to separate the bit elements in the rows (defaults to "").
    /// @param row_prefix Added to the left of each row (defaults to "").
    /// @param row_suffix Added to the right of each row (defaults to "").
    ///
    /// The default prints each row as 0's and 1's without any formatting, and separates the rows with newlines.
    ///
    /// # Example
    /// ```
    /// auto I = BitMatrix<>::identity(4);
    /// assert_eq(I.to_binary_string(), "1000\n0100\n0010\n0001");
    /// ```
    std::string to_binary_string(std::string_view row_sep = "\n", std::string_view bit_sep = "",
                                 std::string_view row_prefix = "", std::string_view row_suffix = "") const {
        usize nr = rows();

        // Edge case?
        if (nr == 0) return std::string{};

        usize       nc = cols();
        usize       per_row = nc + (nc - 1) * bit_sep.size() + row_prefix.size() + row_suffix.size();
        std::string result;
        result.reserve(nr * per_row);

        for (auto i = 0uz; i < nr; ++i) {
            result += m_rows[i].to_binary_string(bit_sep, row_prefix, row_suffix);
            if (i + 1 < nr) result += row_sep;
        }
        return result;
    }

    /// Returns a one-line minimal binary string representation for this bit-matrix.
    ///
    /// Each row is shown as 0's and 1's without any formatting, and separates the rows with a single space.
    ///
    /// # Example
    /// ```
    /// auto I = BitMatrix<>::identity(4);
    /// assert_eq(I.to_compact_binary_string(), "1000 0100 0010 0001");
    /// ```
    std::string to_compact_binary_string() const { return to_binary_string(" "); }

    /// Returns the default string representation for this bit-matrix.
    ///
    /// Each row is shown as 0's and 1's without any formatting, and separates the rows with a newline.
    ///
    /// # Example
    /// ```
    /// auto I = BitMatrix<>::identity(4);
    /// assert_eq(I.to_string(), "1000\n0100\n0010\n0001");
    /// ```
    std::string to_string() const { return to_binary_string("\n"); }

    /// Returns the default pretty string representation for this bit-matrix.
    ///
    /// Each row is shown as 0's and 1's with a space between each element.
    /// The rows are delimited by a light vertical bar on the left and right.
    /// The rows are separated by newlines.
    ///
    /// # Example
    /// ```
    /// auto I = BitMatrix<>::identity(3);
    /// auto bar = "\u2502";
    /// auto expected = std::format("{0}1 0 0{0}\n{0}0 1 0{0}\n{0}0 0 1{0}", bar);
    /// assert_eq(I.to_pretty_string(), expected);
    /// ```
    std::string to_pretty_string() const { return to_binary_string("\n", " ", "\u2502", "\u2502"); }

    /// "Returns the "hex" string representation of the bit-matrix
    ///
    /// Empty bit-matrices are represented as the empty string.
    ///
    /// Each row in a non-empty bit-matrix is a string of hex chars without any spaces, commas, or other formatting.
    /// By default, the rows are separated by newlines.
    ///
    /// Each row may have a two character *suffix* of the form ".base" where `base` is one of 2, 4 or 8. <br>
    /// All hex characters encode 4 bits: "0X0" -> `0b0000`, "0X1" -> `0b0001`, ..., "0XF" -> `0b1111`. <br>
    /// The three possible ".base" suffixes allow for bit-vectors whose length is not a multiple of 4. <br>
    ///
    /// - `0X1`   is the hex representation of the bit-vector `0001` => length 4.
    /// - `0X1.8` is the hex representation of the bit-vector `001`  => length 3.
    /// - `0X1.4` is the hex representation of the bit-vector `01`   => length 2.
    /// - `0X1.2` is the hex representation of the bit-vector `1`    => length 1.
    ///
    /// # Example
    /// ```
    /// BitMatrix m0;
    /// assert_eq(m0.to_hex_string(), "");
    /// auto m1 = BitMatrix<>::zero(4);
    /// m1.set_all();
    /// assert_eq(m1.to_hex_string(), "F\nF\nF\nF");
    /// auto m2 = BitMatrix<>::zero(5);
    /// m2.flip_all();
    /// assert_eq(m2.to_hex_string(), "F1.2\nF1.2\nF1.2\nF1.2\nF1.2");
    /// ```
    std::string to_hex_string(std::string_view row_sep = "\n") const {
        // Edge case.
        usize nr = rows();
        if (nr == 0) return std::string{};

        // Set up the return string with enough space for the entire matrix.
        usize       nc = cols();
        std::string result;
        result.reserve(nr * (nc / 4 + row_sep.size()));

        // Append each row to the result string.
        for (auto i = 0uz; i < nr; ++i) {
            result += m_rows[i].to_hex_string();
            if (i + 1 < nr) result += row_sep;
        }
        return result;
    }

    /// Returns a compact "hex" string representation of the bit-matrix
    ///
    /// Empty bit-matrices are represented as the empty string.
    ///
    /// Each row in a non-empty bit-matrix is a string of hex chars without any spaces, commas, or other formatting.
    /// The rows are separated by a single space.
    ///
    /// Each row may have a two character *suffix* of the form ".base" where `base` is one of 2, 4 or 8. <br>
    /// All hex characters encode 4 bits: "0X0" -> `0b0000`, "0X1" -> `0b0001`, ..., "0XF" -> `0b1111`. <br>
    /// The three possible ".base" suffixes allow for bit-vectors whose length is not a multiple of 4. <br>
    ///
    /// - `0X1`   is the hex representation of the bit-vector `0001` => length 4.
    /// - `0X1.8` is the hex representation of the bit-vector `001`  => length 3.
    /// - `0X1.4` is the hex representation of the bit-vector `01`   => length 2.
    /// - `0X1.2` is the hex representation of the bit-vector `1`    => length 1.
    ///
    /// # Example
    /// ```
    /// BitMatrix m0;
    /// assert_eq(m0.to_compact_hex_string(), "");
    /// auto m1 = BitMatrix<>::zero(4);
    /// m1.set_all();
    /// assert_eq(m1.to_compact_hex_string(), "F F F F");
    /// auto m2 = BitMatrix<>::zero(5);
    /// m2.flip_all();
    /// assert_eq(m2.to_compact_hex_string(), "F1.2 F1.2 F1.2 F1.2 F1.2");
    /// ```
    std::string to_compact_hex_string() const { return to_hex_string(" "); }

    // The usual output stream operator for a bit-store
    constexpr std::ostream& operator<<(std::ostream& s) const { return s << to_string(); }

    /// @}
    /// @name Comparison Operator
    /// @{

    /// Equality operator checks that any pair of bit-matrices are equal in content.
    ///
    /// # Example
    /// ```
    /// auto p = BitMatrix<>::identity(3);
    /// auto q = BitMatrix<>::identity(3);
    /// assert(p == q);
    /// ```
    friend constexpr bool operator==(BitMatrix const& lhs, BitMatrix const& rhs) {
        // Edge case.
        if (&lhs == &rhs) return true;

        // Check the active words for equality.
        auto row_count = lhs.rows();
        if (rhs.rows() != row_count) return false;
        for (auto i = 0uz; i < row_count; ++i)
            if (lhs.m_rows[i] != rhs.m_rows[i]) return false;

        // Made it
        return true;
    }

private:
    // Runs a check to see that all the rows in a vector of rows have the same number of columns.
    constexpr bool check_rows(std::vector<row_type> const& rows) const {
        if (rows.empty()) return true;
        auto nc = rows[0].size();
        for (auto i = 1uz; i < rows.size(); ++i) {
            if (rows[i].size() != nc) return false;
        }
        return true;
    }

    // Performs a single step of Danilevsky's algorithm to reduce a bit-matrix to Frobenius form.
    //
    // Frobenius form is block diagonal with one or more companion matrices along the diagonal. All matrices can be
    // reduced to this form via a sequence of similarity transformations. This methods performs a single one of those
    // transformations.
    //
    // This `frobenius_form` function calls here with an N x N bit-matrix. In each call the method concentrates on just
    // the top-left n x n sub-matrix. On the first call, n should be set to N. The method returns the top row of the
    // companion matrix that is the transformation of the bottom-right (n-k) x (n-k) sub-matrix. The caller can store
    // that result, decrement n, and call again on the smaller top-left sub-matrix. It may be that the whole matrix
    // gets reduced to a single companion matrix in one step and then there will be no need to call again.
    //
    // The method tries to transform the n x n top-left sub-matrix to a companion matrix working from its bottom-right
    // corner up. It stops when it gets to a point where the bottom-right (n-k) x (n-k) sub-matrix is in companion form
    // and returns the top row of that sub-matrix. The caller can store that result, decrement n, and call again on
    // the smaller top-left sub-matrix.
    //
    // NOTE: This method panics if the bit-matrix is not square.
    BitVector<Word> danilevsky_step(usize n) {
        gf2_assert(n <= rows(), "No top-left {} x {} sub-matrix in a matrix with {} rows", n, n, rows());

        // Edge case: A 1 x 1 matrix is already in companion form.
        if (n == 1) return BitVector<Word>::constant(1, get(0, 0));

        // Step k of algorithm attempts to reduce row k to companion form.
        // By construction, rows k+1 or later are already in companion form.
        auto k = n - 1;
        while (k > 0) {
            // If row k's sub-diagonal is all zeros we look for an earlier column with a 1.
            // If found, we swap that column here & then swap the equivalent rows to preserve similarity.
            if (!get(k, k - 1)) {
                for (auto j = 0uz; j < k - 1; ++j) {
                    if (get(k, j)) {
                        swap_rows(j, k - 1);
                        swap_cols(j, k - 1);
                        break;
                    }
                }
            }

            // No joy? Perhaps we have a companion matrix in the lower left corner and can return its top row?
            if (!get(k, k - 1)) break;

            // Still no joy? The sub-diagonal is not all zeros so apply transform to make it so: self <- M^-1 * self *
            // M, where M is the identity matrix with the (k-1)'st row replaced by the k'th row of `self`. We can
            // sparsely represent M as just a clone of that k'th row of `self`.
            auto m = m_rows[k];

            // Note the M^-1 is the same as M and self <- M^-1 * self just alters a few of our elements.
            for (auto j = 0uz; j < n; ++j) set(k - 1, j, dot(m, col(j)));

            // We also use the sparsity of M when computing self <- self * M.
            for (auto i = 0uz; i < k; ++i) {
                for (auto j = 0uz; j < n; ++j) {
                    auto tmp = get(i, k - 1) && m.get(j);
                    if (j == k - 1) {
                        set(i, j, tmp);
                    } else {
                        set(i, j, get(i, j) ^ tmp);
                    }
                }
            }

            // Now put row k into companion form of all zeros with one on the sub-diagonal.
            // All the rows below k are already in companion form.
            m_rows[k].set_all(false);
            set(k, k - 1);

            // Done with row k
            k -= 1;
        }

        // At this point, k == 0 OR the bit-matrix has non-removable zero on the sub-diagonal of row k.
        // Either way, the bottom-right (n-k) x (n-k) sub-matrix, starting at self[k][k], is in companion form.
        // We return the top row of that companion sub-matrix.
        auto top_row = BitVector<Word>::zeros(n - k);
        for (auto j = 0uz; j < n - k; ++j) top_row.set(j, get(k, k + j));

        // Done
        return top_row;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// Matrix multiplication ...
// -------------------------------------------------------------------------------------------------------------------

/// Bit-matrix, bit-store multiplication, `M * v`, returning a new bit-vector.
template<Unsigned Word, BitStore Rhs>
    requires std::same_as<typename Rhs::word_type, Word>
constexpr auto
dot(BitMatrix<Word> const& lhs, Rhs const& rhs) {
    gf2_assert_eq(lhs.cols(), rhs.size(), "Incompatible dimensions: {} != {}", lhs.cols(), rhs.size());
    auto n_rows = lhs.rows();
    auto result = BitVector<Word>::zeros(n_rows);
    for (auto i = 0uz; i < n_rows; ++i) {
        if (dot(lhs.row(i), rhs)) result.set(i, true);
    }
    return result;
}

/// Operator form for bit-matrix, bit-store multiplication, `M * v`, returning a new bit-vector.
template<Unsigned Word, BitStore Rhs>
    requires std::same_as<typename Rhs::word_type, Word>
constexpr auto
operator*(BitMatrix<Word> const& lhs, Rhs const& rhs) {
    return dot(lhs, rhs);
}

/// `Bit-vector, bit-matrix multiplication, `v * M`, returning a new bit-vector.
///
/// @note We store bit-matrices by rows so `dot(BitMatrix, BitVector)` will always be faster than this.
template<Unsigned Word, BitStore Lhs>
    requires std::same_as<typename Lhs::word_type, Word>
constexpr auto
dot(Lhs const& lhs, BitMatrix<Word> const& rhs) {
    gf2_assert_eq(lhs.size(), rhs.rows(), "Incompatible dimensions: {} != {}", lhs.size(), rhs.rows());
    auto n_cols = rhs.cols();
    auto result = BitVector<Word>::zeros(n_cols);
    for (auto j = 0uz; j < n_cols; ++j) {
        if (dot(lhs, rhs.col(j))) result.set(j, true);
    }
    return result;
}

/// Operator form for bit-vector, bit-matrix multiplication, `v * M`, returning a new bit-vector.
///
/// @note We store bit-matrices by rows so `dot(BitMatrix, BitVector)` will always be faster than this.
template<Unsigned Word, BitStore Lhs>
    requires std::same_as<typename Lhs::word_type, Word>
constexpr auto
operator*(Lhs const& lhs, BitMatrix<Word> const& rhs) {
    return dot(lhs, rhs);
}

/// Bit-matrix, bit-matrix multiplication, `M * N`, returning a new bit-matrix.
template<Unsigned Word>
constexpr auto
dot(BitMatrix<Word> const& lhs, BitMatrix<Word> const& rhs) {
    gf2_assert_eq(lhs.cols(), rhs.rows(), "Incompatible dimensions: {} != {}", lhs.cols(), rhs.rows());

    auto n_rows = lhs.rows();
    auto n_cols = rhs.cols();
    auto result = BitMatrix<Word>::zeros(n_rows, n_cols);

    // Row access is cheap, columns expensive, so arrange things to pull out columns as few times as possible.
    for (auto j = 0uz; j < n_cols; ++j) {
        auto rhs_col = rhs.col(j);
        for (auto i = 0uz; i < n_rows; ++i) {
            if (dot(lhs.row(i), rhs_col)) result.set(i, j, true);
        }
    }
    return result;
}

/// Operator form for bit-matrix, bit-matrix multiplication, `M * N`, returning a new bit-matrix.
template<Unsigned Word>
constexpr auto
operator*(BitMatrix<Word> const& lhs, BitMatrix<Word> const& rhs) {
    return dot(lhs, rhs);
}

// --------------------------------------------------------------------------------------------------------------------
// Some utility methods to print multiple matrices & vectors side-by-side ...
// -------------------------------------------------------------------------------------------------------------------

/// Returns a string that shows a bit-matrix and a bit-vector side-by-side.
template<Unsigned Word, BitStore Rhs>
    requires std::same_as<typename Rhs::word_type, Word>
std::string
string_for(BitMatrix<Word> const& A, Rhs const& b) {

    // If either is empty there was likely a bug somewhere!
    gf2_assert(!A.is_empty(), "Matrix A is empty which is likely an error!");
    gf2_assert(!b.is_empty(), "Vector b is empty which is likely an error!");

    // Most rows we ever print
    auto nr = std::max(A.rows(), b.size());

    // Space filler might be needed if the matrix has fewer rows than the vector
    std::string A_fill(A.cols(), ' ');

    // Little lambda that turns a bool into a string
    auto b2s = [](bool x) { return x ? "1" : "0"; };

    // Build the result string
    std::string result;
    for (auto r = 0uz; r < nr; ++r) {
        auto A_str = r < A.rows() ? A[r].to_string() : A_fill;
        auto b_str = r < b.size() ? b2s(b[r]) : " ";
        result += A_str + "\t" + b_str;
        if (r + 1 < nr) result += "\n";
    }

    return result;
}

/// Returns a string that shows a bit-matrix and two bit-vectors side-by-side.
template<Unsigned Word, BitStore Rhs>
    requires std::same_as<typename Rhs::word_type, Word>
std::string
string_for(BitMatrix<Word> const& A, Rhs const& b, Rhs const& c) {

    // If either is empty there was likely a bug somewhere!
    gf2_assert(!A.is_empty(), "Matrix A is empty which is likely an error!");
    gf2_assert(!b.is_empty(), "Vector b is empty which is likely an error!");
    gf2_assert(!c.is_empty(), "Vector c is empty which is likely an error!");

    // Most rows we ever print
    auto nr = std::max({A.rows(), b.size(), c.size()});

    // Space filler might be needed if the matrix has fewer rows than the vector
    std::string A_fill(A.cols(), ' ');

    // Little lambda that turns a bool into a string
    auto b2s = [](bool x) { return x ? "1" : "0"; };

    // Build the result string
    std::string result;
    for (auto r = 0uz; r < nr; ++r) {
        auto A_str = r < A.rows() ? A[r].to_string() : A_fill;
        auto b_str = r < b.size() ? b2s(b[r]) : " ";
        auto c_str = r < c.size() ? b2s(c[r]) : " ";
        result += A_str + "\t" + b_str + "\t" + c_str;
        if (r + 1 < nr) result += "\n";
    }

    return result;
}

/// Returns a string that shows a bit-matrix and three bit-vectors side-by-side.
template<Unsigned Word, BitStore Rhs>
    requires std::same_as<typename Rhs::word_type, Word>
std::string
string_for(BitMatrix<Word> const& A, Rhs const& b, Rhs const& c, Rhs const& d) {

    // If either is empty there was likely a bug somewhere!
    gf2_assert(!A.is_empty(), "Matrix A is empty which is likely an error!");
    gf2_assert(!b.is_empty(), "Vector b is empty which is likely an error!");
    gf2_assert(!c.is_empty(), "Vector c is empty which is likely an error!");
    gf2_assert(!d.is_empty(), "Vector d is empty which is likely an error!");

    // Most rows we ever print
    auto nr = std::max({A.rows(), b.size(), c.size(), d.size()});

    // Space filler might be needed if the matrix has fewer rows than the vector
    std::string A_fill(A.cols(), ' ');

    // Little lambda that turns a bool into a string
    auto b2s = [](bool x) { return x ? "1" : "0"; };

    // Build the result string
    std::string result;
    for (auto r = 0uz; r < nr; ++r) {
        auto A_str = r < A.rows() ? A[r].to_string() : A_fill;
        auto b_str = r < b.size() ? b2s(b[r]) : " ";
        auto c_str = r < c.size() ? b2s(c[r]) : " ";
        auto d_str = r < d.size() ? b2s(d[r]) : " ";
        result += A_str + "\t" + b_str + "\t" + c_str + "\t" + d_str;
        if (r + 1 < nr) result += "\n";
    }

    return result;
}

/// Returns a string that shows two bit-matrices side-by-side.
template<Unsigned Word>
std::string
string_for(BitMatrix<Word> const& A, BitMatrix<Word> const& B) {

    // If either is empty there was likely a bug somewhere!
    gf2_assert(!A.is_empty(), "Matrix A is empty which is likely an error!");
    gf2_assert(!B.is_empty(), "Matrix B is empty which is likely an error!");

    // Most rows we ever print
    auto nr = std::max(A.rows(), B.rows());

    // Space filler might be needed if the matrix has fewer rows than the vector
    std::string A_fill(A.cols(), ' ');
    std::string B_fill(B.cols(), ' ');

    // Build the result string
    std::string result;
    for (auto r = 0uz; r < nr; ++r) {
        auto A_str = r < A.rows() ? A[r].to_string() : A_fill;
        auto B_str = r < B.rows() ? B[r].to_string() : B_fill;
        result += A_str + "\t" + B_str;
        if (r + 1 < nr) result += "\n";
    }

    return result;
}

/// Returns a string that shows three bit-matrices side-by-side.
template<Unsigned Word>
std::string
string_for(BitMatrix<Word> const& A, BitMatrix<Word> const& B, BitMatrix<Word> const& C) {

    // If either is empty there was likely a bug somewhere!
    gf2_assert(!A.is_empty(), "Matrix A is empty which is likely an error!");
    gf2_assert(!B.is_empty(), "Matrix B is empty which is likely an error!");
    gf2_assert(!C.is_empty(), "Matrix C is empty which is likely an error!");

    // Most rows we ever print
    auto nr = std::max({A.rows(), B.rows(), C.rows()});

    // Space filler might be needed if the matrix has fewer rows than the vector
    std::string A_fill(A.cols(), ' ');
    std::string B_fill(B.cols(), ' ');
    std::string C_fill(C.cols(), ' ');

    // Build the result string
    std::string result;
    for (auto r = 0uz; r < nr; ++r) {
        auto A_str = r < A.rows() ? A[r].to_string() : A_fill;
        auto B_str = r < B.rows() ? B[r].to_string() : B_fill;
        auto C_str = r < C.rows() ? C[r].to_string() : C_fill;
        result += A_str + "\t" + B_str + "\t" + C_str;
        if (r + 1 < nr) result += "\n";
    }

    return result;
}

} // namespace gf2

// --------------------------------------------------------------------------------------------------------------------
// Specialises `std::formatter` to handle bit-matrices ...
// -------------------------------------------------------------------------------------------------------------------

/// Specialise `std::formatter` for the `gf2::BitMatrix<Word>` type.
///
/// You can use the format specifier to alter the output:
/// - `{}` -> binary string (no formatting)
/// - `{:p}` -> pretty-string (with square brackets)
/// - `{:x}` -> hex string
///
/// # Example
/// ```
/// auto m = BitMatrix<u8>::zero(4);
/// m.set_all();
/// auto ms = std::format("{}", m);
/// auto mp = std::format("{:p}", m);
/// auto mx = std::format("{:x}", m);
/// assert_eq(ms, "1111\n1111\n1111\n1111");
/// assert_eq(mp, "\u25021 1 1 1\u2502\n\u25021 1 1 1\u2502\n\u25021 1 1 1\u2502\n\u25021 1 1 1\u2502");
/// assert_eq(mx, "F\nF\nF\nF");
/// ```
template<gf2::Unsigned Word>
struct std::formatter<gf2::BitMatrix<Word>> {

    /// Parse a bit-store format specifier where where we recognize {:p} and {:x}
    constexpr auto parse(std::format_parse_context const& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end() && *it != '}') {
            switch (*it) {
                case 'p': m_pretty = true; break;
                case 'x': m_hex = true; break;
                default: m_error = true;
            }
            ++it;
        }
        return it;
    }

    /// Push out a formatted bit-matrix using the various @c to_string(...) methods in the class.
    template<class FormatContext>
    auto format(gf2::BitMatrix<Word> const& rhs, FormatContext& ctx) const {
        // Was there a format specification error?
        if (m_error) return std::format_to(ctx.out(), "'UNRECOGNIZED FORMAT SPECIFIER FOR BIT-MATRIX'");

        // Special handling requested?
        if (m_hex) return std::format_to(ctx.out(), "{}", rhs.to_hex_string());
        if (m_pretty) return std::format_to(ctx.out(), "{}", rhs.to_pretty_string());

        // Default
        return std::format_to(ctx.out(), "{}", rhs.to_string());
    }

    bool m_hex = false;
    bool m_pretty = false;
    bool m_error = false;
};
