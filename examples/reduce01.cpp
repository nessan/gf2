// Compare the optimized  polynomial reduction methods with the naive implementations for accuracy.
// **Note:**  Best run in release mode.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <utilities/thousands.h>
#include <gf2/namespace.h>
#include "naive.h"

int
main() {
    using word_type = usize;
    using poly_type = BitPoly<word_type>;

    // Number of trials & a tick size
    auto n_trials = 1'000uz;
    auto n_tick = n_trials / 20;

    // Max size degree of the polynomial over all trials.
    auto degree_max = 200uz;
    auto degree_dist = std::uniform_int_distribution<std::size_t>(0, degree_max + 1);

    // Reducing x^power where power is in the range power_min to power_max.
    auto power_min = 42uz;
    auto power_max = power_min + 1'000'000uz;
    auto power_dist = std::uniform_int_distribution<std::size_t>(power_min, power_max + 1);

    // A simple Knuth style linear congruential generator seeded to a clock dependent state.
    using lcg = std::linear_congruential_engine<uint64_t, 6364136223846793005U, 1442695040888963407U, 0U>;
    static lcg rng(static_cast<lcg::result_type>(std::chrono::system_clock::now().time_since_epoch().count()));

    utilities::pretty_print_thousands();
    std::print("Running {:L} trials reducing x^n mod P(x) where n ∈ [{:L}, {:L}] and degree(P) ∈ [0, {:L}] ", n_trials,
               power_min, power_max, degree_max);
    for (auto n = 0uz; n < n_trials; ++n) {
        // Progress bar
        if (n % n_tick == 0) std::cout << '.' << std::flush;

        // Polynomial for the trial.
        auto degree = degree_dist(rng);
        auto p = poly_type::ones(degree);

        // Power of x for the trial.
        auto power = power_dist(rng);

        // Do the two different style reductions.
        auto n_reduce = naive::reduce_x_to_the(power, p);
        auto o_reduce = p.reduce_x_to_the(power);

        // Check that the results match.
        gf2_assert_eq(n_reduce, o_reduce, "Mismatch reducing x^{:L} mod P(x): degree(P) = {:L}.", power, degree);
    }
    std::println(" all passed!");
}