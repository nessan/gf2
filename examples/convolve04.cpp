// Convolution test for profiling & benchmarking.
// **Note:**  Needs to be run in profile/release mode.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <chrono>
#include <random>
#include <utilities/stopwatch.h>
#include <gf2/namespace.h>

int
main() {
    using vector_type = BitVec<u8>;

    // Convolving bit-vectors u and v with the following sizes.
    auto nu = 5'000uz;
    auto nv = 5'000uz;

    // Create the two input bit-vectors.
    auto u = vector_type::random(nu);
    auto v = vector_type::random(nv);
    auto w = convolve(u, v);

    // Space for the convolution results.
    vector_type w1, w2;

    // Number of trials & a tick size
    auto n_trials = 1000uz;
    auto n_tick = n_trials / 20;

    // And a timer
    utilities::stopwatch sw;

    // Do things the fast way
    std::print("Running {} calls of `convolve(u[{}], v[{}])`   ", n_trials, u.size(), v.size());
    sw.click();
    for (std::size_t n = 0; n < n_trials; ++n) {
        if (n % n_tick == 0) std::cout << '.' << std::flush;
        w = convolve(u, v);
    }
    sw.click();
    auto lap1 = sw.lap();
    std::print(" done with w[0] = {}.\n", w[0]);

    std::print("convolve loop time: {:.2f}s.\n", lap1);
}
