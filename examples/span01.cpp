// Quick basic check on word-by-word convolution vs. naive bit-by-bit version.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <gf2/namespace.h>

int
main() {

    using word_type = u8;
    using vector_type = BitVector<word_type>;

    auto v = vector_type::ones(77);
    auto s = v.span(5, 33);
    std::println("Original v: {}", v);
    std::println("Original s: {}", s);

    s.flip_all();
    std::println("Flipped  v: {}", v);
    std::println("Flipped  s: {}", s);

    return 0;
}