// Checks on the BitStore copy method.
//
// SPDX-FileCopyrightText:  2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT
#include <string_view>
#include <gf2/namespace.h>

template<typename Dst>
void
test_copy_to(std::string_view label, auto& src) {
    BitVec<Dst> dst(src.size());
    dst.copy(src);
    auto src_str = src.to_string();
    auto dst_str = dst.to_string();
    gf2_always_assert_eq(src.size(), dst.size());
    gf2_always_assert_eq(src.count_ones(), dst.count_ones());
    gf2_always_assert_eq(src.count_zeros(), dst.count_zeros());
    gf2_always_assert_eq(src.to_string(), dst.to_string());
    std::println("{} passed ({} bits)", label, src.size());
}

int
main() {
    auto u8_src = BitVec<u8>::random(1011);
    auto u16_src = BitVec<u16>::random(1111);
    auto u32_src = BitVec<u32>::random(991);

    test_copy_to<u8>("u8 -> u8", u8_src);
    test_copy_to<u32>("u8 -> u32", u8_src);
    test_copy_to<u8>("u16 -> u8", u16_src);
    test_copy_to<u64>("u16 -> u64", u16_src);
    test_copy_to<u8>("u32 -> u8", u32_src);
    test_copy_to<u16>("u32 -> u16", u32_src);

    std::println("All copy tests passed.");
    return 0;
}
