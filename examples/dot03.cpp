/// Dot products u.M for profiling & benchmarking.
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

    // Random vector & matrix
    using Word = usize;
    auto u = BitVec<Word>::random(N);
    auto mat = BitMat<Word>::random(N, N);

    // To do something in the loop, we count how often the first element from the dot product is 1.
    auto count = 0uz;

    // Run the trials ...
    std::print("Running {:L} trials for u.M where M is {:L} x {:L} ", n_trials, N, N);
    utilities::stopwatch sw;
    for (auto trial = 0uz; trial < n_trials; ++trial) {
        if (trial % n_tick == 0) std::cout << '.' << std::flush;
        if (dot(u, mat).get(0)) count++;

        // Change the input a bit for the next trial.
        u.set(trial % N, true);
    }
    std::println(" done.");

    std::println("Loop time: {}", sw);
    std::println("Counter:   {:L}", count);

    return 0;
}
