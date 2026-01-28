#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// A concept for types that can access individual bits packed into contiguous primitive unsigned words, along with
/// many free functions that operate on those types. <br>
/// See the [BitStore](docs/pages/BitStore.md) page for more details.

#include <gf2/assert.h>
#include <gf2/Unsigned.h>
#include <gf2/RNG.h>

#include <algorithm>
#include <bitset>
#include <concepts>
#include <format>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <type_traits>

// --------------------------------------------------------------------------------------------------------------------
// The BitStore concept ...
// --------------------------------------------------------------------------------------------------------------------

namespace gf2 {

/// A concept that is satisfied by all bit-vector-like types, which we refer to as _bit-stores_.
///
/// # Required Type
/// The types all have, or view, a `Store` of contiguous bits packed into contiguous primitive unsigned words.
/// They must expose the specific integer word type, `Store::word_type`, holding the store's bit elements.
/// Almost all operations on and between bit-stores work a whole word at a time, so are inherently parallel.
///
/// # Required Methods
/// The bit-store types must provide the following methods:
///
/// | Method             | Description                                                                             |
/// | ------------------ | --------------------------------------------------------------------------------------- |
/// | `size()`           | Returns the number of bits in the store.                                                |
/// | `store() const`    | Returns a read-only pointer to the first word holding bits in the store.                |
/// | `store()`          | Returns a pointer to the first word holding bits in the store.                          |
/// | `offset()`         | Returns the bit location of the store's first bit element in the `*store()` word.       |
///
/// The offset will always be zero for bit-arrays and bit-vectors but typically non-zero for bit-spans, which are views
/// into contiguous bits that need not aligned with underlying word boundaries.
///
/// # Extra Required Methods
/// Strictly speaking, the four methods above are sufficient. However, in practise bit-stores all provide three extra
/// methods that have a large impact on overall performance:
///
/// | Method             | Description                                                                             |
/// | ------------------ | --------------------------------------------------------------------------------------- |
/// | `words()`          | This is always `gf2::words_needed<Store::word_type>(size())` but cached for efficiency. |
/// | `word(i)`          | Returns a "word" from the store that is possibly synthesised from two real words.       |
/// | `set_word(i, val)` | Sets the value of a "word" in the store, possibly altering two real words.              |
///
/// These methods are at the core of every loop in the library, so have a measurable performance impact.
/// They are trivially implemented for bit-arrays and bit-vectors, but require some work for bit-spans where "words" are
/// usually synthesised on the fly from pairs of real underlying unsigned integers.
///
/// # Provided Functions
/// We define dozens of useful free functions that operate on any type satisfying the `BitStore` concept. <br>
/// See the [BitStore](docs/pages/BitStore.md) page for more details.
template<typename Store>
concept BitStore = requires(Store store, const Store const_store) {
    /// The store should expose a`word_type` which is the unsigned integral type it uses to pack its bits in.
    typename Store::word_type;
    requires Unsigned<typename Store::word_type>;

    /// The`size()` method should return the number of bit elements in the store.
    { const_store.size() } -> std::same_as<usize>;

    /// The const `store()` method should return a const pointer to the real first word in the underlying store.
    { const_store.store() } -> std::same_as<const typename Store::word_type*>;

    /// The non-const `store()` method should return a pointer to the real first word in the underlying store.
    /// The pointer may be to const or non-const words depending on whether the store itself is const or not.
    { store.store() } -> std::convertible_to<const typename Store::word_type*>;

    /// The `offset()` method should return the number of bits from the least significant bit of `store()[0]` to the
    /// first bit in the store. This will be zero for bit-vectors and sets, but generally non-zero for bit-spans.
    { const_store.offset() } -> std::same_as<u8>;

    /// The `words()` method should return the **fewest** number of words needed to hold the bits in the store.
    ///
    /// # Note
    /// The return value will always be identical to `gf2::words_needed<word_type>(size())` so we could compute
    /// `words()` on the fly but all bit-store types cache this value, which has a measurable performance impact.
    { const_store.words() } -> std::same_as<usize>;

    /// The `word(i)` method should return "word" `i` from the store.
    ///
    /// It should behave _as if_ bit `0` in the store is located at the least significant bit of `word(0)`.
    /// This is trivial for bit-vectors and sets, but bit-spans typically need to synthesise appropriate "words" on
    /// demand from two contiguous "real" words.
    ///
    /// For example, if the store has 18 elements and the word type is `u8`, then `word(0)` should cover bit elements 0
    /// through 7, `word(1)` bit elements 8 through 15, and `word(2)` bit elements 16 and 17.
    ///
    /// # Note
    /// The final word may not be fully occupied but the method must guarantee that unused bits are set to 0.
    { const_store.word(usize{}) } -> std::same_as<typename Store::word_type>;

    /// The `set_word(i, value)` method should set "word" `i` from the store to the specified `value`.
    ///
    /// It should behave _as if_ bit `0` in the store is located at the least significant bit of `word(0)`.
    /// This is trivial for bit-vectors and sets, but bit-spans typically need to synthesise appropriate "words" on
    /// demand from two contiguous "real" words.
    ///
    /// For example, if the store has 18 elements and the word type is `u8`, then `set_word(0,v)` should change bit
    /// elements 0 through 7, `set_word(1,v)` should change bit elements 8 through 15, and `set_word(2,v)` should change
    /// just bit elements 16 and 17.
    ///
    /// # Note
    /// The method must ensure that inaccessible bits in the underlying store are not changed by this call.
    { store.set_word(usize{}, typename Store::word_type{}) } -> std::same_as<void>;
};

} // namespace gf2

// --------------------------------------------------------------------------------------------------------------------
// Forward Declarations of BitStore Related Types.
// --------------------------------------------------------------------------------------------------------------------

namespace gf2 {

// clang-format off
template<usize N, Unsigned Word> class BitArray;
template<Unsigned Word> class BitVector;
template<Unsigned Word> class BitSpan;
template<BitStore Store> class BitRef;
template<BitStore Store, bool is_const> class Bits;
template<BitStore Store> class SetBits;
template<BitStore Store> class UnsetBits;
template<BitStore Store> class Words;
// clang-format on

} // namespace gf2

// --------------------------------------------------------------------------------------------------------------------
// Free Functions for BitStore Types.
// --------------------------------------------------------------------------------------------------------------------

namespace gf2 {
/// @name Bit Access Functions:
/// @{

/// Returns the bool value of the bit at index `i` in the given bit-store.
///
/// # Panics
/// In debug mode the index `i` is bounds-checked.
///
/// # Example
/// ```
/// BitVector v{10};
/// assert_eq(get(v, 0), false);
/// set(v, 0);
/// assert_eq(get(v, 0), true);
/// ```
template<BitStore Store>
constexpr bool
get(Store const& store, usize i) {
    gf2_debug_assert(i < store.size(), "Index {} is out of bounds for store of length {}", i, store.size());
    auto [word_index, word_mask] = index_and_mask<typename Store::word_type>(i);
    return store.word(word_index) & word_mask;
}

/// Returns a "reference" to the bit element `i` in the given bit-store.
///
/// The returned object is a `BitRef` reference for the bit element at `index` rather than a true reference.
///
/// # Note
/// The referenced bit-store must continue to exist while the `BitRef` is in use.
///
// # Panics
/// In debug mode the index `i` is bounds-checked.
///
/// # Example
/// ```
/// BitVector v{10};
/// ref(v, 2) = true;
/// assert(to_string(v) == "0010000000");
/// auto w = BitVector<>::ones(10);
/// ref(v, 3) = get(w, 3);
/// assert(to_string(v) == "0011000000");
/// ref(v, 4) |= get(w, 4);
/// assert(to_string(v) == "0011100000");
/// ```
template<BitStore Store>
constexpr auto
ref(Store& store, usize i) {
    gf2_debug_assert(i < store.size(), "Index {} is out of bounds for store of length {}", i, store.size());
    return BitRef{&store, i};
}

/// Returns `true` if the first bit element is set, `false` otherwise.
///
/// # Panics
/// In debug mode the method panics if the store is empty.
///
/// # Example
/// ```
/// auto v = BitVector<>::ones(10);
/// assert_eq(front(v), true);
/// set_all(v, false);
/// assert_eq(front(v), false);
/// ```
template<BitStore Store>
constexpr bool
front(Store const& store) {
    gf2_debug_assert(!is_empty(store), "The store is empty!");
    return get(store, 0);
}

/// Returns `true` if the final bit element is set, `false` otherwise.
///
/// # Panics
/// In debug mode the method panics if the store is empty.
///
/// # Example
/// ```
/// auto v = BitVector<>::ones(10);
/// assert_eq(back(v), true);
/// set_all(v, false);
/// assert_eq(back(v), false);
/// ```
template<BitStore Store>
constexpr bool
back(Store const& store) {
    gf2_debug_assert(!is_empty(store), "The store is empty!");
    return get(store, store.size() - 1);
}

/// Sets the bit-element at the given `index` to the specified boolean `value` (default `value` is `true`).
///
/// # Panics
/// In debug mode the method panics if the index is out of bounds.
///
/// # Example
/// ```
/// BitVector v{10};
/// assert_eq(get(v, 0), false);
/// set(v, 0);
/// assert_eq(get(v, 0), true);
/// ```
template<BitStore Store>
constexpr void
set(Store& store, usize i, bool value = true) {
    gf2_debug_assert(i < store.size(), "Index {} is out of bounds for store of length {}", i, store.size());
    auto [word_index, word_mask] = index_and_mask<typename Store::word_type>(i);
    auto word_value = store.word(word_index);
    auto bit_value = (word_value & word_mask) != 0;
    if (bit_value != value) store.set_word(word_index, word_value ^ word_mask);
}

/// Flips the value of the bit-element at the given `index`.
///
/// # Panics
/// In debug mode the method panics if the index is out of bounds.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::ones(10);
/// flip(v, 0);
/// assert_eq(to_string(v), "0111111111");
/// flip(v, 1);
/// assert_eq(to_string(v), "0011111111");
/// flip(v, 9);
/// assert_eq(to_string(v), "0011111110");
/// ```
template<BitStore Store>
constexpr void
flip(Store& store, usize i) {
    gf2_debug_assert(i < store.size(), "Index {} is out of bounds for store of length {}", i, store.size());
    auto [word_index, word_mask] = index_and_mask<typename Store::word_type>(i);
    auto word_value = store.word(word_index);
    store.set_word(word_index, word_value ^ word_mask);
}

/// Swaps the bits in the bit-store at indices `i0` and `i1`.
///
/// # Panics
/// In debug mode the method panics if either index is out of bounds.
///
/// # Example
/// ```
/// auto v = BitVector<>::zeros(10);
/// set(v, 0);
/// assert_eq(to_string(v), "1000000000");
/// swap(v, 0, 1);
/// assert_eq(to_string(v), "0100000000");
/// swap(v, 0, 1);
/// assert_eq(to_string(v), "1000000000");
/// swap(v, 0, 9);
/// assert_eq(to_string(v), "0000000001");
/// swap(v, 0, 9);
/// assert_eq(to_string(v), "1000000000");
/// ```
template<BitStore Store>
constexpr void
swap(Store& store, usize i0, usize i1) {
    gf2_debug_assert(i0 < store.size(), "index {} is out of bounds for a store of length {}", i0, store.size());
    gf2_debug_assert(i1 < store.size(), "index {} is out of bounds for a store of length {}", i1, store.size());
    if (i0 != i1) {
        auto [w0, mask0] = index_and_mask<typename Store::word_type>(i0);
        auto [w1, mask1] = index_and_mask<typename Store::word_type>(i1);
        auto word0 = store.word(w0);
        auto word1 = store.word(w1);
        auto val0 = (word0 & mask0) != 0;
        auto val1 = (word1 & mask1) != 0;
        if (val0 != val1) {
            if (w0 == w1) {
                // Both bits are in the same word
                store.set_word(w0, word0 ^ mask0 ^ mask1);
            } else {
                // The two bits are in different words
                store.set_word(w0, word0 ^ mask0);
                store.set_word(w1, word1 ^ mask1);
            }
        }
    }
}

/// @}
/// @name Store Queries:
/// @{

/// Returns `true` if the store is empty, `false` otherwise.
///
/// # Example
/// ```
/// BitVector v;
/// assert_eq(is_empty(v), true);
/// BitVector u{10};
/// assert_eq(is_empty(u), false);
/// ```
template<BitStore Store>
constexpr bool
is_empty(Store const& store) {
    return store.size() == 0;
}

/// Returns `true` if at least one bit in the store is set, `false` otherwise.
///
/// # Note
/// Empty stores have no set bits (logical connective for `any` is `OR` with identity `false`).
///
/// # Example
/// ```
/// BitVector v{10};
/// assert_eq(any(v), false);
/// set(v, 0);
/// assert_eq(any(v), true);
/// ```
template<BitStore Store>
constexpr bool
any(Store const& store) {
    for (auto i = 0uz; i < store.words(); ++i)
        if (store.word(i) != 0) return true;
    return false;
}

/// Returns `true` if all bits in the store are set, `false` otherwise.
///
/// # Note
/// Empty stores have no set bits (logical connective for `all` is `AND` with identity `true`).
///
/// # Example
/// ```
/// BitVector v{3};
/// assert_eq(all(v), false);
/// set(v, 0);
/// set(v, 1);
/// set(v, 2);
/// assert_eq(all(v), true);
/// ```
template<BitStore Store>
constexpr bool
all(Store const& store) {
    auto num_words = store.words();
    if (num_words == 0) { return true; }

    // Check the fully occupied words ...
    using word_type = typename Store::word_type;
    for (auto i = 0uz; i < num_words - 1; ++i) {
        if (store.word(i) != MAX<word_type>) return false;
    }

    // Last word might not be fully occupied ...
    auto bits_per_word = BITS<word_type>;
    auto tail_bits = store.size() % bits_per_word;
    auto unused_bits = tail_bits == 0 ? 0 : bits_per_word - tail_bits;
    auto last_word_max = MAX<word_type> >> unused_bits;
    if (store.word(num_words - 1) != last_word_max) return false;

    return true;
}

/// Returns `true` if no bits in the store are set, `false` otherwise.
///
/// # Note
/// Empty store have no set bits (logical connective for `none` is `AND` with identity `true`).
///
/// # Example
/// ```
/// BitVector v{10};
/// assert_eq(none(v), true);
/// set(v,0);
/// assert_eq(none(v), false);
/// ```
template<BitStore Store>
constexpr bool
none(Store const& store) {
    return !any(store);
}

/// @}
/// @name Store Mutators:
/// @{

/// Sets the bits in the store to the boolean `value`.
///
/// By default, all bits are set to `true`.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::zeros(10);
/// set_all(v);
/// assert_eq(to_string(v), "1111111111");
/// ```
template<BitStore Store>
constexpr void
set_all(Store& store, bool value = true) {
    using word_type = typename Store::word_type;
    auto word_value = value ? MAX<word_type> : word_type{0};
    for (auto i = 0uz; i < store.words(); ++i) store.set_word(i, word_value);
}

/// Flips the value of the bits in the store.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::zeros(10);
/// flip_all(v);
/// assert_eq(to_string(v), "1111111111");
/// ```
template<BitStore Store>
constexpr void
flip_all(Store& store) {
    using word_type = typename Store::word_type;
    for (auto i = 0uz; i < store.words(); ++i) store.set_word(i, static_cast<word_type>(~store.word(i)));
}

/// @}
/// @name Store Fills:
/// @{

/// Copies all the bits from _any_ unsigned integral `src` value to an _equal-sized_ bit-store.
///
/// # Note
/// 1. We allow *any* unsigned integral source, e.g. copying a single `u64` into a `BitVector<u8>` of size 64.
/// 2. The least-significant bit of the source becomes the bit at index 0 in the store.
///
/// # Panics
/// Panics if the size of the store does not match the number of bits in the source integer type.
///
/// # Example
/// ```
/// BitVector<u8> v{16};
/// u16 src = 0b1010101010101010;
/// copy(src, v);
/// assert_eq(to_string(v), "0101010101010101");
/// BitVector<u32> w{16};
/// copy(src, w);
/// assert_eq(to_string(w), "0101010101010101");
/// ```
template<Unsigned Src, BitStore Store>
constexpr void
copy(Src src, Store& store) {
    constexpr auto src_bits = BITS<Src>;
    gf2_assert(store.size() == src_bits, "Lengths do not match: {} != {}.", store.size(), src_bits);

    using dst_type = typename Store::word_type;
    constexpr auto dst_bpw = BITS<dst_type>;

    if constexpr (src_bits <= dst_bpw) {
        // The source fits into a single store word. We can pad the source to get it to the correct size.
        // It happens that casting zero-pads --- but `set_word` only ever looks at the low `src_bits` piece anyway.
        store.set_word(0, static_cast<dst_type>(src));
    } else {
        // The source spans some integer number of store words (we assume it is an integer number).
        // Proceed by nibbling off destination-sized words from the larger source until its exhausted.
        constexpr auto dst_words = src_bits / dst_bpw;
        constexpr auto dst_mask = MAX<dst_type>;
        for (auto i = 0uz; i < dst_words; ++i, src >>= dst_bpw) {
            auto src_word = static_cast<dst_type>(src & dst_mask);
            store.set_word(i, src_word);
        }
    }
}

/// Copies all the bits from an iteration of _any_ unsigned integral `src` values to an _equal-sized_ bit-store.
///
/// # Note
/// We allow *any* unsigned integral source, e.g. copying `u64` words into a `BitVector<u8>` of the correct size.
///
/// # Panics
/// Panics if the size of the store does not match the number of bits in the source iteration.
///
/// # Example
/// ```
/// BitVector<u8> v{48};
/// std::vector<u16> src = { 0b1010101010101010, 0b1010101010101010, 0b1111111111111111 };
/// gf2::copy(src.begin(), src.end(), v);
/// assert_eq(to_string(v), "010101010101010101010101010101011111111111111111");
/// BitVector<u32> w{48};
/// gf2::copy(src.begin(), src.end(), w);
/// assert_eq(to_string(w), "010101010101010101010101010101011111111111111111");
/// ```
template<typename Iter, BitStore Store>
    requires std::is_unsigned_v<typename std::iterator_traits<Iter>::value_type>
constexpr void
copy(Iter src_begin, Iter src_end, Store& store) {

    // What are the two word types (without any const qualifiers)?
    using dst_type = std::remove_const_t<typename Store::word_type>;
    using src_type = typename std::iterator_traits<Iter>::value_type;

    // Numbers of bits per respective word.
    constexpr auto dst_bpw = BITS<dst_type>;
    constexpr auto src_bpw = BITS<src_type>;

    // Size of the source in words and bits.
    auto src_words = static_cast<std::size_t>(std::distance(src_begin, src_end));
    auto src_bits = src_words * src_bpw;

    // The sizes in bits must match ...
    gf2_assert(store.size() == src_bits, "Lengths do not match: {} != {}.", store.size(), src_bits);

    // What we do next depends on the size of the respective words.
    if constexpr (src_bpw == dst_bpw) {
        // Same sized words -- we can copy whole words directly ...
        auto src_iter = src_begin;
        for (auto i = 0uz; i < store.words(); ++i, ++src_iter) store.set_word(i, static_cast<dst_type>(*src_iter));
    } else {
        // Otherwise proceed by copying one source word at a time into the appropriate span in the destination store.
        // The actual copying is done by the earlier method that copies any unsigned into the store,
        auto src_iter = src_begin;
        auto dst_begin = 0uz;
        for (auto i = 0uz; i < src_words; ++i, ++src_iter, dst_begin += src_bpw) {
            auto dst_span = span(store, dst_begin, dst_begin + src_bpw);
            copy(*src_iter, dst_span);
        }
    }
}

/// Copies all the bits from _any_ `src` bit-store to another _equal-sized_ `dst` bit-store.
///
/// # Note:
/// This is one of the few methods in the library that *doesn't* require the two stores to have the same `word_type`.
/// You can use it to convert between different `word_type` stores (e.g., from `BitVector<u32>` to `BitVector<u8>`) as
/// long as the sizes match.
///
/// # Panics
/// Panics if the sizes of the two stores do not match.
///
/// # Example
/// ```
/// auto v = BitVector<u64>::ones(10);
/// assert_eq(to_string(v), "1111111111");
/// copy(BitVector<u8>::alternating(10), v);
/// assert_eq(to_string(v), "1010101010");
/// ```
template<BitStore Src, BitStore Dst>
constexpr void
copy(Src const& src, Dst& dst) {
    // The sizes must match ...
    gf2_assert(dst.size() == src.size(), "Lengths do not match: {} != {}.", dst.size(), src.size());

    // What are the word types (without any const qualifiers)?
    using dst_type = std::remove_const_t<typename Dst::word_type>;
    using src_type = std::remove_const_t<typename Src::word_type>;

    // Numbers of bits per respective words
    constexpr auto dst_bpw = BITS<dst_type>;
    constexpr auto src_bpw = BITS<src_type>;

    // What we do next depends on the size of the respective words.
    if constexpr (dst_bpw == src_bpw) {
        // Same sized words holding the same number of bits => same number of words in each ...
        for (auto i = 0uz; i < dst.words(); ++i) {
            auto value = static_cast<dst_type>(src.word(i));
            dst.set_word(i, value);
        }
    } else if constexpr (dst_bpw > src_bpw) {
        // The destination word type is larger -- some number of source words fit into each destination word.
        // We assume that the destination word size is an integer multiple of the source word size.
        constexpr auto ratio = dst_bpw / src_bpw;

        auto src_words = src.words();
        for (auto i = 0uz; i < dst.words(); ++i) {
            // Combine `ratio` source words into a single destination word being careful not to run off the end of `src`
            auto value = ZERO<dst_type>;
            for (auto j = 0uz, src_index = i * ratio; j < ratio && src_index < src_words; ++j, ++src_index) {
                auto src_word = static_cast<dst_type>(src.word(src_index));
                value |= (src_word << (j * src_bpw));
            }
            dst.set_word(i, value);
        }
    } else {
        // The destination word type is smaller -- each source word becomes multiple destination words.
        // We assume that the source word size is an integer multiple of the destination word size.
        constexpr auto ratio = src_bpw / dst_bpw;

        auto dst_words = dst.words();
        auto dst_mask = MAX<dst_type>;
        for (auto i = 0uz; i < src.words(); ++i) {
            // Split each source word into `ratio` destination words being careful not to run off the end of `dst`
            auto src_word = src.word(i);
            for (auto j = 0uz, dst_index = i * ratio; j < ratio && dst_index < dst_words; ++j, ++dst_index) {
                dst.set_word(dst_index, static_cast<dst_type>(src_word & dst_mask));
                src_word >>= dst_bpw;
            }
        }
    }
}

/// Copies all the bits from a `std::bitset` to an _equal-sized_ bit-store.
///
/// # Note
/// A `std::bitset` prints its bit elements in *bit-order* which is the reverse of our convention.
///
/// # Panics
/// Panics if the size of the store does not match the number of bits in the source `std::bitset`.
///
/// # Example
/// ```
/// std::bitset<10> src{0b1010101010};
/// BitVector v{10};
/// copy(src, v);
/// assert_eq(to_string(v), "0101010101");
/// ```
template<usize N, BitStore Store>
constexpr void
copy(std::bitset<N> const& src, Store& store) {
    // The sizes must match
    gf2_assert(store.size() == N, "Lengths do not match: {} != {}.", store.size(), N);

    // Doing this the dumb way as there is no standard access to the bitset's underlying store
    set_all(store, false);
    for (auto i = 0uz; i < N; ++i)
        if (src[i]) set(store, i);
}

/// Copies bits from enumerating calls `f(i)` for each index in the bit-store
///
/// # Example
/// ```
/// BitVector v{10};
/// copy(v, [](usize i) { return i % 2 == 0; });
/// assert_eq(v.size(), 10);
/// assert_eq(to_string(v), "1010101010");
/// ```
template<BitStore Store>
constexpr void
copy(Store& store, std::invocable<usize> auto f) {
    set_all(store, false);
    for (auto i = 0uz; i < store.size(); ++i)
        if (f(i) == true) set(store, i);
}

/// Fill the store with random bits based on an optional probability `p` and an optional `seed` for the RNG.
///
/// The default call `fill_random()` sets each bit to 1 with probability 0.5 (fair coin).
///
/// @param p The probability of the elements being 1 (defaults to a fair coin, i.e. 50-50).
/// @param seed The seed to use for the random number generator (defaults to 0, which means use entropy).
///
/// If `p < 0` then the fill is all zeros, if `p > 1` then the fill is all ones.
///
/// # Example
/// ```
/// BitVector u{10}, v{10};
/// u64 seed = 1234567890;
/// fill_random(u, 0.5, seed);
/// fill_random(v, 0.5, seed);
/// assert(u == v);
/// ```
template<BitStore Store>
constexpr void
fill_random(Store& store, double p = 0.5, u64 seed = 0) {
    // Keep a single static RNG per thread for all calls to this method, seeded with entropy on the first call.
    thread_local RNG rng;

    // Edge case handling ...
    if (p < 0) {
        set_all(store, false);
        return;
    }

    // Scale p by 2^64 to remove floating point arithmetic from the main loop below.
    // If we determine p rounds to 1 then we can just set all elements to 1 and return early.
    p = p * 0x1p64 + 0.5;
    if (p >= 0x1p64) {
        set_all(store);
        return;
    }

    // p does not round to 1 so we use a 64-bit URNG and check each draw against the 64-bit scaled p.
    auto scaled_p = static_cast<u64>(p);

    // If a seed was provided, set the RNG's seed to it. Otherwise, we carry on from where we left off.
    u64 old_seed = rng.seed();
    if (seed != 0) rng.set_seed(seed);

    set_all(store, false);
    for (auto i = 0uz; i < store.size(); ++i)
        if (rng.u64() < scaled_p) set(store, i);

    // Restore the old seed if necessary.
    if (seed != 0) rng.set_seed(old_seed);
}

/// @}
/// @name Bit Counts:
/// @{

/// Returns the number of set bits in the store.
///
/// # Example
/// ```
/// BitVector v{10};
/// assert_eq(count_ones(v), 0);
/// set(v, 0);
/// assert_eq(count_ones(v), 1);
/// ```
template<BitStore Store>
constexpr usize
count_ones(Store const& store) {
    usize count = 0;
    for (auto i = 0uz; i < store.words(); ++i) count += static_cast<usize>(gf2::count_ones(store.word(i)));
    return count;
}

/// Returns the number of unset bits in the store.
///
/// # Example
/// ```
/// BitVector v{10};
/// assert_eq(count_zeros(v), 10);
/// set(v, 0);
/// assert_eq(count_zeros(v), 9);
/// ```
template<BitStore Store>
constexpr usize
count_zeros(Store const& store) {
    return store.size() - count_ones(store);
}

/// Returns the number of leading zeros in the store.
///
/// # Example
/// ```
/// BitVector v{37};
/// assert_eq(leading_zeros(v), 37);
/// set(v, 27);
/// assert_eq(leading_zeros(v), 27);
/// auto w = BitVector<u8>::ones(10);
/// assert_eq(leading_zeros(w), 0);
/// ```
template<BitStore Store>
constexpr usize
leading_zeros(Store const& store) {
    // Note: Even if the last word is not fully occupied, we know unused bits are set to 0.
    auto bits_per_word = BITS<typename Store::word_type>;
    for (auto i = 0uz; i < store.words(); ++i) {
        auto w = store.word(i);
        if (w != 0) return i * bits_per_word + static_cast<usize>(gf2::trailing_zeros(w));
    }
    return store.size();
}

/// Returns the number of trailing zeros in the store.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::zeros(27);
/// assert_eq(trailing_zeros(v), 27);
/// set(v, 0);
/// assert_eq(trailing_zeros(v), 26);
/// ```
template<BitStore Store>
constexpr usize
trailing_zeros(Store const& store) {
    if (is_empty(store)) return 0;

    // The last word may not be fully occupied so we need to subtract the number of unused bits.
    auto bits_per_word = BITS<typename Store::word_type>;
    auto tail_bits = store.size() % bits_per_word;
    auto unused_bits = tail_bits == 0 ? 0 : bits_per_word - tail_bits;
    auto num_words = store.words();
    auto i = num_words;
    while (i--) {
        auto w = store.word(i);
        if (w != 0) {
            auto whole_count = (num_words - i - 1) * bits_per_word;
            auto partial_count = static_cast<usize>(gf2::leading_zeros(w)) - unused_bits;
            return whole_count + partial_count;
        }
    }
    return store.size();
}

/// @}
/// @name Set-bit indices:
/// @{

/// Returns the index of the first set bit in the bit-store or `{}` if no bits are set.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::zeros(37);
/// assert(first_set(v) == std::optional<usize>{});
/// set(v, 2);
/// assert(first_set(v) == std::optional<usize>{2});
/// set(v, 2, false);
/// assert(first_set(v) == std::optional<usize>{});
/// set(v, 27);
/// assert(first_set(v) == std::optional<usize>{27});
/// BitVector empty;
/// assert(first_set(empty) == std::optional<usize>{});
/// ```
template<BitStore Store>
constexpr std::optional<usize>
first_set(Store const& store) {
    // Iterate forward looking for a word with a set bit and use the lowest of those ...
    // Remember that any unused bits in the final word are guaranteed to be unset.
    auto bits_per_word = BITS<typename Store::word_type>;
    for (auto i = 0uz; i < store.words(); ++i) {
        if (auto loc = lowest_set_bit(store.word(i))) return i * bits_per_word + loc.value();
    }
    return {};
}

/// Returns the index of the last set bit in the bit-store or `{}` if no bits are set.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::zeros(37);
/// assert(last_set(v) == std::optional<usize>{});
/// set(v, 2);
/// assert(last_set(v) == std::optional<usize>{2});
/// set(v, 27);
/// assert(last_set(v) == std::optional<usize>{27});
/// BitVector empty;
/// assert(last_set(empty) == std::optional<usize>{});
/// ```
template<BitStore Store>
constexpr std::optional<usize>
last_set(Store const& store) {
    // Iterate backward looking for a word with a set bit and use the highest of those ...
    // Remember that any unused bits in the final word are guaranteed to be unset.
    auto bits_per_word = BITS<typename Store::word_type>;
    for (auto i = store.words(); i--;) {
        if (auto loc = highest_set_bit(store.word(i))) return i * bits_per_word + loc.value();
    }
    return {};
}

/// Returns the index of the next set bit after `index` in the store or `{}` if no more set bits exist.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::zeros(37);
/// assert(next_set(v,0) == std::optional<usize>{});
/// set(v,2);
/// set(v,27);
/// assert(next_set(v,0) == std::optional<usize>{2});
/// assert(next_set(v,2) == std::optional<usize>{27});
/// assert(next_set(v,27) == std::optional<usize>{});
/// ```
template<BitStore Store>
constexpr std::optional<usize>
next_set(Store const& store, usize index) {
    // Start our search at index + 1.
    index = index + 1;

    // Perhaps we are off the end? (This also handles the case of an empty type).
    if (index >= store.size()) return {};

    // Where is that starting index located in the word store?
    using word_type = typename Store::word_type;
    auto bits_per_word = BITS<word_type>;
    auto [word_index, bit] = index_and_offset<word_type>(index);

    // Iterate forward looking for a word with a new set bit and use the lowest one ...
    for (auto i = word_index; i < store.words(); ++i) {
        auto w = store.word(i);
        if (i == word_index) {
            // First word -- turn all the bits before our starting bit into zeros so we don't see them as set.
            reset_bits(w, 0, bit);
        }
        if (auto loc = lowest_set_bit(w)) return i * bits_per_word + loc.value();
    }
    return {};
}

/// Returns the index of the previous set bit before `index` in the store or `{}` if there are none.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::zeros(37);
/// assert(previous_set(v,36) == std::optional<usize>{});
/// set(v,2);
/// set(v,27);
/// assert(previous_set(v,36) == std::optional<usize>{27});
/// assert(previous_set(v,27) == std::optional<usize>{2});
/// assert(previous_set(v,2)  == std::optional<usize>{});
/// ```
template<BitStore Store>
constexpr std::optional<usize>
previous_set(Store const& store, usize index) {
    // Edge case: If the store is empty or we are at the start, there are no previous set bits.
    if (is_empty(store) || index == 0) return {};

    // Silently fix large indices and also adjust the index down a slot.
    if (index > store.size()) index = store.size();
    index--;

    // Where is that starting index located in the word store?
    using word_type = typename Store::word_type;
    auto bits_per_word = BITS<word_type>;
    auto [word_index, bit] = index_and_offset<word_type>(index);

    // Iterate backwards looking for a word with a new set bit and use the highest one ...
    for (auto i = word_index + 1; i--;) {
        auto w = store.word(i);
        if (i == word_index) {
            // First word -- turn all higher bits after our starting bit into zeros so we don't see them as set.
            reset_bits(w, bit + 1, bits_per_word);
        }
        if (auto loc = highest_set_bit(w)) return i * bits_per_word + loc.value();
    }
    return {};
}

/// @}
/// @name Unset-bit Indices:
/// @{

/// Returns the index of the first unset bit in the bit-store or `{}` if no bits are unset.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::ones(37);
/// assert(first_unset(v) == std::optional<usize>{});
/// set(v,2,false);
/// assert(first_unset(v) == std::optional<usize>{2});
/// set(v,2);
/// assert(first_unset(v) == std::optional<usize>{});
/// set(v,27,false);
/// assert(first_unset(v) == std::optional<usize>{27});
/// BitVector empty;
/// assert(empty.first_unset() == std::optional<usize>{});
/// ```
template<BitStore Store>
constexpr std::optional<usize>
first_unset(Store const& store) {
    // Iterate forward looking for a word with a unset bit and use the lowest of those ...
    using word_type = typename Store::word_type;
    auto bits_per_word = BITS<word_type>;
    for (auto i = 0uz; i < store.words(); ++i) {
        auto w = store.word(i);
        if (i == store.words() - 1) {
            // Final word may have some unused zero bits that we need to replace with ones.
            auto last_occupied_bit = bit_offset<word_type>(store.size() - 1);
            set_bits(w, last_occupied_bit + 1, bits_per_word);
        }
        if (auto loc = lowest_unset_bit(w)) return i * bits_per_word + loc.value();
    }
    return {};
}

/// Returns the index of the last unset bit in the bit-store or `{}` if no bits are unset.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::ones(37);
/// assert(last_unset(v) == std::optional<usize>{});
/// set(v,2, false);
/// assert(last_unset(v) == std::optional<usize>{2});
/// set(v,2);
/// assert(last_unset(v) == std::optional<usize>{});
/// set(v,27, false);
/// assert(last_unset(v) == std::optional<usize>{27});
/// BitVector empty;
/// assert(empty.last_unset() == std::optional<usize>{});
/// ```
template<BitStore Store>
constexpr std::optional<usize>
last_unset(Store const& store) {
    // Iterate backwards looking for a word with a unset bit and use the highest of those ...
    using word_type = typename Store::word_type;
    auto bits_per_word = BITS<word_type>;
    for (auto i = store.words(); i--;) {
        auto w = store.word(i);
        if (i == store.words() - 1) {
            // Final word may have some unused zero bits that we need to replace with ones.
            auto last_occupied_bit = bit_offset<word_type>(store.size() - 1);
            set_bits(w, last_occupied_bit + 1, bits_per_word);
        }
        if (auto loc = highest_unset_bit(w)) return i * bits_per_word + loc.value();
    }
    return {};
}

/// Returns the index of the next unset bit after `index` in the store or `{}` if no more unset bits exist.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::ones(37);
/// assert(v.next_unset(0) == std::optional<usize>{});
/// set(v,2, false);
/// set(v,27, false);
/// assert(v.next_unset(0) == std::optional<usize>{2});
/// assert(v.next_unset(2) == std::optional<usize>{27});
/// assert(v.next_unset(27) == std::optional<usize>{});
/// BitVector empty;
/// assert(empty.next_unset(0) == std::optional<usize>{});
/// ```
template<BitStore Store>
constexpr std::optional<usize>
next_unset(Store const& store, usize index) {
    // Start our search at index + 1.
    index = index + 1;

    // Perhaps we are off the end? (This also handles the case of an empty type).
    if (index >= store.size()) return {};

    // Where is that starting index located in the word store?
    using word_type = typename Store::word_type;
    auto bits_per_word = BITS<word_type>;
    auto [word_index, bit] = index_and_offset<word_type>(index);

    // Iterate forward looking for a word with a new unset bit and use the lowest of those ...
    for (auto i = word_index; i < store.words(); ++i) {
        auto w = store.word(i);
        if (i == word_index) {
            // First word -- turn all the bits before our starting bit into ones so we don't see them as unset.
            set_bits(w, 0, bit);
        }
        if (i == store.words() - 1) {
            // Final word may have some unused zero bits that we need to replace with ones.
            auto last_occupied_bit = bit_offset<word_type>(store.size() - 1);
            set_bits(w, last_occupied_bit + 1, bits_per_word);
        }
        if (auto loc = lowest_unset_bit(w)) return i * bits_per_word + loc.value();
    }
    return {};
}

/// Returns the index of the previous unset bit before `index` in the store or `{}` if no more unset bits exist.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::ones(37);
/// assert(v.previous_unset(0) == std::optional<usize>{});
/// set(v,2, false);
/// set(v,27, false);
/// assert(v.previous_unset(36) == std::optional<usize>{27});
/// assert(v.previous_unset(27) == std::optional<usize>{2});
/// assert(v.previous_unset(2) == std::optional<usize>{});
/// BitVector empty;
/// assert(empty.previous_unset(0) == std::optional<usize>{});
/// ```
template<BitStore Store>
constexpr std::optional<usize>
previous_unset(Store const& store, usize index) {
    // Edge case: If the store is empty or we are at the start, there are no previous set bits.
    if (is_empty(store) || index == 0) return {};

    // Silently fix large indices and also adjust the index down a slot.
    if (index > store.size()) index = store.size();
    index--;

    // Where is that starting index located in the word store?
    using word_type = typename Store::word_type;
    auto bits_per_word = BITS<word_type>;
    auto [word_index, bit] = index_and_offset<word_type>(index);

    // Iterate backwards looking for a word with a new unset bit and use the highest of those ...
    for (auto i = word_index + 1; i--;) {
        auto w = store.word(i);
        if (i == word_index) {
            // First word -- set all the bits after our starting bit into ones so we don't see them as unset.
            set_bits(w, bit + 1, bits_per_word);
        }
        if (i == store.words() - 1) {
            // Final word may have some unused zero bits that we need to replace with ones.
            auto last_occupied_bit = bit_offset<word_type>(store.size() - 1);
            set_bits(w, last_occupied_bit + 1, bits_per_word);
        }
        if (auto loc = highest_unset_bit(w)) return i * bits_per_word + loc.value();
    }
    return {};
}

/// @}
/// @name Store Iterators:
/// @{

/// Returns a const iterator over the `bool` values of the bits in the const bit-store.
///
/// You can use this iterator to iterate over the bits in the store and get the values of each bit as a `bool`.
///
/// # Note
/// For the most part, try to avoid iterating through individual bits. It is much more efficient to use methods that
/// work on whole words of bits at a time.
///
/// # Example
/// ```
/// auto u = BitVector<u8>::ones(10);
/// for (auto&& bit : bits(u)) assert_eq(bit, true);
/// ```
template<BitStore Store>
constexpr auto
bits(Store const& store) {
    return Bits<const Store, true>(&store, 0);
}

/// Returns a non-const iterator over the values of the bits in the mutable bit-store.
///
/// You can use this iterator to iterate over the bits in the store to get *or* set the value of each bit.
///
/// # Note
/// For the most part, try to avoid iterating through individual bits. It is much more efficient to use methods that
/// work on whole words of bits at a time.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::zeros(10);
/// for (auto&& bit : bits(v)) bit = true;
/// assert_eq(to_string(v), "1111111111");
/// ```
template<BitStore Store>
constexpr auto
bits(Store& store) {
    return Bits<Store, false>(&store, 0);
}

/// Returns an iterator over the *indices* of any *set* bits in the bit-store.
///
/// You can use this iterator to iterate over the set bits in the store and get the index of each bit.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::alternating(10);
/// assert_eq(to_string(v), "1010101010");
/// auto indices = std::ranges::to<std::vector>(set_bits(v));
/// assert_eq(indices, (std::vector<usize>{0, 2, 4, 6, 8}));
/// ```
template<BitStore Store>
constexpr auto
set_bits(Store const& store) {
    return SetBits<Store>(&store);
}

/// Returns an iterator over the *indices* of any *unset* bits in the bit-store.
///
/// You can use this iterator to iterate over the unset bits in the store and get the index of each bit.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::alternating(10);
/// assert_eq(to_string(v), "1010101010");
/// auto indices = std::ranges::to<std::vector>(unset_bits(v));
/// assert_eq(indices, (std::vector<usize>{1, 3, 5, 7, 9}));
/// ```
template<BitStore Store>
constexpr auto
unset_bits(Store const& store) {
    return UnsetBits<Store>(&store);
}

/// Returns a const iterator over all the *words* underlying the bit-store.
///
/// You can use this iterator to iterate over the words in the store and read the `Word` value of each word.
/// You **cannot** use this iterator to modify the words in the store.
///
/// # Note
/// The words here may be a synthetic construct. The expectation is that the bit `0` in the store is located at the
/// bit-location `0` of `word(0)`. That is always the case for bit-vectors but bit-slices typically synthesise "words"
/// on the fly from adjacent pairs of real underlying words. Nevertheless, almost all the methods in `BitStore` are
/// implemented efficiently by operating on those words.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::ones(10);
/// assert_eq(to_string(v), "1111111111");
/// auto words = std::ranges::to<std::vector>(store_words(v));
/// assert_eq(words, (std::vector<u8>{0b1111'1111, 0b0000'0011}));
/// ```
template<BitStore Store>
constexpr auto
store_words(Store const& store) {
    return Words<Store>(&store);
}

/// Returns a copy of the words underlying this bit-store and puts them into the passed output iterator.
///
/// # Note
/// 1. The last word in the store may not be fully occupied but unused slots will be all zeros.
/// 2. The output iterator must be able to accept values of the store's `word_type`.
/// 3. The output iterator must have enough space to accept all the words in the store.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::ones(10);
/// std::vector<u8> out8(v.words());
/// to_words(v, out8.begin());
/// assert_eq(out8, (std::vector<u8>{0b1111'1111, 0b0000'0011}));
/// std::vector<u16> out16(v.words());
/// to_words(v, out16.begin());
/// assert_eq(out16, (std::vector<u16>{0b1111'1111, 0b0000'0011}));
/// ```
template<BitStore Store>
constexpr void
to_words(Store const& store, std::output_iterator<typename Store::word_type> auto out) {
    for (auto i = 0uz; i < store.words(); ++i, ++out) *out = store.word(i);
}

/// Returns a copy of the words underlying this bit-store as new `std::vector`.
///
/// # Note
/// - The last word in the store may not be fully occupied but unused slots will be all zeros.
/// - The returned `std::vector`'s element type will be the same as the store's `word_type`.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::ones(10);
/// auto words = to_words(v);
/// assert_eq(words, (std::vector<u8>{0b1111'1111, 0b0000'0011}));
/// ```
template<BitStore Store>
constexpr auto
to_words(Store const& store) {
    return std::ranges::to<std::vector>(store_words(store));
}

/// @}
/// @name Store Spans:
/// @{

/// Constructs a read-only bit-span over the const bit-store `store` for its bits in the range `[begin, end)`.
///
/// It is the responsibility of the caller to ensure that the underlying bit-store continues to exist for as long as the
/// `BitSpan` is in use.
///
/// # Panics
/// This method panics if the span range is invalid.
///
/// # Example
/// ```
/// const auto v = BitVector<>::alternating(10);
/// auto s = span(v, 1, 5);
/// assert_eq(to_string(s), "0101");
/// ```
template<BitStore Store>
static constexpr auto
span(Store const& store, usize begin, usize end) {
    // Check that the range is correctly formed & does not extend beyond the end of the bit-store.
    gf2_assert(begin <= end, "Span range [{}, {}) is mis-ordered.", begin, end);
    gf2_assert(end <= store.size(), "Span end {} extends beyond the store end {}.", end, store.size());

    // Get the zero-based word index and bit offset in that word for the begin location.
    using word_type = typename Store::word_type;
    auto bits_per_word = BITS<word_type>;
    auto [data_index, bit_offset] = index_and_offset<word_type>(begin);

    // If the store is already a span, we need to adjust those values.
    bit_offset += store.offset();
    if (bit_offset >= bits_per_word) {
        data_index += 1;
        bit_offset %= bits_per_word;
    }

    // Return the read-only bit-span over the underlying word data.
    return BitSpan<const word_type>(store.store() + data_index, bit_offset, end - begin);
}

/// Constructs a bit-span over the bit-store `store` for its bits in the range `[begin, end)`.
/// This span allows modification of the underlying bits (unless its created from a span that is itself const).
///
/// It is the responsibility of the caller to ensure that the underlying bit-store continues to exist for as long as the
/// `BitSpan` is in use.
///
/// # Panics
/// This method panics if the span range is invalid.
///
/// # Example
/// ```
/// const auto v = BitVector<>::alternating(10);
/// auto s = span(v, 1, 5);
/// assert_eq(to_string(s), "0101");
/// ```
template<BitStore Store>
static constexpr auto
span(Store& store, usize begin, usize end) {
    // Check that the range is correctly formed & does not extend beyond the end of the bit-store.
    gf2_assert(begin <= end, "Span range [{}, {}) is mis-ordered.", begin, end);
    gf2_assert(end <= store.size(), "Span end {} extends beyond the store end {}.", end, store.size());

    // Get the zero-based word index and bit offset in that word for the begin location.
    using word_type = typename Store::word_type;
    auto bits_per_word = BITS<word_type>;
    auto [data_index, bit_offset] = index_and_offset<word_type>(begin);

    // If the store is already a span, we need to adjust those values.
    bit_offset += store.offset();
    if (bit_offset >= bits_per_word) {
        data_index += 1;
        bit_offset %= bits_per_word;
    }

    // Return the read-only bit-span over the underlying word data.
    return BitSpan<word_type>(store.store() + data_index, bit_offset, end - begin);
}

/// @}
/// @name Sub-vectors:
/// @{

/// Returns a *clone* of the elements in the half-open range `[begin, end)` as a new bit-vector.
///
/// # Panics
/// This method panics if the span range is invalid.
///
/// # Example
/// ```
/// auto v = BitVector<>::alternating(10);
/// auto s = sub(v,1,5);
/// assert_eq(to_string(s), "0101");
/// s.set_all();
/// assert_eq(to_string(s), "1111");
/// assert_eq(to_string(v), "1010101010");
/// ```
template<BitStore Store>
constexpr auto
sub(Store const& store, usize begin, usize end) {
    return BitVector<typename Store::word_type>::from(gf2::span(store, begin, end));
}

/// @}
/// @name Store Splits and Joins:
/// @{

/// Views a bit-store as two parts containing the elements `[0, at)` and `[at, size())` respectively.
/// Clones of the parts are stored in the passed bit-vectors `left` and `right`.
///
/// On return, `left` contains the bits from the start of the bit-vector up to but not including `at` and `right`
/// contains the bits from `at` to the end of the bit-vector. This bit-vector itself is not modified.
///
/// This lets one reuse the `left` and `right` destinations without having to allocate new bit-vectors.
/// This is useful when implementing iterative algorithms that need to split a bit-vector into two parts repeatedly.
///
/// # Panics
/// This method panics if the split point is beyond the end of the bit-vector.
///
/// # Example
/// ```
/// auto v = BitVector<>::alternating(10);
/// BitVector left, right;
/// split(v, 5, left, right);
/// assert_eq(to_string(left), "10101");
/// assert_eq(to_string(right), "01010");
/// assert_eq(to_string(v), "1010101010");
/// ```
template<BitStore Store>
constexpr void
split(Store const& store, usize at, BitVector<typename Store::word_type>& left,
      BitVector<typename Store::word_type>& right) {
    auto sz = store.size();
    gf2_assert(at <= sz, "Oops, split point {} is beyond the end of the bit-store {}", at, sz);
    left.clear();
    right.clear();
    left.append(span(store, 0, at));
    right.append(span(store, at, sz));
}

/// Views a bit-store as two parts containing the elements `[0, at)` and `[at, size())` respectively.
/// Clones of the parts are returned as a pair of new bit-vectors [`left`, `right`].
///
/// On return, `left` is a clone of the bits from the start of the bit-vector up to but not including `at` and
/// `right` contains the bits from `at` to the end of the bit-vector. This bit-vector itself is not modified.
///
/// # Panics
/// This method panics if the split point is beyond the end of the bit-vector.
///
/// # Example
/// ```
/// auto v = BitVector<>::alternating(10);
/// auto [left, right] = split(v, 5);
/// assert_eq(to_string(left), "10101");
/// assert_eq(to_string(right), "01010");
/// assert_eq(to_string(v), "1010101010");
/// ```
template<BitStore Store>
constexpr auto
split(Store const& store, usize at) {
    BitVector<typename Store::word_type> left, right;
    split(store, at, left, right);
    return std::pair{left, right};
}

/// Returns a new bit-vector formed by joining two bit-stores `lhs` and `rhs`.
///
/// This is one of the few methods in library that allows the operands to have different word types.
/// The returned bit-vector will use the same word type as `lhs`.
///
/// # Example
/// ```
/// auto lhs = BitVector<u16>::zeros(12);
/// auto rhs = BitVector<u8>::ones(12);
/// auto v = join(lhs, rhs);
/// assert_eq(to_string(v), "000000000000111111111111");
/// ```
template<BitStore Lhs, BitStore Rhs>
auto
join(Lhs const& lhs, Rhs const& rhs) {
    auto lhs_size = lhs.size();
    auto rhs_size = rhs.size();

    using word_type = typename Lhs::word_type;
    BitVector<word_type> result(lhs_size + rhs_size);

    result.span(0, lhs_size).copy(lhs);
    result.span(lhs_size, lhs_size + rhs_size).copy(rhs);
    return result;
}

/// @}
/// @name Interleaving With Zeros:
/// @{

/// Interleaves the bits of a bit-store with zeros storing the result into the passed bit-vector `dst`.
///
/// On return, `dst` will have the bits of this bit-store interleaved with zeros. For example, if this
/// bit-store has the bits `abcde` then `dst` will have the bits `a0b0c0d0e`.
///
/// **Note:** There is no last zero bit in `dst`.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::ones(10);
/// BitVector<u8> dst;
/// riffle(v, dst);
/// assert_eq(to_string(dst), "1010101010101010101");
/// ```
template<BitStore Store>
constexpr void
riffle(Store const& store, BitVector<typename Store::word_type>& dst) {
    // Nothing to do if the store is empty or has just one element. With two bits `ab` we fill `dst` with `a0b`.
    auto sz = store.size();
    if (sz < 2) {
        dst.resize(sz);
        dst.copy(store);
        return;
    }

    // Make sure `dst` is large enough to hold the riffled bits (a bit too big but we fix that below).
    dst.resize(2 * sz);
    auto dst_words = dst.words();

    // Riffle each word in the store into two adjacent words in `dst`.
    for (auto i = 0uz; i < store.words(); ++i) {
        auto [lo, hi] = riffle(store.word(i));
        dst.set_word(2 * i, lo);

        // Note that `hi` may be completely superfluous ...
        if (2 * i + 1 < dst_words) dst.set_word(2 * i + 1, hi);
    }

    // If this bit-store was say `abcde` then `dst` will now be `a0b0c0d0e0`. Pop the last 0.
    dst.pop();
}

/// Returns a new bit-vector that is the result of riffling the bits in this bit-store with zeros.
///
/// If bit-store has the bits `abcde` then the output bit-vector will have the bits `a0b0c0d0e`.
///
/// **Note:** There is no last zero bit in `dst`.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::ones(10);
/// auto dst = riffle(v);
/// assert_eq(to_string(dst), "1010101010101010101");
/// ```
template<BitStore Store>
constexpr auto
riffle(Store const& store) {
    BitVector<typename Store::word_type> result;
    riffle(store, result);
    return result;
}

/// @}
/// @name String Representations:
/// @{

/// Returns a binary string representation of a store.
///
/// The string is formatted as a sequence of `0`s and `1`s with the least significant bit on the right.
///
/// @param store The bit-store to convert to a binary string.
/// @param sep The separator between bit elements which defaults to no separator.
/// @param pre The prefix to add to the string which defaults to no prefix.
/// @param post The postfix to add to the string which defaults to no postfix.
///
/// # Example
/// ```
/// BitVector v{10};
/// assert_eq(to_binary_string(v), "0000000000");
/// set(v, 0);
/// assert_eq(to_binary_string(v), "1000000000");
/// assert_eq(to_binary_string(v, ",", "[", "]"), "[1,0,0,0,0,0,0,0,0,0]");
/// ```
template<BitStore Store>
static std::string
to_binary_string(Store const& store, std::string_view sep = "", std::string_view pre = "", std::string_view post = "") {
    // Edge case ...
    if (is_empty(store)) { return std::format("{}{}", pre, post); }

    // Start with the raw binary string by iterating over the words ...
    // We preallocate enough space to hold the entire string (actually this is usually a bit bigger than we need).
    auto        num_words = store.words();
    std::string binary_string;
    binary_string.reserve(num_words * BITS<typename Store::word_type>);
    for (auto i = 0uz; i < num_words; ++i) {
        auto w = reverse_bits(store.word(i));
        binary_string.append(gf2::to_binary_string(w));
    }

    // The last word may not be fully occupied and we need to remove the spurious zeros
    binary_string.resize(store.size());

    // If there is no separator, return early ...
    if (sep.empty()) { return std::format("{}{}{}", pre, binary_string, post); }

    // Build `result` with separators between each character ...
    std::string result;
    result.reserve(pre.size() + store.size() * (sep.size() + 1) + post.size());
    result.append(pre);
    for (auto i = 0uz; i < store.size(); ++i) {
        result.push_back(binary_string[i]);
        if (i != store.size() - 1) { result.append(sep); }
    }
    result.append(post);
    return result;
}

/// Returns a binary string representation of a store.
///
/// The string is formatted as a sequence of `0`s and `1`s with the least significant bit on the right.
///
/// @param store The bit-store to convert to a binary string.
/// @param sep The separator between bit elements which defaults to no separator.
/// @param pre The prefix to add to the string which defaults to no prefix.
/// @param post The postfix to add to the string which defaults to no postfix.
///
/// # Example
/// ```
/// BitVector v{10};
/// assert_eq(to_string(v), "0000000000");
/// set(v, 0);
/// assert_eq(to_string(v), "1000000000");
/// assert_eq(to_string(v, ",", "[", "]"), "[1,0,0,0,0,0,0,0,0,0]");
/// ```
template<BitStore Store>
static std::string
to_string(Store const& store, std::string_view sep = "", std::string_view pre = "", std::string_view post = "") {
    return to_binary_string(store, sep, pre, post);
}

/// Returns a "pretty" string representation of a store.
///
/// The output is a string of 0's and 1's with spaces between each bit, and the whole thing enclosed in square
/// brackets.
///
/// # Example
/// ```
/// auto v = BitVector<>::alternating(10);
/// assert_eq(to_pretty_string(v), "[1,0,1,0,1,0,1,0,1,0]");
/// BitVector empty;
/// assert_eq(to_pretty_string(empty), "[]");
/// ```
template<BitStore Store>
static std::string
to_pretty_string(Store const& store) {
    return to_binary_string(store, ",", "[", "]");
}

/// Returns the "hex" string representation of the bits in the bit-store
///
/// The output is a string of hex characters without any spaces, commas, or other formatting.
///
/// The string may have a two character *suffix* of the form ".base" where `base` is one of 2, 4 or 8. <br>
/// All hex characters encode 4 bits: "0X0" -> `0b0000`, "0X1" -> `0b0001`, ..., "0XF" -> `0b1111`. <br>
/// The three possible ".base" suffixes allow for bit-vectors whose length is not a multiple of 4. <br>
/// Empty bit-vectors are represented as the empty string.
///
/// - `0X1`   is the hex representation of the bit-vector `0001` => length 4.
/// - `0X1.8` is the hex representation of the bit-vector `001`  => length 3.
/// - `0X1.4` is the hex representation of the bit-vector `01`   => length 2.
/// - `0X1.2` is the hex representation of the bit-vector `1`    => length 1.
///
/// The output is in *vector-order*.
/// If "h0" is the first hex digit in the output string, you can print it as four binary digits `v_0v_1v_2v_3`.
/// For example, if h0 = "A" which is `1010` in binary, then v = 1010.
///
/// # Example
/// ```
/// BitVector v0;
/// assert_eq(to_hex_string(v0), "");
/// auto v1 = BitVector<>::ones(4);
/// assert_eq(to_hex_string(v1), "F");
/// auto v2 = BitVector<>::ones(5);
/// assert_eq(to_hex_string(v2), "F1.2");
/// auto v3 = BitVector<>::alternating(8);
/// assert_eq(to_binary_string(v3), "10101010");
/// assert_eq(to_hex_string(v3), "AA");
/// ```
template<BitStore Store>
static std::string
to_hex_string(Store const& store) {
    // Edge case: No bits in the store, return the empty string.
    if (is_empty(store)) return "";

    // The number of digits in the output string. Generally hexadecimal but the last may be to a lower base.
    auto digits = (store.size() + 3) / 4;

    // Preallocate space allowing for a possible lower base on the last digit such as "_2".
    std::string result;
    result.reserve(digits + 2);

    //  Reverse the bits in each word to vector-order and get the fully padded hex string representation.
    using word_type = typename Store::word_type;
    for (auto i = 0uz; i < store.words(); ++i) {
        auto w = reverse_bits(store.word(i));
        result.append(gf2::to_hex_string<word_type>(w));
    }

    // Last word may not be fully occupied and padded with spurious zeros so we truncate the output string.
    result.resize(digits);

    // Every four elements is encoded by a single hex digit but `size()` may not be a multiple of 4.
    auto k = store.size() % 4;
    if (k != 0) {
        // That last hex digit should really be encoded to a lower base -- 2, 4 or 8.
        // We compute the number represented by the trailing `k` elements in the bit-vector.
        int num = 0;
        for (auto i = 0uz; i < k; ++i)
            if (get(store, store.size() - 1 - i)) num |= 1 << i;

        // Convert that number to hex & use it to *replace* the last hex digit in our `result` string.
        // Also append the appropriate base to the output string so that the last digit can be interpreted properly.
        result.resize(result.size() - 1);
        result.append(std::format("{:X}.{}", num, 1 << k));
    }
    return result;
}

/// Detailed Description of a Bit-Store
///
/// This method is useful for debugging but you should not rely on the output format which may change.
template<BitStore Store>
static std::string
describe(Store const& store) {
    using word_type = typename Store::word_type;
    auto        bits_per_word = BITS<word_type>;
    std::string result;
    result.reserve(1000);
    result += std::format("binary format:        {}\n", to_binary_string(store));
    result += std::format("hex format:           {}\n", to_hex_string(store));
    result += std::format("number of bits:       {}\n", store.size());
    result += std::format("number of set bits:   {}\n", count_ones(store));
    result += std::format("number of unset bits: {}\n", count_zeros(store));
    result += std::format("bits per word:        {}\n", bits_per_word);
    result += std::format("word count:           {}\n", store.words());
    result += "words in hex:         [";
    for (auto i = 0uz; i < store.words(); ++i) {
        auto w = store.word(i);
        result += std::format("{:0{}X}", w, (bits_per_word + 3) / 4);
        result += std::format("{:x}", store.word(i));
        if (i + 1 < store.words()) result += ", ";
    }
    result += "]\n";
    return result;
}

/// The usual output stream operator for a bit-store
template<BitStore Store>
inline std::ostream&
operator<<(Store const& store, std::ostream& s) {
    return s << to_string(store);
}

/// @}
/// @name The Equality Operator:
/// @{

/// Checks that any pair of bit-stores are equal in content.
///
/// Equality here means that the two bit-stores have identical content and *also* the same underlying word_type word.
///
// # Example
/// ```
/// auto u = BitVector<u8>::ones(55);
/// auto v = BitVector<u8>::ones(55);
/// assert(u == v);
/// v.set(23, false);
/// assert(u != v);
/// ```
template<BitStore Lhs, BitStore Rhs>
constexpr bool
operator==(Lhs const& lhs, Rhs const& rhs) {
    if constexpr (!std::same_as<typename Lhs::word_type, typename Rhs::word_type>) return false;
    if (&lhs != &rhs) {
        if (lhs.size() != rhs.size()) return false;
        for (auto i = 0uz; i < lhs.words(); ++i)
            if (lhs.word(i) != rhs.word(i)) return false;
    }
    return true;
}

/// @}
/// @name In-place Bit Shifts:
/// @{

/// In-place left shift of a bit-store by `shift` bits.
///
/// Shifting is in *vector-order* so if `v = [v0,v1,v2,v3]` then `v <<= 1` is `[v1,v2,v3,0]` with zeros added to
/// the right. Left shifting in vector-order is the same as right shifting in bit-order.
///
/// **Note:** Only accessible bits are affected by the shift.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::ones(20);
/// v <<= 8;
/// assert_eq(to_string(v), "11111111111100000000");
/// ```
template<BitStore Store>
constexpr void
operator<<=(Store& store, usize shift) {
    // Edge cases where there is nothing to do.
    if (shift == 0 || store.size() == 0) return;

    // Perhaps we are shifting all the bits out and are left with all zeros.
    if (shift >= store.size()) {
        set_all(store, false);
        return;
    }

    // For larger shifts, we can efficiently shift by whole words first.
    using word_type = typename Store::word_type;
    auto  bits_per_word = BITS<word_type>;
    usize word_shift = shift / bits_per_word;
    usize end_word = store.words() - word_shift;

    // Do the whole word shifts first, pushing in zero words to fill the empty slots.
    if (word_shift > 0) {
        // Shift whole words -- starting at the beginning of the store.
        for (auto i = 0uz; i < end_word; ++i) store.set_word(i, store.word(i + word_shift));

        // Fill in the high order words with zeros.
        for (auto i = end_word; i < store.words(); ++i) store.set_word(i, 0);

        // How many bits are left to shift?
        shift -= word_shift * bits_per_word;
    }

    // Perhaps there are some partial word shifts left to do.
    if (shift != 0) {
        // Do the "interior" words where the shift moves bits from one word to the next.
        if (end_word > 0) {
            auto shift_complement = bits_per_word - shift;
            for (auto i = 0uz; i < end_word - 1; ++i) {
                auto lo = static_cast<word_type>(store.word(i) >> shift);
                auto hi = static_cast<word_type>(store.word(i + 1) << shift_complement);
                store.set_word(i, lo | hi);
            }
        }

        // Do the last word.
        auto value = store.word(end_word - 1);
        store.set_word(end_word - 1, static_cast<word_type>(value >> shift));
    }
}

/// In-place right shift of a store by `shift` bits.
///
/// Shifting is in *vector-order* so if `v = [v0,v1,v2,v3]` then `v >>= 1` is `[0,v0,v1,v2]` with zeros added to
/// the left. Right shifting in vector-order is the same as left shifting in bit-order.
///
/// **Note:** Only accessible bits are affected by the shift.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::ones(20);
/// v >>= 8;
/// assert_eq(to_string(v), "00000000111111111111");
/// ```
template<BitStore Store>
constexpr void
operator>>=(Store& store, usize shift) {
    // Edge cases where there is nothing to do.
    if (shift == 0 || store.size() == 0) return;

    // Perhaps we are shifting all the bits out and are left with all zeros.
    if (shift >= store.size()) {
        set_all(store, false);
        return;
    }

    // For larger shifts, we can efficiently shift by whole words first.
    using word_type = typename Store::word_type;
    auto  bits_per_word = BITS<word_type>;
    usize word_shift = shift / bits_per_word;

    // Do the whole word shifts first, pushing in zero words to fill the empty slots.
    if (word_shift > 0) {
        // Shift whole words -- starting at the end of the store.
        for (auto i = store.words(); i-- > word_shift;) store.set_word(i, store.word(i - word_shift));

        // Fill in the low order words with zeros.
        for (auto i = 0uz; i < word_shift; ++i) store.set_word(i, 0);

        // How many bits are left to shift?
        shift -= word_shift * bits_per_word;
    }

    // Perhaps there are some partial word shifts left to do.
    if (shift != 0) {
        // Do the "interior" words where the shift moves bits from one word to the next.
        auto shift_complement = bits_per_word - shift;
        for (auto i = store.words(); i-- > word_shift + 1;) {
            auto lo = static_cast<word_type>(store.word(i - 1) >> shift_complement);
            auto hi = static_cast<word_type>(store.word(i) << shift);
            store.set_word(i, lo | hi);
        }
    }

    // Do the "first" word.
    auto value = store.word(word_shift);
    store.set_word(word_shift, static_cast<word_type>(value << shift));
}

/// @}
/// @name Out-of-place Bit Shifts:
/// @{

/// Returns a new bit-vector that is the store shifted left by `shift` bits.
///
/// Shifting is in *vector-order* so if `v = [v0,v1,v2,v3]` then `v << 1` is `[v1,v2,v3,0]` with zeros added to
/// the right. Left shifting in vector-order is the same as right shifting in bit-order.
///
/// **Note:** Only accessible bits are affected by the shift.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::ones(20);
/// auto w = v << 8;
/// assert_eq(to_string(w), "11111111111100000000");
/// ```
template<BitStore Store>
constexpr auto
operator<<(Store const& store, usize shift) {
    auto result = BitVector<typename Store::word_type>::from(store);
    result <<= shift;
    return result;
}

/// Returns a new bit-vector that is `lhs` shifted right by `shift` bits.
///
/// Shifting is in *vector-order* so if `v = [v0,v1,v2,v3]` then `v >>= 1` is `[0,v0,v1,v2]` with zeros added to
/// the left. Right shifting in vector-order is the same as left shifting in bit-order.
///
/// **Note:** Only accessible bits are affected by the shift.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::ones(20);
/// auto w = v >> 8;
/// assert_eq(to_string(w), "00000000111111111111");
/// ```
template<BitStore Store>
constexpr auto
operator>>(Store const& store, usize shift) {
    auto result = BitVector<typename Store::word_type>::from(store);
    result >>= shift;
    return result;
}

/// @}
/// @name In-place Bitwise Operations:
/// @{

/// In-place `XOR` of one bit-store with an equal-sized bit-store.
///
/// **Note:** Only accessible bits are affected by the shift.
///
/// # Example
/// ```
/// auto v1 = BitVector<u8>::alternating(10);
/// v1 ^= ~v1;
/// assert_eq(to_string(v1), "1111111111");
/// ```
template<BitStore Lhs, BitStore Rhs>
    requires std::same_as<typename Lhs::word_type, typename Rhs::word_type>
constexpr void
operator^=(Lhs& lhs, Rhs const& rhs) {
    gf2_assert(lhs.size() == rhs.size(), "Lengths do not match: {} != {}.", lhs.size(), rhs.size());
    for (auto i = 0uz; i < lhs.words(); ++i) lhs.set_word(i, lhs.word(i) ^ rhs.word(i));
}

/// In-place `AND` of one bit-store with an equal-sized bit-store.
///
/// # Panics
/// In debug mode, this method panics if the lengths of the two bit-stores do not match.
///
/// # Example
/// ```
/// auto v1 = BitVector<u8>::alternating(10);
/// v1 &= ~v1;
/// assert_eq(to_string(v1), "0000000000");
/// ```
template<BitStore Lhs, BitStore Rhs>
    requires std::same_as<typename Lhs::word_type, typename Rhs::word_type>
constexpr void
operator&=(Lhs& lhs, Rhs const& rhs) {
    gf2_assert(lhs.size() == rhs.size(), "Lengths do not match: {} != {}.", lhs.size(), rhs.size());
    for (auto i = 0uz; i < lhs.words(); ++i) lhs.set_word(i, lhs.word(i) & rhs.word(i));
}

/// In-place `OR` of one bit-store with an equal-sized bit-store.
///
/// # Panics
/// In debug mode, this method panics if the lengths of the two bit-stores do not match.
///
/// # Example
/// ```
/// auto v1 = BitVector<u8>::alternating(10);
/// v1 |= ~v1;
/// assert_eq(to_string(v1), "1111111111");
/// ```
template<BitStore Lhs, BitStore Rhs>
    requires std::same_as<typename Lhs::word_type, typename Rhs::word_type>
constexpr void
operator|=(Lhs& lhs, Rhs const& rhs) {
    gf2_assert(lhs.size() == rhs.size(), "Lengths do not match: {} != {}.", lhs.size(), rhs.size());
    for (auto i = 0uz; i < lhs.words(); ++i) lhs.set_word(i, lhs.word(i) | rhs.word(i));
}

/// @}
/// @name Out-of-place Bitwise Operations:
/// @{

/// Returns a new bit-vector that has the same bits as a bit-store but with all the bits flipped.
///
/// # Example
/// ```
/// auto v = BitVector<u8>::alternating(10);
/// assert_eq(to_string(v), "1010101010");
/// auto w = ~v;
/// assert_eq(to_string(w), "0101010101");
/// ```
template<BitStore Store>
constexpr auto
operator~(Store const& store) {
    auto result = BitVector<typename Store::word_type>::from(store);
    result.flip_all();
    return result;
}

/// Returns the `XOR` of two equal-sized bit-stores as a new bit-vector.
///
/// # Panics
/// In debug mode, this method panics if the lengths of the two bit-stores do not match.
///
/// # Example
/// ```
/// auto v1 = BitVector<u8>::alternating(10);
/// auto v2 = ~v1;
/// auto v3 = v1 ^ v2;
/// assert_eq(to_string(v3), "1111111111");
/// ```
template<BitStore Lhs, BitStore Rhs>
    requires std::same_as<typename Lhs::word_type, typename Rhs::word_type>
constexpr auto
operator^(Lhs const& lhs, Rhs const& rhs) {
    auto result = BitVector<typename Lhs::word_type>::from(lhs);
    result ^= rhs;
    return result;
}

/// Returns the `AND` of two equal-sized bit-stores as a new bit-vector.
///
/// # Panics
/// In debug mode, this method panics if the lengths of the two bit-stores do not match.
///
/// # Example
/// ```
/// auto v1 = BitVector<u8>::alternating(10);
/// auto v2 = ~v1;
/// auto v3 = v1 & v2;
/// assert_eq(to_string(v3), "0000000000");
/// ```
template<BitStore Lhs, BitStore Rhs>
    requires std::same_as<typename Lhs::word_type, typename Rhs::word_type>
constexpr auto
operator&(Lhs const& lhs, Rhs const& rhs) {
    auto result = BitVector<typename Lhs::word_type>::from(lhs);
    result &= rhs;
    return result;
}

/// Returns the `OR` of two equal-sized bit-stores as a new bit-vector.
///
/// # Panics
/// In debug mode, this method panics if the lengths of the two bit-stores do not match.
///
/// # Example
/// ```
/// auto v1 = BitVector<u8>::alternating(10);
/// auto v2 = ~v1;
/// auto v3 = v1 | v2;
/// assert_eq(to_string(v3), "1111111111");
/// ```
template<BitStore Lhs, BitStore Rhs>
    requires std::same_as<typename Lhs::word_type, typename Rhs::word_type>
constexpr auto
operator|(Lhs const& lhs, Rhs const& rhs) {
    auto result = BitVector<typename Lhs::word_type>::from(lhs);
    result |= rhs;
    return result;
}

/// @}
/// @name In-place Arithmetic Operations:
/// @{

/// In-place addition of one bit-store with an equal-sized bit-store.
///
/// In GF(2) addition is the same as `XOR`.
///
/// # Panics
/// In debug mode, this method panics if the lengths of the two bit-stores do not match.
///
/// # Example
/// ```
/// auto v1 = BitVector<u8>::alternating(10);
/// v1 += ~v1;
/// assert_eq(to_string(v1), "1111111111");
/// ```
template<BitStore Lhs, BitStore Rhs>
    requires std::same_as<typename Lhs::word_type, typename Rhs::word_type>
constexpr void
operator+=(Lhs& lhs, Rhs const& rhs) {
    lhs ^= rhs;
}

/// In-place difference of one bit-store with an equal-sized bit-store.
///
/// In GF(2) difference is the same as `XOR`.
///
/// # Panics
/// In debug mode, this method panics if the lengths of the two bit-stores do not match.
///
/// # Example
/// ```
/// auto v1 = BitVector<u8>::alternating(10);
/// v1 -= ~v1;
/// assert_eq(to_string(v1), "1111111111");
/// ```
template<BitStore Lhs, BitStore Rhs>
    requires std::same_as<typename Lhs::word_type, typename Rhs::word_type>
constexpr void
operator-=(Lhs& lhs, Rhs const& rhs) {
    lhs ^= rhs;
}

/// @}
/// @name Out-of-place Arithmetic Operations:
/// @{

/// Adds two equal-sized bit-stores and returns the result as a new bit-vector.
///
/// In GF(2) addition is the same as `XOR`.
///
/// # Panics
/// In debug mode, this method panics if the lengths of the two bit-stores do not match.
///
/// # Example
/// ```
/// auto v1 = BitVector<u8>::alternating(10);
/// auto v2 = ~v1;
/// auto v3 = v1 + v2;
/// assert_eq(to_string(v3), "1111111111");
/// ```
template<BitStore Lhs, BitStore Rhs>
    requires std::same_as<typename Lhs::word_type, typename Rhs::word_type>
constexpr auto
operator+(Lhs const& lhs, Rhs const& rhs) {
    auto result = BitVector<typename Lhs::word_type>::from(lhs);
    result += rhs;
    return result;
}

/// Subtracts two equal-sized bit-stores and returns the result as a new bit-vector.
///
/// In GF(2) addition is the same as `XOR`.
///
/// # Panics
/// In debug mode, this method panics if the lengths of the two bit-stores do not match.
///
/// # Example
/// ```
/// auto v1 = BitVector<u8>::alternating(10);
/// auto v2 = ~v1;
/// auto v3 = v1 - v2;
/// assert_eq(to_string(v3), "1111111111");
/// ```
template<BitStore Lhs, BitStore Rhs>
    requires std::same_as<typename Lhs::word_type, typename Rhs::word_type>
constexpr auto
operator-(Lhs const& lhs, Rhs const& rhs) {
    auto result = BitVector<typename Lhs::word_type>::from(lhs);
    result -= rhs;
    return result;
}

/// @}
/// Dot Products:
/// @{

/// Returns the dot product of `lhs` and `rhs` as a boolean value.
///
/// For any pair of vector-like types, the dot product is: $u * v = \sum_i u_i v_i $ where the sum is over all the
/// indices so the two operands must have the same length. In GF(2) the sum is modulo 2 and, by convention, the scalar
/// output is a boolean value.
///
/// # Panics
/// In debug mode, this method panics if the lengths of the two bit-stores do not match.
///
/// # Example
/// ```
/// auto v1 = BitVector<u8>::alternating(10);
/// auto v2 = ~v1;
/// assert_eq(dot(v1, v1), true);
/// assert_eq(dot(v1, v2), false);
/// ```
template<BitStore Lhs, BitStore Rhs>
    requires std::same_as<typename Lhs::word_type, typename Rhs::word_type>
constexpr bool
dot(Lhs const& lhs, Rhs const& rhs) {
    gf2_debug_assert_eq(lhs.size(), rhs.size(), "Length mismatch {} != {}", lhs.size(), rhs.size());
    using word_type = typename Lhs::word_type;
    auto sum = word_type{0};
    for (auto i = 0uz; i < lhs.words(); ++i) { sum ^= static_cast<word_type>(lhs.word(i) & rhs.word(i)); }
    return count_ones(sum) % 2 == 1;
}

/// Operator `*` returns the dot product of `lhs` and `rhs` as a boolean value.
///
/// For any pair of vector-like types, the dot product is: $u * v = \sum_i u_i v_i $ where the sum is over all the
/// indices so the two operands must have the same length. In GF(2) the sum is modulo 2 and, by convention, the scalar
/// output is a boolean value.
///
/// # Panics
/// In debug mode, this method panics if the lengths of the two bit-stores do not match.
///
/// # Example
/// ```
/// auto v1 = BitVector<u8>::alternating(10);
/// auto v2 = ~v1;
/// assert_eq(v1*v1, true);
/// assert_eq(v1*v2, false);
/// ```
template<BitStore Lhs, BitStore Rhs>
    requires std::same_as<typename Lhs::word_type, typename Rhs::word_type>
constexpr bool
operator*(Lhs const& lhs, Rhs const& rhs) {
    return dot(lhs, rhs);
}

/// @}
/// Convolutions:
/// @{

/// Returns the convolution of two bit-stores as a new bit-vector.
///
/// The *convolution* of $u$ and $v$ is a vector with the elements $ (u * v)_k = \sum_j u_j v_{k-j+1} $
/// where the sum is taken over all $j$ such that the indices in the formula are valid.
///
/// # Example
/// ```
/// auto lhs = BitVector<>::ones(3);
/// auto rhs = BitVector<>::ones(2);
/// auto result = convolve(lhs, rhs);
/// assert_eq(to_string(result), "1001");
/// ```
template<BitStore Lhs, BitStore Rhs>
    requires std::same_as<typename Lhs::word_type, typename Rhs::word_type>
auto
convolve(Lhs const& lhs, Rhs const& rhs) {
    using word_type = typename Lhs::word_type;
    auto bits_per_word = BITS<word_type>;

    // Edge case: if either store is empty then the convolution is empty.
    if (lhs.is_empty() || rhs.is_empty()) return BitVector<word_type>{};

    // Set up the result bit-vector.
    auto result = BitVector<word_type>::zeros(lhs.size() + rhs.size() - 1);

    // If either vector is all zeros then the convolution is all zeros.
    if (lhs.none() || rhs.none()) return result;

    // Only need to consider words in `rhs` up to and including the one holding its final set bit.
    // We have already checked that `rhs` is not all zeros so we know there is a last set bit!
    auto rhs_words_end = word_index<word_type>(rhs.last_set().value()) + 1;

    // Initialize `result` by copying the live words from `rhs`
    for (auto i = 0uz; i < rhs_words_end; ++i) result.set_word(i, rhs.word(i));

    // Work backwards from the last set bit in `lhs` (which we know exists as we checked `lhs` is not all zeros).
    for (auto i = lhs.last_set().value(); i-- > 0;) {
        word_type prev = 0;
        for (auto j = 0uz; j < result.words(); ++j) {
            auto left = static_cast<word_type>(prev >> (bits_per_word - 1));
            prev = result.word(j);
            result.set_word(j, static_cast<word_type>(prev << 1) | left);
        }

        if (lhs.get(i)) {
            for (auto j = 0uz; j < rhs_words_end; ++j) result.set_word(j, result.word(j) ^ rhs.word(j));
        }
    }
    return result;
}

/// @}

} // namespace gf2

/// Specialise `std::formatter` to handle bit-stores ...
template<gf2::BitStore Store>
struct std::formatter<Store> {
    // Parse a bit-store format specifier where where we recognize {:p} and {:x}
    constexpr auto parse(std::format_parse_context const& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end() && *it != '}') {
            switch (*it) {
                case 'p': m_pretty = true; break;
                case 'x': m_hex = true; break;
                default: m_error = true;
            }
            ++it;
        }
        return it;
    }

    // Format the bit-store according using the appropriate string conversion function.
    template<class FormatContext>
    auto format(Store const& rhs, FormatContext& ctx) const {
        // Was there a format specification error?
        if (m_error) return std::format_to(ctx.out(), "'UNRECOGNIZED FORMAT SPECIFIER FOR BIT-STORE'");

        // Special handling requested?
        if (m_hex) return std::format_to(ctx.out(), "{}", to_hex_string(rhs));
        if (m_pretty) return std::format_to(ctx.out(), "{}", to_pretty_string(rhs));

        // Default
        return std::format_to(ctx.out(), "{}", to_string(rhs));
    }

    bool m_hex = false;
    bool m_pretty = false;
    bool m_error = false;
};
