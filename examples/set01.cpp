// Compare the optimized `*_set` methods with the naive implementations for accuracy.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <utilities/thousands.h>
#include <gf2/namespace.h>
#include "naive.h"

int
main() {
    auto n_trials = 10uz;
    auto N = 1'000'000uz;
    auto p = 0.5;

    utilities::pretty_print_thousands();

    std::println("Running trials of forward search on bit-vectors of length {:L}.", N);
    for (auto i = 0uz; i < n_trials; ++i) {
        std::print("Trial {:3} of {:L}: ", i + 1, n_trials);
        auto bv = BitVector<>::biased_random(N, p);

        // Naive count of the number of set & unset bits.
        auto n_set = 0uz;
        auto n_unset = 0uz;

        // Iterate forward looking for set bits, checking for a mismatch.
        auto n = naive::first_set(bv);
        auto o = bv.first_set();
        while (n) {
            gf2_always_assert_eq(n.value(), o.value(), "Mismatch on set bit {:L}: {:L} vs {:L}.", n_set, n.value(),
                                 o.value());
            n_set++;
            n = naive::next_set(bv, *n);
            o = bv.next_set(*o);
        }

        // Iterate forward looking for unset bits, checking for a mismatch.
        n = naive::first_unset(bv);
        o = bv.first_unset();
        while (n) {
            gf2_always_assert_eq(n.value(), o.value(), "Mismatch on unset bit {:L}: {:L} vs {:L}.", n_unset, n.value(),
                                 o.value());
            n_unset++;
            n = naive::next_unset(bv, *n);
            o = bv.next_unset(*o);
        }

        // Check that the naive and optimized counts match.
        gf2_always_assert_eq(n_set, bv.count_ones(), "Mismatch on count of set bits {:L} vs {:L}.", n_set,
                             bv.count_ones());
        gf2_always_assert_eq(n_unset, bv.count_zeros(), "Mismatch on count of unset bits {:L} vs {:L}.", n_unset,
                             bv.count_zeros());

        // Check that the total is correct.
        gf2_always_assert_eq(n_set + n_unset, N, "Mismatch on total count {:L} vs {:L}.", n_set + n_unset, N);

        // Passed!
        auto expect = p * static_cast<double>(N);
        auto err = 100.0 * std::abs(static_cast<double>(n_set) - expect) / expect;
        std::println("PASS -- both methods found {:L} ones (expected {:.0Lf}, error {:0.2f}%).", n_set, expect, err);
    }
    std::println("");

    std::println("Running trials of reverse search on bit-vectors of length {:L}.", N);
    for (auto i = 0uz; i < n_trials; ++i) {
        std::print("Trial {:3} of {:L}: ", i + 1, n_trials);
        auto bv = BitVector<>::biased_random(N, p);

        // Naive count of the number of set & unset bits.
        auto n_set = 0uz;
        auto n_unset = 0uz;

        // Iterate backward looking for set bits, checking for a mismatch.
        auto n = naive::last_set(bv);
        auto o = bv.last_set();
        while (n) {
            gf2_always_assert_eq(n.value(), o.value(), "Mismatch on set bit {:L}: {:L} vs {:L}.", n_set, n.value(),
                                 o.value());
            n_set++;
            n = naive::previous_set(bv, *n);
            o = bv.previous_set(*o);
        }

        // Iterate backward looking for unset bits, checking for a mismatch.
        n = naive::last_unset(bv);
        o = bv.last_unset();
        while (n) {
            gf2_always_assert_eq(n.value(), o.value(), "Mismatch on unset bit {:L}: {:L} vs {:L}.", n_unset, n.value(),
                                 o.value());
            n_unset++;
            n = naive::previous_unset(bv, *n);
            o = bv.previous_unset(*o);
        }

        // Check that the naive and optimized counts match.
        gf2_always_assert_eq(n_set, bv.count_ones(), "Mismatch on count of set bits {:L} vs {:L}.", n_set,
                             bv.count_ones());
        gf2_always_assert_eq(n_unset, bv.count_zeros(), "Mismatch on count of unset bits {:L} vs {:L}.", n_unset,
                             bv.count_zeros());

        // Check that the total is correct.
        gf2_always_assert_eq(n_set + n_unset, N, "Mismatch on total count {:L} vs {:L}.", n_set + n_unset, N);

        // Passed!
        auto expect = p * static_cast<double>(N);
        auto err = 100.0 * std::abs(static_cast<double>(n_set) - expect) / expect;
        std::println("PASS -- both methods found {:L} ones (expected {:.0Lf}, error {:0.2f}%).", n_set, expect, err);
    }

    return 0;
}