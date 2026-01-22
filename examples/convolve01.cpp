// Quick basic check on word-by-word convolution vs. naive bit-by-bit version.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <gf2/namespace.h>
#include "naive.h"

int
main() {
    auto u = BitVector<>::ones(19);
    auto v = BitVector<>::ones(23);
    v.append(BitVector<>::zeros(8));
    v.set(8, true);

    std::println("Convolving {} and {}.", u, v);

    auto w1 = naive::convolve(u, v);
    auto w2 = convolve(u, v);

    std::println("Element-by-element: {}", w1);
    std::println("Word-by-word:       {}", w2);
    std::println("Results match?      {}", w1 == w2);
}
