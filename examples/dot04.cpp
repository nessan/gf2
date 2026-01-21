/// Dot products M.N for profiling & benchmarking.
/// Run in release mode for realistic timings.
///
/// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
/// SPDX-License-Identifier: MIT
#include <print>
#include <fstream>

#include <chrono>
#include <utilities/stopwatch.h>
#include <utilities/thousands.h>
#include <gf2/namespace.h>

int
main() {
    utilities::pretty_print_thousands();

    // Number of trials & ticks.
    auto n_trials = 1'000uz;
    auto n_tick = n_trials / 20;

    // Matrix/vector size.
    auto N = 1'000uz;

    // Random matrices
    using Word = usize;
    auto m0 = BitMat<Word>::random(N, N);
    auto m1 = BitMat<Word>::random(N, N);

    // To do something in the loop, we count how often the top-right element from the product is 1.
    auto count = 0uz;

    // Run the trials ...
    std::print("Running {:L} trials for M.N where the matrices are {:L} x {:L} ", n_trials, N, N);
    utilities::stopwatch sw;
    for (auto trial = 0uz; trial < n_trials; ++trial) {
        if (trial % n_tick == 0) std::cout << '.' << std::flush;
        if (dot(m0, m1).get(0,0)) count++;

        // Change the input a bit for the next trial.
        auto i = trial % N;
        m0.set(i, i, true);
    }
    std::println(" done.");

    std::println("Loop time: {}", sw);
    std::println("Counter:   {:L}", count);

    return 0;
}
