// Compare the optimized `shift` methods with the naive implementations for accuracy.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <utilities/thousands.h>
#include <gf2/namespace.h>
#include "naive.h"

int
main() {
    using word_type = usize;
    using vector_type = BitVector<word_type>;

    // Number of trials & a tick size
    auto n_trials = 1'000uz;
    auto n_tick = n_trials / 20;

    // Max size of the bit-vector over all trials.
    auto N_max = 1'000'000uz;

    // A simple Knuth style linear congruential generator seeded to a clock dependent state.
    using lcg = std::linear_congruential_engine<uint64_t, 6364136223846793005U, 1442695040888963407U, 0U>;
    static lcg rng(static_cast<lcg::result_type>(std::chrono::system_clock::now().time_since_epoch().count()));

    utilities::pretty_print_thousands();
    std::println("Running {:L} trials of shifts left & right on bit-vectors of length up to {:L}:", n_trials, N_max);
    for (auto n = 0uz; n < n_trials; ++n) {
        // Progress bar
        if (n % n_tick == 0) std::cout << '.' << std::flush;

        // Size of the bit-vector for this trial.
        auto size_range = std::uniform_int_distribution<std::size_t>(1, N_max);
        auto n_size = size_range(rng);
        auto v = vector_type::ones(n_size);

        // The shift amount for this trial.
        auto shift_range = std::uniform_int_distribution<std::size_t>(0, n_size);
        auto n_shift = shift_range(rng);

        // Do the two different style shifts.
        auto n_right = naive::shift_right(v, n_shift);
        auto n_left = naive::shift_left(v, n_shift);
        auto o_right = v >> n_shift;
        auto o_left = v << n_shift;

        // Check that the results match.
        gf2_assert_eq(n_right, o_right, "Mismatch on right shift: len = {:L}, shift = {:L}.", n_size, n_shift);
        gf2_assert_eq(n_left, o_left, "Mismatch on left shift: len = {:L}, shift = {:L}.", n_size, n_shift);
    }
    std::println("\nAll passed!");
}