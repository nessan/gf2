#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// The LU decomposition for square bit-matrices. <br>
/// See the [BitLU](docs/pages/BitLU.md) page for more details.

#include <gf2/BitMat.h>

namespace gf2 {

/// The `BitLU` class provides the `LU` decomposition for bit-matrices.
///
/// The class also has methods for solving systems of linear equations over GF(2) and for inverting square bit-matrices.
///
/// Formally, *LU* decomposition of `A` is $P \cdot A = L \cdot U$ where $P$ is a permutation matrix, $L$ is a unit
/// lower triangular matrix and $U$ is an upper triangular matrix.
///
/// The `L` and `U` triangles can be efficiently packed into a single bit-matrix of the same size as `A`
///
/// `P` is a permutation of the identity matrix with one non-zero entry per row. In practice, we just need to store the
/// locations of those entries as some form of vector.
///
/// @note There are generalisations of the LU decomposition for non-square matrices but those are not considered yet.
///
/// @see [Finite field arithmetic](https://en.wikipedia.org/wiki/Finite_field_arithmetic)
template<Unsigned Word>
class BitLU {
private:
    BitMat<Word>       m_lu;    // The LU decomposition stored in a single bit-matrix.
    std::vector<usize> m_swaps; // The list of row swap instructions (a permutation in LAPACK format).
    usize              m_rank;  // The rank of the underlying matrix.

public:
    /// Constructs the LU decomposition object for a square matrix `A`.
    ///
    /// On construction, this method computes a unit lower triangular matrix `L`, an upper triangular matrix `U`,
    /// and a permutation matrix `P` such that `P.A = L.U`. The `L` and `U` triangles are efficiently packed into a
    /// single matrix and `P` is stored as a vector of row swap instructions.
    ///
    /// The construction works even if `A` is singular, though the solver methods will not.
    ///
    /// If `A` is `n x n`, then the construction takes O(n^3) operations. There are block iterative methods that can
    /// reduce that to a sub-cubic count but they are not implemented here. Of course, the method works on whole words
    /// or bit elements at a time so is very efficient even without those enhancements.
    ///
    /// @note Panics if the `A` matrix is not square. There are generalisations of the LU decomposition for
    /// non-square matrices but those are not considered yet.
    ///
    /// # Example (checks that `LU = PA` for a random matrix `A`)
    /// ```
    /// auto A = BitMat<u8>::random(100, 100);
    /// auto lu = A.LU();
    /// auto LU = lu.L() * lu.U();
    /// auto PA = A;
    /// lu.permute(PA);
    /// assert_eq(PA, LU);
    /// ```
    BitLU(BitMat<Word> const& A) : m_lu(A), m_swaps(A.rows(), 0uz), m_rank(A.rows()) {
        // Only handle square matrices
        gf2_assert(A.is_square(), "Matrix is {} x {} but it should be square!", A.rows(), A.cols());

        // Iterate through the bit-matrix.
        auto nr = m_lu.rows();
        auto nc = m_lu.cols();
        for (auto j = 0uz; j < nr; ++j) {

            // Initialize this element of the row swap instruction vector.
            m_swaps[j] = j;

            // Find a non-zero element on or below the diagonal in column -- a pivot.
            auto p = j;
            while (p < nr && !m_lu.get(p, j)) ++p;

            // Perhaps no such element exists in this column? If so the matrix is singular!
            if (p >= nr) {
                m_rank--;
                continue;
            }

            // If necessary, swap the pivot row  into place & record the rows swap instruction
            if (p != j) {
                m_lu.swap_rows(j, p);
                m_swaps[j] = p;
            }

            // At this point m_lu(j,j) == 1
            for (auto i = j + 1; i < nr; ++i) {
                if (m_lu.get(i, j)) {
                    for (auto k = j + 1; k < nc; ++k) {
                        auto tmp = m_lu.get(i, k) ^ m_lu.get(j, k);
                        m_lu.set(i, k, tmp);
                    }
                }
            }
        }
    }

    /// Returns the rank of the matrix.
    ///
    /// # Example
    /// ```
    /// auto A = BitMat<>::left_rotation(100, 1);
    /// auto lu = A.LU();
    /// assert_eq(lu.rank(), 100);
    /// ```
    constexpr usize rank() const { return m_rank; }

    /// Returns `true` if the underlying matrix is singular (i.e. rank deficient).
    constexpr bool is_singular() const { return m_rank < m_lu.rows(); }

    /// Returns the value of the determinant of the underlying matrix as `true` or `false` for 1 or 0.
    constexpr bool determinant() const { return !is_singular(); }

    /// Read-only access to the LU form of the bit-matrix where A -> [L\\U]
    constexpr const BitMat<Word>& LU() const { return m_lu; }

    /// Returns the unit lower triangular matrix `L` in the LU decomposition.
    constexpr BitMat<Word> L() const { return m_lu.unit_lower(); }

    /// Returns the upper triangular matrix `U` in the LU decomposition.
    constexpr BitMat<Word> U() const { return m_lu.upper(); }

    /// Returns a reference to the row swap instructions in [`LAPACK`] form.
    ///
    /// A permutation matrix is just some row permutation of the identity matrix, so it has a single non-zero, 1, entry
    /// in each row or column. You don't need to store the entire matrix but instead store the locations of those 1's.
    ///
    /// In the literature, the permutation vector is often given as a permutation of the index vector. For example, the
    /// permutation vector `[0,2,1,4,3]` tells you that elements/rows 1 and 2 are swapped, as are elements/rows 3 and 4.
    /// This form is easy to interpret at a glance. However, it is tedious to use as a guide to actually executing the
    /// permutations in place.
    ///
    /// The `LAPACK` style `swaps` vector is an alternate, equally compact, form of the permutation matrix. Our
    /// previous example becomes `[0,2,2,4,4]`. This is interpreted as follows:
    ///
    /// - No swap for row 0.
    /// - Swap row 1 with row 2.
    /// - No swap for row 2.
    /// - Swap row 3 with row 4.
    /// - No swap for row 4.
    ///
    /// [`LAPACK`]: https://en.wikipedia.org/wiki/LAPACK
    constexpr const std::vector<usize>& swaps() const { return m_swaps; }

    /// Returns the permutation matrix as a vector of showing the index positions of the non-zero entries.
    ///
    /// A permutation matrix is just some row permutation of the identity matrix, so it has a single non-zero, 1, entry
    /// in each row or column. You don't need to store the entire matrix but instead store the locations of those 1's.
    ///
    /// In the literature, the permutation vector is often given as a permutation of the index vector. For example, the
    /// permutation vector `[0,2,1,4,3]` tells you that elements/rows 1 and 2 are swapped, as are elements/rows 3 and 4.
    /// This form is easy to interpret at a glance and is returned by the `P_vector` method.
    ///
    /// See the `swaps` method for an alternative form of the permutation matrix that is more convenient for executing
    /// the permutations in place.
    std::vector<usize> permutation_vector() const {
        auto n = m_swaps.size();

        std::vector<usize> result(n);
        for (auto i = 0uz; i < n; ++i) result[i] = i;
        for (auto i = 0uz; i < n; ++i) std::swap(result[m_swaps[i]], result[i]);
        return result;
    }

    /// Permutes the rows of the input matrix `B` in-place using our row-swap instruction vector.
    ///
    /// @note This method panics if the dimensions of `B` and the row-swap instruction vector do not match.
    constexpr void permute(BitMat<Word>& B) const {
        auto n = m_swaps.size();
        gf2_assert(B.rows() == n, "Matrix has {} rows but the row-swap instruction vector has {}.", B.rows(), n);
        for (auto i = 0uz; i < n; ++i)
            if (m_swaps[i] != i) B.swap_rows(i, m_swaps[i]);
    }

    /// Permute the rows of the input vector `b` in-place using our row-swap instruction vector.
    ///
    /// @note This method panics if the dimensions of `b` and the row-swap instruction vector do not match.
    template<BitStore Store>
        requires(std::same_as<typename Store::word_type, Word>)
    constexpr void permute(Store& b) const {
        auto n = m_swaps.size();
        gf2_assert(b.size() == n, "Vector has {} elements but the row-swap instruction vector has {}.", b.size(), n);
        for (auto i = 0uz; i < n; ++i)
            if (m_swaps[i] != i) b.swap(i, m_swaps[i]);
    }

    /// Solves the linear system `A.x_for = b` for any `b` where `A` is the matrix used to construct the `BitLU` object.
    ///
    /// This methods returns `std::nullopt` if the matrix is singular.
    ///
    /// @note This method panics if the bit-matrix `b` has a different number of rows than the number of row swap
    /// instructions.
    ///
    /// # Example
    /// ```
    /// auto n = 100uz;
    /// auto A = BitMat<>::left_rotation(n, 1);
    /// auto lu = A.LU();
    /// auto b = BitVec<>::random(n);
    /// auto x = lu(b).value();
    /// assert_eq(A * x, b);
    /// ```
    template<BitStore Store>
        requires(std::same_as<typename Store::word_type, Word>)
    std::optional<BitVec<Word>> operator()(Store const& b) const {
        auto n = m_lu.rows();
        gf2_assert(b.size() == n, "RHS b has {} elements but the LHS matrix has {} rows.", b.size(), n);

        // Can we solve this at all?
        if (is_singular()) return std::nullopt;

        // Make a permuted copy of the the right hand side
        auto x = b;
        permute(x);

        // Forward substitution
        for (auto i = 0uz; i < n; ++i) {
            for (auto j = 0uz; j < i; ++j)
                if (m_lu(i, j)) x[i] ^= x[j];
        }

        // Backward substitution
        for (auto i = n; i--;) {
            for (auto j = i + 1; j < n; ++j)
                if (m_lu(i, j)) x[i] ^= x[j];
        }

        return x;
    }

    /// Solves the linear system `A.X_for = B` for any `B` where `A` is the matrix used to construct the `BitLU` object.
    ///
    /// This methods returns `std::nullopt` if the matrix is singular.
    ///
    /// Each column of `X` is a solution to the linear system `A.x_for = b` where `b` is the corresponding column of
    /// `B`.
    ///
    /// @note This method panics if the bit-matrix `B` has a different number of rows than the number of row swap
    /// instructions.
    ///
    /// # Example
    /// ```
    /// auto A = BitMat<>::left_rotation(100, 5);
    /// auto B = BitMat<>::random(100, 12);
    /// auto lu = A.LU();
    /// auto X = lu(B).value();
    /// assert_eq(A * X, B);
    /// ```
    std::optional<BitMat<Word>> operator()(BitMat<Word> const& B) const {
        auto n = m_lu.rows();
        gf2_assert(B.rows() == n, "RHS B has {} rows but the LHS matrix has {} rows.", B.rows(), n);

        // Can we solve this at all?
        if (is_singular()) return std::nullopt;

        // Make a permuted copy of the the right hand side
        auto X = B;
        permute(X);

        // Solve for each column
        for (auto j = 0uz; j < n; ++j) {

            // Forward substitution
            for (auto i = 0uz; i < n; ++i) {
                for (auto k = 0uz; k < i; ++k)
                    if (m_lu(i, k)) X(i, j) ^= X(k, j);
            }

            // Backward substitution
            for (auto i = n; i--;) {
                for (auto k = i + 1; k < n; ++k)
                    if (m_lu(i, k)) X(i, j) ^= X(k, j);
            }
        }

        return X;
    }

    /// Returns the inverse of the matrix `A` as a full independent bit-matrix or `std::nullopt` if the matrix is
    /// singular.
    ///
    /// # Example
    /// ```
    /// auto A = BitMat<>::left_rotation(100, 1);
    /// BitLU lu{A};
    /// auto A_inv = lu.inverse().value();
    /// assert_eq(A_inv, BitMat<>::right_rotation(100, 1));
    /// ```
    std::optional<BitMat<Word>> inverse() const {
        // We just solve the system A.A_inv = I for A_inv
        return operator()(BitMat<Word>::identity(m_lu.rows()));
    }
};
} // namespace gf2