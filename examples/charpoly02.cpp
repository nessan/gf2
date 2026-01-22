// Characteristic polynomial test for profiling & benchmarking.
// **Note:**  This should be run in profile/release mode.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <fstream>

#include <chrono>
#include <utilities/stopwatch.h>
#include <utilities/thousands.h>
#include <gf2/namespace.h>

int
main() {
    using word_type = usize;
    using matrix_type = BitMatrix<word_type>;

    // Number of trials & ticks.
    auto n_trials = 1000uz;
    auto n_tick = n_trials / 20;

    // Matrix size.
    auto N = 100uz;

    // A timer
    utilities::stopwatch sw;

    utilities::pretty_print_thousands();
    std::print("Running {:L} trials for random {:L} x {:L} bit-matrices ", n_trials, N, N);
    sw.reset();
    for (auto n = 0uz; n < n_trials; ++n) {
        if (n % n_tick == 0) std::cout << '.' << std::flush;
        auto m = matrix_type::random(N, N);
        auto p = m.characteristic_polynomial();
        gf2_always_assert(p(m).is_zero(), "Oops! p(m) != 0 for trial {}", n);
    }
    sw.click();
    auto lap = sw.lap();
    std::println(" done.");
    std::println("Characteristic polynomial loop time: {:.2f}s.", lap);

    return 0;
}
