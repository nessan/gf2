// Check that all the bit-store iterators are bidirectional.
// @link  https://nessan.github.io/gf2pp
///
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <print>
#include <gf2/namespace.h>

// A store type to use for the iterator concept checks.
using store_type = BitVector<std::size_t>;

// Check that we covered all the necessary bases for the `SetBits` iterators to be bidirectional.
static_assert(std::bidirectional_iterator<SetBits<store_type>>);

// Check that we covered all the necessary bases for the `UnsetBits` iterators to be bidirectional.
static_assert(std::bidirectional_iterator<UnsetBits<store_type>>);

// Check that we covered all the necessary bases for the const and non-const `Bits` iterators to be bidirectional.
// Note that the specific bit-store type and specific unsigned are both irrelevant but need to be specified for the
// test.
using store_type = BitVector<std::size_t>;
static_assert(std::bidirectional_iterator<Bits<store_type, false>>);
static_assert(std::bidirectional_iterator<Bits<store_type, true>>);

int
main() {
    std::println("PASSED -- the bit-store iterators all satisfy the `std::bidirectional_iterator` concept!");
    return 0;
}
