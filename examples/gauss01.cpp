// Quick basic check of our Gauss solver.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <gf2/namespace.h>
int
main() {
    using word_type = std::uint64_t;
    using vector_type = BitVec<word_type>;
    using matrix_type = BitMat<word_type>;

    auto n = 12uz;
    auto A = matrix_type::random(n, n);
    auto b = vector_type::random(n);
    auto x = A.x_for(b);

    if (x) {
        std::println("bit-matrix A, solution x, right hand side b, and as a check A.x:");
        auto str = string_for(A, *x, b, dot(A, *x));
        std::println("{}", str);
    } else {
        std::println("bit-matrix A with right hand side b has NO solution!:");
        auto str = string_for(A, b);
        std::println("{}", str);
    }
}
