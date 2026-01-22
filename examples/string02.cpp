// Compare the speed of word-by-word to_string vs. naive bit-by-bit version.
// **Note:**  Best run in release mode.
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
compare_bin(std::size_t size, std::size_t n_trials) {
    utilities::stopwatch sw;

    auto bv = vector_type::random(size);

    std::string sn;
    sw.click();
    for (auto i = 0uz; i < n_trials; ++i) { sn = naive::to_binary_string(bv); }
    sw.click();
    auto lap_n = sw.lap();

    std::string so;
    sw.click();
    for (auto i = 0uz; i < n_trials; ++i) { so = bv.to_binary_string(); }
    sw.click();
    auto lap_o = sw.lap();

    gf2_assert_eq(sn, so, "Mismatch for size = {:L}", size);
    std::println("Naive binary string time:     {:.2f}s.", lap_n);
    std::println("Optimized binary string time: {:.2f}s.", lap_o);
    std::println("Ratio:                        {:.2f}.", lap_n / lap_o);
}

void
compare_hex(std::size_t size, std::size_t n_trials) {
    utilities::stopwatch sw;

    auto bv = vector_type::random(size);

    std::string sn;
    sw.click();
    for (auto i = 0uz; i < n_trials; ++i) { sn = naive::to_hex_string(bv); }
    sw.click();
    auto lap_n = sw.lap();

    std::string so;
    sw.click();
    for (auto i = 0uz; i < n_trials; ++i) { so = bv.to_hex_string(); }
    sw.click();
    auto lap_o = sw.lap();

    gf2_assert_eq(sn, so, "Mismatch for size = {:L}", size);
    std::println("Naive hex string time:        {:.2f}s.", lap_n);
    std::println("Optimized hex string time:    {:.2f}s.", lap_o);
    std::println("Ratio:                        {:.2f}.", lap_n / lap_o);
}

int
main() {
    utilities::pretty_print_thousands();
    auto n_trials = 10'000uz;
    auto size = 100'000uz;
    compare_bin(size, n_trials);
    compare_hex(size, n_trials);
}
