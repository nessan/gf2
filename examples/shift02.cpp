// Compare the optimized `shift` methods with the naive implementations for speed.
// **Note:**  Best run in release mode.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <utilities/stopwatch.h>
#include <utilities/thousands.h>
#include <gf2/namespace.h>
#include "naive.h"

int
main() {
    using word_type = usize;
    using vector_type = BitVec<word_type>;

    // Number of trials & a tick size
    auto n_trials = 1'000uz;
    auto n_tick = n_trials / 20;

    // Size of the bit-vector and the shift amount.
    auto N = 5'000'000uz;
    auto shift = N / 2;

    // The vectors to shift.
    auto vo = vector_type::ones(N);
    auto vn = vector_type::ones(N);

    // A timer.
    utilities::stopwatch sw;

    utilities::pretty_print_thousands();
    std::print("Running {:L} optimized shifts on bit-vectors of length {:L} ", n_trials, N);
    sw.click();
    for (auto n = 0uz; n < n_trials; ++n) {
        // Progress bar
        if (n % n_tick == 0) std::cout << '.' << std::flush;

        vo = vo >> shift;
        vn = vn << shift;
    }
    sw.click();
    auto lap_o = sw.lap();
    std::println(" done.");

    std::print("Running {:L} naive shifts on bit-vectors of length {:L} ", n_trials, N);
    sw.click();
    for (auto n = 0uz; n < n_trials; ++n) {
        // Progress bar
        if (n % n_tick == 0) std::cout << '.' << std::flush;

        vo = naive::shift_right(vo, shift);
        vn = naive::shift_left(vn, shift);
    }
    sw.click();
    auto lap_n = sw.lap();
    std::println(" done.");

    // Check that the results match.
    gf2_assert_eq(vo, vn, "Optimized and naive shifts did not match!");

    // Show the results.
    std::println("Optimized shift time: {:.2f}s.", lap_o);
    std::println("Naive shift time:     {:.2f}s.", lap_n);
    std::println("Ratio:                {:.2f}.", lap_n / lap_o);
}