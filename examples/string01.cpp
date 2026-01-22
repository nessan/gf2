// Quick basic check on word-by-word to_string vs. naive bit-by-bit version.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <utilities/stopwatch.h>
#include <utilities/thousands.h>
#include <gf2/namespace.h>
#include "naive.h"

using word_type = usize;
using vector_type = BitVector<word_type>;

void
test_to_binary_string(std::size_t size, std::size_t n_trials) {
    for (auto i = 0uz; i < n_trials; ++i) {
        auto bv = vector_type::random(size);
        gf2_assert_eq(naive::to_binary_string(bv), bv.to_binary_string(), "Mismatch for size = {:L}", size);
    }
}

void
test_to_hex_string(std::size_t size, std::size_t n_trials) {
    for (auto i = 0uz; i < n_trials; ++i) {
        auto bv = vector_type::random(size);
        gf2_assert_eq(naive::to_hex_string(bv), bv.to_hex_string(), "Mismatch for size = {:L}", size);
    }
}

int
main() {
    utilities::pretty_print_thousands();

    auto n_trials = 10'000uz;
    auto size = 8'127uz;
    test_to_binary_string(size, n_trials);
    std::println("Binary string test complete -- all good!");
    test_to_hex_string(size, n_trials);
    std::println("Hex string test complete -- all good!");
}
