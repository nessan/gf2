#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// Utility functions that work on primitive unsigned word types. <br>
/// See the [Unsigned](docs/pages/Unsigned.md) page for more details.

#include <gf2/assert.h>

#include <bit>
#include <concepts>
#include <cstdint>
#include <format>
#include <optional>
#include <utility>
#include <type_traits>

namespace gf2 {

/// The `Unsigned` concept is the same as `std::unsigned_integral`.
/// It is satisfied by all primitive unsigned integer types.
template<typename T>
concept Unsigned = std::unsigned_integral<T>;

/// @name Shorthand Type Aliases.
/// @{

/// Word type alias for an 8-bit unsigned integer.
using u8 = std::uint8_t;

/// Word type alias for a 16-bit unsigned integer.
using u16 = std::uint16_t;

/// Word type alias for a 32-bit unsigned integer.
using u32 = std::uint32_t;

/// Word type alias for a 64-bit unsigned integer.
using u64 = std::uint64_t;

/// Word type alias for the platform's "native"-sized unsigned integer.
using usize = std::size_t;

/// @}
/// @name Variables:
/// @{

/// The number of bits in an `Unsigned` returned as a `u8`.
///
/// # Example
/// ```
/// assert_eq(BITS<u16>, 16);
/// ```
template<Unsigned Word>
constexpr u8 BITS = std::numeric_limits<Word>::digits;

/// The zero value for an `Unsigned` type.
///
/// # Example
/// ```
/// assert_eq(ZERO<u8>, 0);
/// ```
template<Unsigned Word>
constexpr Word ZERO = Word{0};

/// The one value for an `Unsigned` type.
///
/// # Example
/// ```
/// assert_eq(ONE<u8>, 1);
/// ```
template<Unsigned Word>
constexpr Word ONE = Word{1};

/// The `Unsigned` instance with all its bits set to 1.
///
/// # Example
/// ```
/// assert_eq(MAX<u8>, 255);
/// ```
template<Unsigned Word>
constexpr Word MAX = std::numeric_limits<Word>::max();

/// The `Unsigned` instance with alternating bits set to 0 and 1.
///
/// # Example
/// ```
/// assert_eq(ALTERNATING<u8>, 0b0101'0101);
/// ```
template<Unsigned Word>
constexpr Word ALTERNATING = MAX<Word> / 3;

/// @}
/// @name Constructors:
/// @{

/// Returns an `Unsigned` with the bits in the half-open range `[begin, end)` set to 1 and the others set to 0.
///
/// # Panics
/// In debug mode, the range `[begin, end) is checked for validity and the program exits on failure.
///
/// # Example
/// ```
/// assert_eq(with_set_bits<u8>(0, 0), 0b0000'0000);
/// assert_eq(with_set_bits<u8>(0, 1), 0b0000'0001);
/// assert_eq(with_set_bits<u8>(0, 2), 0b0000'0011);
/// assert_eq(with_set_bits<u8>(1, 3), 0b0000'0110);
/// assert_eq(with_set_bits<u8>(0, 8), 0b1111'1111);
/// ```
template<Unsigned Word>
constexpr Word
with_set_bits(u8 begin, u8 end) {
    gf2_debug_assert(end <= BITS<Word>, "End of range `{}` should be at most `{}`.", end, BITS<Word>);
    gf2_debug_assert(begin <= end, "Mis-ordered range: [{}, {})", begin, end);
    return (MAX<Word> << begin) & (MAX<Word> >> (BITS<Word> - end));
}

/// Returns an `Unsigned` with the bits in the half-open range `[begin, end)` set to 0 and the others set to 1.
///
/// # Panics
/// In debug mode, the range `[begin, end) is checked for validity and the program exits on failure.
///
/// # Example
/// ```
/// assert_eq(with_unset_bits<u8>(0, 0), 0b1111'1111);
/// assert_eq(with_unset_bits<u8>(0, 1), 0b1111'1110);
/// assert_eq(with_unset_bits<u8>(0, 2), 0b1111'1100);
/// assert_eq(with_unset_bits<u8>(1, 3), 0b1111'1001);
/// assert_eq(with_unset_bits<u8>(0, 8), 0b0000'0000);
/// ```
template<Unsigned Word>
constexpr Word
with_unset_bits(u8 begin, u8 end) {
    gf2_debug_assert(end <= BITS<Word>, "End of range `{}` should be at most `{}`.", end, BITS<Word>);
    gf2_debug_assert(begin <= end, "Mis-ordered range: [{}, {})", begin, end);
    auto result = with_set_bits<Word>(begin, end);
    return static_cast<Word>(~result);
}

/// @}
/// @name Bit mutators:
/// @{

/// Sets the bits in the half-open range `[begin, end)` of `word` to one, leaving the others unchanged.
///
/// # Panics
/// In debug mode, the range `[begin, end) is checked for validity and the program exits on failure.
///
/// # Example
/// ```
/// u8 word = 0b0000'0000;
/// set_bits(word, 1, 3);
/// assert_eq(word, 0b0000'0110);
/// ```
template<Unsigned Word>
constexpr void
set_bits(Word& word, u8 begin, u8 end) {
    gf2_debug_assert(end <= BITS<Word>, "End of range `{}` should be at most `{}`.", end, BITS<Word>);
    gf2_debug_assert(begin <= end, "Mis-ordered range: [{}, {})", begin, end);
    if (begin != end) word |= with_set_bits<Word>(begin, end);
}

/// Resets the bits in the half-open range `[begin, end)` of `word` to zero, leaving the others unchanged.
///
/// # Panics
/// In debug mode, the range `[begin, end) is checked for validity and the program exits on failure.
///
/// # Example
/// ```
/// u8 word = 0b1111'1111;
/// reset_bits(word, 1, 3);
/// assert_eq(word, 0b1111'1001);
/// ```
template<Unsigned Word>
constexpr void
reset_bits(Word& word, u8 begin, u8 end) {
    gf2_debug_assert(end <= BITS<Word>, "End of range `{}` should be at most `{}`.", end, BITS<Word>);
    gf2_debug_assert(begin <= end, "Mis-ordered range: [{}, {})", begin, end);
    if (begin != end) {
        auto mask = with_set_bits<Word>(begin, end);
        mask = static_cast<Word>(~mask);
        word &= mask;
    }
}

/// Sets the bits of `word` to 1 *except* for those in the half-open range `[begin, end)`  which are unchanged.
///
/// # Panics
/// In debug mode, the range `[begin, end) is checked for validity and the program exits on failure.
///
/// # Example
/// ```
/// u8 word = 0b0000'0000;
/// set_except_bits(word, 1, 3);
/// assert_eq(word, 0b1111'1001);
/// ```
template<Unsigned Word>
constexpr void
set_except_bits(Word& word, u8 begin, u8 end) {
    gf2_debug_assert(end <= BITS<Word>, "End of range `{}` should be at most `{}`.", end, BITS<Word>);
    gf2_debug_assert(begin <= end, "Mis-ordered range: [{}, {})", begin, end);
    if (begin != end) {
        auto mask = with_set_bits<Word>(begin, end);
        mask = static_cast<Word>(~mask);
        word |= mask;
    }
}

/// Sets the bits of `word` to 0 *except* for those in the half-open range `[begin, end)`  which are unchanged.
///
/// # Panics
/// In debug mode, the range `[begin, end) is checked for validity and the program exits on failure.
///
/// # Example
/// ```
/// u8 word = 0b1111'1111;
/// reset_except_bits(word, 1, 3);
/// assert_eq(word, 0b000'0110);
/// ```
template<Unsigned Word>
constexpr void
reset_except_bits(Word& word, u8 begin, u8 end) {
    gf2_debug_assert(end <= BITS<Word>, "End of range `{}` should be at most `{}`.", end, BITS<Word>);
    gf2_debug_assert(begin <= end, "Mis-ordered range: [{}, {})", begin, end);
    if (begin != end) {
        auto mask = with_set_bits<Word>(begin, end);
        word &= mask;
    }
}

/// Returns a copy of `word` with all its bits reversed.
///
/// Reverses the order of bits in `word`. The least significant bit becomes the most significant bit, second
/// least-significant bit becomes second most-significant bit, etc.
///
/// # Example
/// ```
/// u32  n32 = 0x12345678;
/// auto m32 = reverse_bits(n32);
/// assert_eq(m32, 0x1e6a2c48);
/// u64  n64 = 0x1234567890123456;
/// auto m64 = reverse_bits(n64);
/// assert_eq(m64, 0x6a2c48091e6a2c48);
/// u16 zero = 0;
/// assert_eq(reverse_bits(zero), 0);
/// ```
template<Unsigned Word>
constexpr Word
reverse_bits(Word word) {
    Word result = 0;
    for (u8 i = 0; i < BITS<Word>; ++i, word >>= 1) { result = static_cast<Word>((result << 1) | (word & 1)); }
    return result;
}

/// Replace the bits of `word` in the range `[begin, end)` with the bits from `other` leaving the rest unchanged.
///
/// The range is *half* open so the `end` bit is not copied.
///
/// The `Other` type must be convertible to `Unsigned`, perhaps by just removing a `const` qualifier.
///
/// # Panics
/// In debug mode, the range `[begin, end) is checked for validity and the program exits on failure.
///
/// # Example
/// ```
/// u8 word = 0b1111'1111;
/// replace_bits(word, 1, 3, 0b0000'0000);
/// assert_eq(word, 0b1111'1001);
/// ```
template<Unsigned Word, typename Other>
constexpr void
replace_bits(Word& word, u8 begin, u8 end, Other other)
    requires std::convertible_to<Other, Word>
{
    gf2_debug_assert(end <= BITS<Word>, "End of range `{}` should be at most `{}`.", end, BITS<Word>);
    gf2_debug_assert(begin <= end, "Mis-ordered range: [{}, {})", begin, end);
    if (begin != end) {
        auto other_mask = with_set_bits<Word>(begin, end);
        auto word_mask = static_cast<Word>(~other_mask);
        word = (word & word_mask) | (static_cast<Word>(other) & other_mask);
    }
}

/// @}
/// @name Bit Riffling:
/// @{

/// Riffles an `Unsigned` into a pair of others containing the bits in the original word interleaved with zeros.
///
/// For example, if `self` is a `u8` with the binary representation `abcdefgh`, then on return `lo` will have the
/// bits `0a0b0c0d` and `hi` will have the bits `0e0f0g0h`. The `lo` and `hi` words are returned in a tuple.
///
/// # Example
/// ```
/// u8 word = 0b1111'1111;
/// auto [lo, hi] = riffle(word);
/// assert_eq(lo, 0b0101'0101);
/// assert_eq(hi, 0b0101'0101);
/// ```
template<Unsigned Word>
constexpr std::pair<Word, Word>
riffle(Word word) {
    auto half_bits = BITS<Word> / 2;
    Word lo = word & (MAX<Word> >> half_bits);
    Word hi = word >> half_bits;

    // Some magic to interleave the respective halves with zeros starting with half the bits in each half.
    Word one = 1;
    auto shift = BITS<Word> / 4;
    while (shift > 0) {
        Word div = Word(one << shift) | one;
        Word mask = MAX<Word> / div;
        lo = (lo ^ (lo << shift)) & mask;
        hi = (hi ^ (hi << shift)) & mask;
        shift /= 2;
    }
    return std::pair{lo, hi};
}

/// @}
/// @name Bit Counts:
/// @{

/// Returns the number of set bits in an `Unsigned`.
///
/// # Example
/// ```
/// assert_eq(count_ones(u8{0b0000'0000}), 0);
/// assert_eq(count_ones(u8{0b0000'0001}), 1);
/// assert_eq(count_ones(u8{0b0000'0010}), 1);
/// assert_eq(count_ones(u8{0b1111'1111}), 8);
/// ```
template<Unsigned Word>
constexpr u8
count_ones(Word word) {
    return static_cast<u8>(std::popcount(word));
}

/// Returns the number of unset bits in an `Unsigned`
///
/// # Example
/// ```
/// assert_eq(count_zeros(u8{0b0000'0000}), 8);
/// assert_eq(count_zeros(u8{0b0000'0001}), 7);
/// assert_eq(count_zeros(u8{0b0000'0010}), 7);
/// assert_eq(count_zeros(u8{0b1111'1111}), 0);
/// ```
template<Unsigned Word>
constexpr u8
count_zeros(Word word) {
    return BITS<Word> - count_ones(word);
}

/// Returns the number of trailing zeros in an `Unsigned`.
///
/// # Example
/// ```
/// assert_eq(trailing_zeros(u8{0b0000'0000}), 8);
/// assert_eq(trailing_zeros(u8{0b0000'0001}), 0);
/// assert_eq(trailing_zeros(u8{0b0000'0010}), 1);
/// assert_eq(trailing_zeros(u8{0b1111'1111}), 0);
/// ```
template<Unsigned Word>
constexpr u8
trailing_zeros(Word word) {
    return static_cast<u8>(std::countr_zero(word));
}

/// Returns the number of leading zeros in an `Unsigned`.
///
/// # Example
/// ```
/// assert_eq(leading_zeros(u8{0b0000'0000}), 8);
/// assert_eq(leading_zeros(u8{0b0000'0001}), 7);
/// assert_eq(leading_zeros(u8{0b0000'0010}), 6);
/// assert_eq(leading_zeros(u8{0b1111'1111}), 0);
/// ```
template<Unsigned Word>
constexpr u8
leading_zeros(Word word) {
    return static_cast<u8>(std::countl_zero(word));
}

/// Returns the number of trailing ones in an `Unsigned`.
///
/// # Example
/// ```
/// assert_eq(trailing_ones(u8{0b0000'0000}), 0);
/// assert_eq(trailing_ones(u8{0b0000'0001}), 1);
/// assert_eq(trailing_ones(u8{0b0000'0010}), 0);
/// assert_eq(trailing_ones(u8{0b1111'1111}), 8);
/// ```
template<Unsigned Word>
constexpr u8
trailing_ones(Word word) {
    return static_cast<u8>(std::countr_one(word));
}

/// Returns the number of leading ones in an `Unsigned`.
///
/// # Example
/// ```
/// assert_eq(leading_ones(u8{0b0000'0000}), 0);
/// assert_eq(leading_ones(u8{0b0000'0001}), 0);
/// assert_eq(leading_ones(u8{0b1000'0010}), 1);
/// assert_eq(leading_ones(u8{0b1111'1111}), 8);
/// ```
template<Unsigned Word>
constexpr u8
leading_ones(Word word) {
    return static_cast<u8>(std::countl_one(word));
}

/// Returns the index of the lowest set bit in an `Unsigned` or `std::nullopt` if no bits are set.
///
/// # Example
/// ```
/// assert(lowest_set_bit(u8{0b0000'0000}) == std::optional<u8>{});
/// assert(lowest_set_bit(u8{0b0000'0001}) == std::optional<u8>{0});
/// assert(lowest_set_bit(u8{0b0000'0010}) == std::optional<u8>{1});
/// assert(lowest_set_bit(u8{0b1000'0000}) == std::optional<u8>{7});
/// ```
template<Unsigned Word>
constexpr std::optional<u8>
lowest_set_bit(Word word) {
    if (word != 0) return trailing_zeros(word);
    return {};
}

/// @}
/// @name Searches:
/// @{

/// Returns the index of the highest set bit in an `Unsigned` or `std::nullopt` if no bits are set.
///
/// # Example
/// ```
/// assert(highest_set_bit(u8{0b0000'0000}) == std::optional<u8>{});
/// assert(highest_set_bit(u8{0b0000'0001}) == std::optional<u8>{0});
/// assert(highest_set_bit(u8{0b0000'0010}) == std::optional<u8>{1});
/// assert(highest_set_bit(u8{0b1000'0000}) == std::optional<u8>{7});
/// ```
template<Unsigned Word>
constexpr std::optional<u8>
highest_set_bit(Word word) {
    if (word != 0) return BITS<Word> - leading_zeros(word) - 1;
    return {};
}

/// Returns the index of the lowest unset bit in an `Unsigned` or `std::nullopt` if there are no unset bits.
///
/// # Example
/// ```
/// assert(lowest_unset_bit(u8{0b1111'1111}) == std::optional<u8>{});
/// assert(lowest_unset_bit(u8{0b0001'1000}) == std::optional<u8>{0});
/// assert(lowest_unset_bit(u8{0b0000'1001}) == std::optional<u8>{1});
/// assert(lowest_unset_bit(u8{0b1000'1111}) == std::optional<u8>{4});
/// ```
template<Unsigned Word>
constexpr std::optional<u8>
lowest_unset_bit(Word word) {
    if (word != MAX<Word>) return trailing_ones(word);
    return {};
}

/// Returns the index of the highest unset bit in an `Unsigned` or `std::nullopt` if there are none.
///
/// # Example
/// ```
/// assert(highest_unset_bit(u8{0b1111'1111}) == std::optional<u8>{});
/// assert(highest_unset_bit(u8{0b1100'0000}) == std::optional<u8>{5});
/// assert(highest_unset_bit(u8{0b0001'0011}) == std::optional<u8>{7});
/// assert(highest_unset_bit(u8{0b1000'0000}) == std::optional<u8>{6});
/// ```
template<Unsigned Word>
constexpr std::optional<u8>
highest_unset_bit(Word word) {
    if (word != MAX<Word>) return BITS<Word> - leading_ones(word) - 1;
    return {};
}

/// @}
/// @name Stringification:
/// @{

/// Returns the binary string representation of an `Unsigned` showing *all* the bits.
///
/// # Example
/// ```
/// assert_eq(to_binary_string(u8{0b0000'0011}), "00000011");
/// assert_eq(to_binary_string(u8{0b1111'1111}), "11111111");
/// ```
///
/// @todo Make constexpr in C++26 when gcc makes `__builtin_clzg` constexpr.
template<Unsigned Word>
static inline std::string
to_binary_string(Word word) {
    return std::format("{:0{}b}", word, BITS<Word>);
}

/// Returns the hex string representation of an `Unsigned` showing *all* the bits.
///
/// # Example
/// ```
/// assert_eq(to_hex_string(u8{0b0000'0011}), "03");
/// assert_eq(to_hex_string(u8{0b1111'1111}), "FF");
/// ```
///
/// @todo Make constexpr in C++26 when gcc makes `__builtin_clzg` constexpr.
template<Unsigned Word>
static inline std::string
to_hex_string(Word word) {
    return std::format("{:0{}X}", word, BITS<Word> / 4);
}

/// @}
/// @name Bit Locations:
/// @{

/// Returns the number of `Unsigned`s needed to store `n_bits` bits.
///
/// # Example
/// ```
/// assert_eq(words_needed<u8>(0), 0);
/// assert_eq(words_needed<u8>(8), 1);
/// assert_eq(words_needed<u8>(19), 3);
/// ```
template<Unsigned Word>
constexpr usize
words_needed(usize n_bits) {
    // Usually divide returns *both* the quotient & remainder & the compiler will optimise the next line.
    return n_bits / BITS<Word> + (n_bits % BITS<Word> != 0);
}

/// Returns the index of the `Unsigned` word holding bit element `i`.
///
/// Assumes we are holding bits in a sequence of `Unsigned` words.
///
/// # Example
/// ```
/// assert_eq(word_index<u8>(0), 0);
/// assert_eq(word_index<u8>(8), 1);
/// assert_eq(word_index<u8>(19), 2);
/// ```
template<Unsigned Word>
constexpr usize
word_index(usize i) {
    return i / BITS<Word>;
}

/// Returns the bit position within the containing word for bit element `i`.
///
/// Assumes we are holding bits in a sequence of `Unsigned` words.
///
/// # Example
/// ```
/// assert_eq(bit_offset<u8>(0), 0);
/// assert_eq(bit_offset<u8>(8), 0);
/// assert_eq(bit_offset<u8>(19), 3);
/// ```
template<Unsigned Word>
constexpr u8
bit_offset(usize i) {
    return i % BITS<Word>;
}

/// Returns a pair of the index of the word and the bit position within the word for bit element `i`.
///
/// - Assumes we are holding bits in a sequence of `Unsigned` words.
/// - Use with structured binding e.g. `auto [index, offset] = gf2::index_and_offset<gf::u8>(i);`
///
/// # Example
/// ```
/// assert_eq(index_and_offset<u8>(0), (std::pair{0, 0}));
/// assert_eq(index_and_offset<u8>(8), (std::pair{1, 0}));
/// assert_eq(index_and_offset<u8>(19), (std::pair{2, 3}));
/// ```
template<Unsigned Word>
constexpr std::pair<usize, u8>
index_and_offset(usize i) {
    return std::pair{word_index<Word>(i), bit_offset<Word>(i)};
}

/// Returns a pair of the index of the word and a mask isolating bit element `i`.
///
/// - Assumes we are holding bits in a sequence of `Unsigned` words.
/// - Use with structured binding e.g. `auto [index, mask] = gf2::index_and_mask<gf2::u8>(i);`
///
/// # Example
/// ```
/// assert_eq(index_and_mask<u8>(0), (std::pair{0, 1}));
/// assert_eq(index_and_mask<u8>(8), (std::pair{1, 1}));
/// assert_eq(index_and_mask<u8>(19), (std::pair{2, 1 << 3}));
/// ```
template<Unsigned Word>
constexpr std::pair<usize, Word>
index_and_mask(usize i) {
    return std::pair{word_index<Word>(i), static_cast<Word>(ONE<Word> << bit_offset<Word>(i))};
}

/// @}

} // namespace gf2
