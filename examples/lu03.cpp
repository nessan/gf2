// Checks on our LU decomposition’s ability to invert square bit-matrices
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <gf2/namespace.h>

int
main() {
    using word_type = usize;
    using matrix_type = BitMat<word_type>;

    // Number of trials
    auto n_trials = 100uz;

    // Each trial will run on a bit-matrix of this size
    auto N = 200uz;

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
            ;
        }

        // A is non-singular so check that A.A_inv == I
        auto A_inv = LU.inverse().value();
        auto I = dot(A, A_inv);
        auto ok = I.is_identity();
        if (!ok) {
            std::println(stderr, "A:\n{}", A);
            std::println(stderr, "A_inv:\n{}", A_inv);
            std::println(stderr, "A.A_inv:\n{}", I);
            exit(1);
        }
        std::println("Trial {:4}: A.Inverse[A] == I? {}", n, ok);
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
