// Create lots of "random" bit-matrices and check that if the matrix is not singular then the inverse is
// correct.
// **Note:**  Best run in release mode.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <utilities/thousands.h>
#include <utilities/stopwatch.h>
#include <gf2/namespace.h>

int
main() {
    utilities::pretty_print_thousands();

    using matrix_type = BitMat<u8>;

    auto n_trials = 1'000uz;
    auto n_tick = n_trials / 20;
    auto N = 500uz;
    auto I = matrix_type::identity(N);

    // A timer
    utilities::stopwatch sw;

    auto singular = 0uz;
    std::print("Running {:L} trials inverting {:L} x {:L} bit-matrices ", n_trials, N, N);
    sw.reset();
    for (auto n = 0uz; n < n_trials; ++n) {
        if (n % n_tick == 0) std::cout << '.' << std::flush;
        auto A = matrix_type::random(N, N);
        if (auto A_inv = A.inverse()) {
            gf2_assert_eq(A * A_inv.value(), I, "Oops! A_inv * A != I");
        } else {
            singular++;
        }
    }
    sw.click();
    auto lap = sw.lap();
    std::println(" done.");

    // Print some statistics ...
    auto p = matrix_type::probability_singular(N);
    std::println("loop time:       {:.2f}s.", lap);
    std::println("bit-matrix size: {:L} x {:L}", N, N);
    std::println("prob[singular]:  {:.2f}%", 100 * p);
    std::println("trials:          {:L}", n_trials);
    std::println("singular:        {:L} times", singular);
    std::println("expected:        {:L} times", int(p * double(n_trials)));
    std::println("In ALL cases `A * A_inv == I`!");

    return 0;
}