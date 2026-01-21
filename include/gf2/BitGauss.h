#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// A Gaussian elimination solver for systems of linear equations over GF(2). <br>
/// See the [BitGauss](docs/pages/BitGauss.md) page for more details.

#include <gf2/BitMat.h>

namespace gf2 {

/// The `BitGauss` class is a Gaussian elimination solver for systems of linear equations over GF(2). <br>
/// It solves systems of the form $A \cdot x = b$ where `A` is a square matrix, `x` is a vector of unknowns, and `b`
/// is the known right-hand side vector.
///
/// Over the reals, systems of linear equations can have 0, 1, or, if the system is underdetermined, an infinite number
/// of solutions. By contrast, over GF(2), in an underdetermined system, the number of solutions is 2^f where `f` is
/// the number of "free" variables". This is because underdetermined variables can be set to one of just two values.
/// Therefore, over GF(2) the number of solutions is 0, 1, or 2^f where `f` is the number of free variables.
///
/// If the system is consistent then we can index into the solutions by an integer `i` where `0 <= i < 2^f`.
///
/// - The `BitGauss::operator()` method returns a random solution.
/// - The `BitGauss::operator(i)` method returns "the" solution indexed `i`.
///
/// For underdetermined systems, the "indexing" is something convenient and consistent across runs but not unique.
///
/// @note The `BitLU` class provides another, often more efficient, way to solve systems of linear equations over GF(2).
/// It also provides the `BitLU::inverse` method for computing the inverse of a matrix.
///
/// # Example
/// ```
/// auto A = BitMat<>::ones(3, 3);
/// auto b = BitVec<>::ones(3);
/// auto solver = A.solver_for(b);
/// assert_eq(solver.rank(), 1);
/// assert_eq(solver.free_count(), 2);
/// assert_eq(solver.solution_count(), 4);
/// assert_eq(solver.is_underdetermined(), true);
/// assert_eq(solver.is_consistent(), true);
/// ```
template<Unsigned Word>
class BitGauss {
private:
    BitMat<Word>       m_A;         // The reduced row echelon form of the matrix `A`.
    BitVec<Word>       m_b;         // The equivalent reduced row echelon form of the vector `b`.
    usize              m_rank;      // The rank of the matrix `A`.
    usize              m_solutions; // The number of solutions to the system.
    std::vector<usize> m_free;      // The indices of the free variables in the system.

public:
    /// Constructs a new `BitGauss` object where we are solving the system of linear equations `A.x = b`.
    ///
    /// @note Panics if the `A` matrix is not square or if the `A` matrix and `b` vector have a different number of rows
    ///
    /// # Example
    /// ```
    /// auto A = BitMat<>::ones(3, 3);
    /// auto b = BitVec<>::ones(3);
    /// BitGauss solver{A, b};
    /// assert_eq(solver.rank(), 1);
    /// assert_eq(solver.is_underdetermined(), true);
    /// assert_eq(solver.is_consistent(), true);
    /// assert_eq(solver.free_count(), 2);
    /// assert_eq(solver.solution_count(), 4);
    /// ```
    template<BitStore Rhs>
    BitGauss(BitMat<Word> const& A, Rhs const& b)
        requires std::same_as<typename Rhs::word_type, Word>
    {
        gf2_assert(A.is_square(), "Matrix is {} x {} but it should be square!", A.rows(), A.cols());
        gf2_assert(A.rows() == b.size(), "Matrix has {} rows but the RHS vector has {} elements.", A.rows(), b.size());

        // Copy A & augment it with b on the right.
        m_A = A;
        m_A.append_col(b);

        // Reduce the augmented matrix to reduced row echelon form.
        auto has_pivot = m_A.to_reduced_echelon_form();

        // Grab the last column of the reduced augmented matrix as a standalone vector, removing it from the matrix.
        m_b = m_A.remove_col().value();

        // Also pop the last element of the `has_pivot` bit-vector as it corresponds to the column that we just removed.
        has_pivot.pop();

        // The rank is the number of columns with a pivot (it is also the number if non-zero rows in the reduced
        // matrix).
        m_rank = has_pivot.count_ones();

        // Any columns without a pivot are free variables.
        for (auto i = 0uz; i < has_pivot.size(); ++i) {
            if (!has_pivot[i]) m_free.push_back(i);
        }

        // Check that all zero rows in the reduced matrix are matched by zero elements in the equivalent reduced right
        // hand side vector (this is consistency). Zero rows are at the bottom from `rank` to the end.
        auto consistent = true;
        for (auto i = m_rank; i < m_A.rows(); ++i) {
            if (m_b[i]) {
                consistent = false;
                break;
            }
        }

        // The number of solutions we can index into is either 0 or 2^f where f is the number of free variables.
        // However, for practical reasons we limit the number of solutions to `min(2^f, 2^63)`.
        m_solutions = 0;
        if (consistent) {
            auto f = m_free.size();
            auto b_max = static_cast<usize>(BITS<Word> - 1);
            auto f_max = std::min(f, b_max);
            m_solutions = 1uz << f_max;
        }
    }

    /// Returns the rank of the matrix `A`.
    ///
    /// # Example
    /// ```
    /// auto A = BitMat<>::ones(3, 3);
    /// auto b = BitVec<>::ones(3);
    /// auto solver = A.solver_for(b);
    /// assert_eq(solver.rank(), 1);
    /// ```
    constexpr usize rank() const { return m_rank; }

    /// Returns the number of free variables in the system.
    ///
    /// # Example
    /// ```
    /// auto A = BitMat<>::ones(3, 3);
    /// auto b = BitVec<>::ones(3);
    /// auto solver = A.solver_for(b);
    /// assert_eq(solver.free_count(), 2);
    /// ```
    constexpr usize free_count() const { return m_free.size(); }

    /// Returns `true` if the system is underdetermined.
    ///
    /// # Example
    /// ```
    /// auto A = BitMat<>::ones(3, 3);
    /// auto b = BitVec<>::ones(3);
    /// auto solver = A.solver_for(b);
    /// assert_eq(solver.is_underdetermined(), true);
    /// ```
    constexpr bool is_underdetermined() const { return !m_free.empty(); }

    /// Returns `true` if the system of linear equations `A.x = b` is consistent.
    ///
    /// A system is consistent if there is at least one solution.
    ///
    /// # Example
    /// ```
    /// auto A = BitMat<>::ones(3, 3);
    /// auto b = BitVec<>::ones(3);
    /// auto solver = A.solver_for(b);
    /// assert_eq(solver.is_consistent(), true);
    /// ```
    constexpr bool is_consistent() const { return m_solutions > 0; }

    /// Returns the maximum number of solutions we can index into.
    ///
    /// This may be 0, 1, or 2^f for some `f` where `f` is the number of free variables in an underdetermined system.
    /// For the `xi_for(i: usize)` function we limit that to the largest power of 2 that fits in `usize`.
    ///
    /// If `usize` is 64 bits then `solution_count = min(2^f, 2^63)`.
    ///
    /// # Example
    /// ```
    /// auto A = BitMat<>::ones(3, 3);
    /// auto b = BitVec<>::ones(3);
    /// auto solver = A.solver_for(b);
    /// assert_eq(solver.solution_count(), 4);
    /// ```
    constexpr usize solution_count() const { return m_solutions; }

    /// Returns a solution to the system of linear equations `A.x = b` or `std::nullopt` if the system is
    /// inconsistent.
    ///
    /// If the system is underdetermined with `f` free variables the returned solution will have `f` random 0/1 entries
    /// for those indices.
    ///
    /// # Example
    /// ```
    /// auto A = BitMat<>::identity(3);
    /// auto b = BitVec<>::ones(3);
    /// auto solver = A.solver_for(b);
    /// assert_eq(solver().value().to_string(), "111");
    /// ```
    std::optional<BitVec<Word>> operator()() const {
        if (!is_consistent()) return std::nullopt;

        // Create a random starting point.
        auto x = BitVec<Word>::random(m_b.size());

        // All non-free variables will be overwritten by back substitution.
        back_substitute_into(x);
        return x;
    }

    /// Returns the `i`th solution to the system of linear equations `A.x = b` or `std::nullopt` if the
    /// system is inconsistent or if `i` is out of bounds.
    ///
    /// If the system is consistent and determined, there is a unique solution and `operator()(0)` is the same as
    /// `operator()()`.
    ///
    /// If the system is underdetermined with `f` free variables, it has `2^f` possible solutions.
    /// If `f` is large, `2^f` may not fit in `usize` but here we limit the number of *indexable* solutions to the
    /// largest power of 2 that fits in `usize`. The indexing scheme is certainly not unique but it is consistent
    /// across runs.
    ///
    /// # Example
    /// ```
    /// auto A = BitMat<>::ones(3, 3);
    /// auto b = BitVec<>::ones(3);
    /// auto solver = A.solver_for(b);
    /// assert_eq(solver.solution_count(), 4);
    /// assert_eq(solver(0).value().to_string(), "100");
    /// assert_eq(solver(1).value().to_string(), "010");
    /// assert_eq(solver(2).value().to_string(), "001");
    /// assert_eq(solver(3).value().to_string(), "111");
    /// ```
    std::optional<BitVec<Word>> operator()(usize i_solution) const {
        if (!is_consistent()) { return std::nullopt; }
        if (i_solution > solution_count()) { return std::nullopt; }

        // We start with a zero vector and then set the free variable slots to the fixed bit pattern for `i`.
        auto x = BitVec<Word>::zeros(m_b.size());
        for (auto f = 0uz; f < m_free.size(); ++f) {
            x[m_free[f]] = (i_solution & 1);
            i_solution >>= 1;
        }

        // Back substitution will now overwrite any non-free variables in `x` with their correct values.
        back_substitute_into(x);
        return x;
    }

private:
    // Helper function that performs back substitution to solve for the non-free variables in `x`.
    void back_substitute_into(BitVec<Word>& x) const {
        // Iterate from the bottom up, starting at the first non-zero row, solving for the non-free variables in `x`.
        for (auto i = m_rank; i--;) {
            auto j = m_A[i].first_set().value();
            x.set(j, m_b[i]);
            for (auto k = j + 1; k < x.size(); ++k)
                if (m_A(i, k)) { x.set(j, x[j] ^ x[k]); }
        }
    }
};

} // namespace gf2
