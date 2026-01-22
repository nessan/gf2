// Check that `to_string` & `from_string` are inverses of each other for bit-vectors.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <utilities/stopwatch.h>
#include <utilities/thousands.h>
#include <gf2/namespace.h>

using word_type = usize;
using vector_type = BitVector<word_type>;

void
test_binary_string(std::size_t size, std::size_t n_trials) {
    for (auto i = 0uz; i < n_trials; ++i) {
        auto v = vector_type::random(size);
        auto s = v.to_binary_string();
        auto w = vector_type::from_binary_string(s).value();
        gf2_assert_eq(v, w, "Mismatch for trial = {:L}", i);
    }
}

void
test_hex_string(std::size_t size, std::size_t n_trials) {
    for (auto i = 0uz; i < n_trials; ++i) {
        auto v = vector_type::random(size);
        auto s = v.to_hex_string();
        auto w = vector_type::from_hex_string(s).value();
        gf2_assert_eq(v, w, "Mismatch for trial = {:L}", i);
    }
}

int
main() {
    utilities::pretty_print_thousands();

    auto n_trials = 10'000uz;
    auto size = 8'127uz;
    test_binary_string(size, n_trials);
    std::println("Binary string test complete -- all good!");
    test_hex_string(size, n_trials);
    std::println("Hex string test complete -- all good!");
}
