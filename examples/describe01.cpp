// Dump a description of a bit-vector.
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <gf2/namespace.h>

int
main() {
    auto v0 = BitVec<>::alternating(77);
    std::println("{}", v0.describe());
    std::println();
    auto v1 = BitVec<u8>::alternating(77);
    std::println("{}", v1.describe());
}
