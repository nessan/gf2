// Checks on our LU decomposition’s ability to solve A.x = b
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <gf2/namespace.h>

int
main() {
    using word_type = u8;
    using vector_type = BitVector<word_type>;
    using matrix_type = BitMatrix<word_type>;

    // Number of trials
    auto n_trials = 100uz;

    // Each trial will run on a bit-matrix of this size
    auto N = 37uz;

    // Number of singular bit-matrices
    std::size_t singular = 0;

    // Start the trials ...
    for (auto n = 0uz; n < n_trials; ++n) {

        // Create a random bit-matrix & decompose it.
        auto A = matrix_type::random(N, N);
        auto LU = A.LU();

        // Count the number of singular bit-matrices we come across
        if (LU.is_singular()) {
            singular++;
            continue;
        }

        // OK, A is non-singular so check that A.x = b for a random bit-vector b.
        auto b = vector_type::random(N);
        auto x = LU(b).value();
        auto Ax = dot(A, x);
        auto ok = Ax == b;
        if (!ok) {
            std::println(stderr, "A:\n{}", A);
            std::println(stderr, "x:\n{}", x);
            std::println(stderr, "A.x:\n{}", Ax);
            std::println(stderr, "b:\n{}", b);
            exit(1);
        }
        std::println("Trial {:4}: A.x == b? {}", n, ok);
    }

    // Stats ...
    auto p = matrix_type::probability_singular(N);
    std::print("\nSingularity stats ...\n");
    std::print("bit-matrix size: {} x {}\n", N, N);
    std::print("prob[singular]:  {:.2f}%\n", 100 * p);
    std::print("trials:          {}\n", n_trials);
    std::print("singular:        {} times\n", singular);
    std::print("expected:        {} times\n", int(p * double(n_trials)));

    return 0;
}