// Check that `to_string` & `from_string` are inverses of each other for bit-matrices.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <utilities/stopwatch.h>
#include <utilities/thousands.h>
#include <gf2/namespace.h>

using word_type = usize;
using matrix_type = BitMatrix<word_type>;

void
test_binary_string(std::size_t rows, std::size_t cols, std::size_t n_trials) {
    auto n_ticks = 20uz;
    std::print("Testing to/from binary strings for {} x {} bit-matrices ", rows, cols);
    for (auto i = 0uz; i < n_trials; ++i) {
        if (i % n_ticks == 0) std::cout << '.' << std::flush;
        auto m = matrix_type::random(rows, cols);
        auto s = m.to_binary_string();
        auto n = matrix_type::from_string(s).value();
        gf2_assert_eq(m, n, "Mismatch for trial = {:L}", i);
    }
    std::println(" all good!");
}

void
test_hex_string(std::size_t rows, std::size_t cols, std::size_t n_trials) {
    auto n_ticks = 20uz;
    std::print("Testing to/from hex strings for {} x {} bit-matrices ", rows, cols);
    for (auto i = 0uz; i < n_trials; ++i) {
        if (i % n_ticks == 0) std::cout << '.' << std::flush;
        auto m = matrix_type::random(rows, cols);
        auto s = m.to_hex_string();
        auto n = matrix_type::from_string(s).value();
        gf2_assert_eq(m, n, "Mismatch for trial = {:L}", i);
    }
    std::println(" all good!");
}

int
main() {
    auto n_trials = 1'000uz;
    auto rows = 127uz;
    auto cols = 275uz;
    test_binary_string(rows, cols, n_trials);
    test_hex_string(rows, cols, n_trials);
}
