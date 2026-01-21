// Compare the optimized  polynomial reduction methods with the naive implementations for speed.
// **Note:**  Best run in release mode.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <utilities/thousands.h>
#include <utilities/stopwatch.h>
#include <gf2/namespace.h>
#include "naive.h"

int
main() {
    utilities::pretty_print_thousands();

    using word_type = usize;
    using poly_type = BitPoly<word_type>;

    // The polynomial for the trials.
    auto degree = 17uz;
    auto p = poly_type::random(degree);

    // Going to compute x^N mod p(x) where N = 2^n.
    // NOTE: The iterative method can be very slow if n is large.
    std::size_t n = 27;
    std::size_t N = 1ULL << n;
    std::println("Computing x^2^{:L} mod p(x) == x^{:L} mod p(x).", n, N);

    utilities::stopwatch sw;

    // Use the optimized reduce method with repeated squaring on n.
    std::println("Method `p.reduce({:L}, true)` returns ...", n);
    sw.click();
    auto rr = p.reduce_x_to_the(n, true);
    sw.click();
    std::println("{}\nin {:.6f} seconds.", rr, sw.lap());

    // Use the general optimized reduce method on N.
    std::println("Method `p.reduce({:L}, false)` returns ...", N);
    sw.click();
    rr = p.reduce_x_to_the(N);
    sw.click();
    std::println("{}\nin {:.6f} seconds.", rr, sw.lap());

    // Use the iterative method which might be dog slow!
    std::println("Method `naive::reduce_x_to_the({:L}, p)` returns ...", N);
    sw.click();
    auto rn = naive::reduce_x_to_the(N, p);
    sw.click();
    std::println("{}\nin {:.6f} seconds.", rn, sw.lap());

    return 0;
}