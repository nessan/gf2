// Creates lots of random bit-vectors and check that the ratio of 1s to 0s is as expected.
// **Note:**  Best run in release mode.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <gf2/namespace.h>
#include <utilities/thousands.h>

using namespace gf2;

// Computes the error between the number of ones found in a random bit-vector and the expected number of ones.
// @param len The length of the bit-vector.
// @param p The probability of a bit being set.
// @return The error between the number of ones in a random bit-vector and the expected number of ones.
double
error_for(std::size_t len, double p) {
    if (p < 0 || p > 1) throw std::invalid_argument("probability p must be between 0 and 1");
    auto   v = BitVector<>::biased_random(len, p);
    double n_ones = static_cast<double>(v.count_ones());
    double expected = static_cast<double>(len) * p;
    return std::abs(n_ones - expected) / expected;
}
int
main() {
    utilities::pretty_print_thousands();

    // The probability of a bit being set.
    double p = 0.25;

    // We will test vectors of lengths up to `max_size` in steps of `n_size_step`.
    std::size_t max_size = 100'000;
    std::size_t n_sizes = 10;
    std::size_t n_size_step = max_size / n_sizes;

    // For each vector length, we will run `n_trials` trials and compute the average error_for.
    std::size_t n_trials = 1'000;

    std::println("Running {} trials, creating vectors with a {:.0f}% chance of a bit being set.", n_trials, p * 100.0);
    for (std::size_t i = 1; i <= n_sizes; ++i) {
        std::size_t size = n_size_step * i;

        double total_error = 0.0;
        for (std::size_t j = 0; j < n_trials; ++j) { total_error += error_for(size, p); }

        double average_error_for = 100.0 * total_error / static_cast<double>(n_trials);
        std::println("    vector length: {:10L} average error: {:0.2f}%", size, average_error_for);
    }
}