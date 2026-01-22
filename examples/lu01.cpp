// Checks on our LU decomposition implementation (does `P.A == L.U` for lots of random `A`s?)
// **Note:**  Best run in release mode.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <iostream>
#include <print>
#include <gf2/namespace.h>

int
main() {
    using word_type = usize;
    using matrix_type = BitMatrix<word_type>;

    // Number of trials
    auto n_trials = 200uz;
    auto n_tick = n_trials / 20;

    // Each trial will run on a bit-matrix of this size
    auto N = 300uz;

    // Number of non-singular bit-matrices
    auto singular = 0uz;

    // Start the trials ...
    std::print("Running {} LU decompositions of {} x {} bit-matrices ", n_trials, N, N);
    for (auto n = 0uz; n < n_trials; ++n) {

        // Progress bar
        if (n % n_tick == 0) std::cout << '.' << std::flush;

        // Create a random bit-matrix & decompose it
        auto A = matrix_type::random(N, N);
        auto lu = A.LU();

        // Check whether P.A = L.U as it should. Exit early if not!
        auto LU = lu.L() * lu.U();
        lu.permute(A);
        gf2_assert_eq(A, LU, "Oops, P.A != L.U!");

        // Count the number of singular bit-matrices we come across.
        if (lu.is_singular()) singular++;
    }
    std::print(" done.\n");

    // Stats ...
    auto p = matrix_type::probability_singular(N);
    std::println("bit-matrix size: {} x {}", N, N);
    std::println("prob[singular]:  {:.2f}%", 100 * p);
    std::println("trials:          {}", n_trials);
    std::println("singular:        {} times", singular);
    std::println("expected:        {} times", int(p * double(n_trials)));
    std::println("In ALL cases P.A == L.U!");

    return 0;
}