#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// The base class for the vector-like types in the `gf2` library. <br>
/// See the [BitStore](docs/pages/BitStore.md) page for more details.

#include <gf2/BitRef.h>
#include <gf2/assert.h>
#include <gf2/Iterators.h>
#include <gf2/RNG.h>
#include <gf2/unsigned_word.h>
#include <gf2/store_word.h>

#include <bitset>
#include <concepts>
#include <format>
#include <iostream>
#include <limits>
#include <optional>
#include <ranges>
#include <string>
namespace gf2 {

/// The base class for the vector-like types in the `gf2` library: [`BitSet`](BitSet.md), [`BitVec`](BitVec.md), and
/// [`BitSpan`](BitSpan.md)<br> The template parameter `Store` is a derived class --- we use the CRTP idiom for compile
/// time polymorphism. The `Store` subclass provides an implementation for a small number of required methods and in
/// return, inherit the many methods provided by this base class.
///
/// @note The `BitStore` class has no data members of its own. Users typically will not use this class directly -- it's
/// just an implementation detail to avoid code duplication. Instead, they will create Instead, they will create
/// `gf2::BitSet` and `gf2::BitVec` instances, along with `gf2::BitSpan`'s from those objects.
template<typename Store>
class BitStore {
private:
    // CRTP helper method that returns a reference to the derived type.
    constexpr Store& store() { return static_cast<Store&>(*this); }

    // CRTP helper method that returns a const reference to the derived type.
    constexpr const Store& store() const { return static_cast<const Store&>(*this); }

public:
    /// Unsigned word type used to store the bits (`u8`, `16`, `u32`, `u64`, or `usize`).
    using word_type = typename store_word<Store>::word_type;

    /// The number of bits in each store word.
    static constexpr u8 bits_per_word = BITS<word_type>;

    // A default constructor plus the rule of 5 to stop picky compilers from complaining ...
    BitStore() = default;
    virtual ~BitStore() = default;
    BitStore(const BitStore&) = default;
    BitStore& operator=(const BitStore&) = default;
    BitStore(BitStore&&) = default;
    BitStore& operator=(BitStore&&) = default;

    /// @name Required methods:
    /// The `BitStore` subclasses implement the following methods ...
    /// @{

    /// Returns the number of bit elements in the store.
    constexpr usize size() const { return store().size(); }

    /// Returns the **fewest** number of words needed to store the bits in the store.
    ///
    /// This method is trivially implemented by the `BitVec` class. However, the bits in a `BitSpan` may not be aligned
    /// to word boundaries but that class synthesises words *as if* they were by combining adjacent words as needed.
    constexpr usize words() const { return store().words(); }

    /// Returns "word" `i` underpinning this bit store.
    ///
    /// For example, if the store has 18 elements, then the first word will be returned for bit elements 0 through 7,
    /// the second word will be returned for bit elements 8 through 15, and the third word will be returned for bit
    /// elements 16 and 17.
    ///
    /// This method is trivially implemented by the `BitVec` class. However, the bits in a `BitSpan` may not be aligned
    /// to word boundaries but that class synthesises words *as if* they were by combining adjacent words as needed.
    ///
    /// @note The final word may not be fully occupied but the method must guarantee that unused bits are set to 0.
    constexpr word_type word(usize i) const { return store().word(i); }

    /// This method sets "word" at index `i` to the specified `value`.
    ///
    /// For example, if the store has 18 bit elements, then setting the first word changes bit elements 0 through
    /// 7, the second word changes bit elements 8 through 15, and the third word changes bit elements 16 and 17.
    ///
    /// This method is trivially implemented by the `BitVec` class.
    /// However, the bits in a `BitSpan` may not be aligned to word boundaries but that class synthesises words *as if*
    /// they were by combining adjacent words as needed.
    ///
    /// @note The method must ensure that inaccessible bits in the underlying store are not changed by this call.
    constexpr void set_word(usize i, word_type value) { store().set_word(i, value); }

    /// Returns a const pointer to the real first word in the underlying store.
    constexpr const word_type* data() const { return store().data(); }

    /// Returns a pointer to the real first word in the underlying store.
    ///
    /// @note The pointer is non-const but you should be careful about using it to modify the words in the store.
    constexpr word_type* data() { return store().data(); }

    /// Returns the offset (in bits)  from the bit 0 of `data()[0]` to the first bit in the store.
    ///
    /// @note This is always zero for `BitSet` and `BitVec` but may be non-zero for `BitSpan`.
    constexpr u8 offset() const { return store().offset(); }

    /// @}
    /// @name Individual bit reads:
    /// @{

    /// Returns `true` if the bit at the given index `i` is set, `false` otherwise.
    ///
    /// @note In debug mode the index `i` is bounds-checked.
    ///
    /// # Example
    /// ```
    /// BitVec v{10};
    /// assert_eq(v.get(0), false);
    /// v.set(0);
    /// assert_eq(v.get(0), true);
    /// ```
    constexpr bool get(usize i) const {
        gf2_debug_assert(i < size(), "Index {} is out of bounds for a bit store of length {}", i, size());
        auto [word_index, mask] = index_and_mask<word_type>(i);
        return word(word_index) & mask;
    }

    /// Returns the boolean value of the bit element at `index`.
    ///
    /// @note In debug mode the index is bounds-checked.
    ///
    /// # Example
    /// ```
    /// BitVec v{10};
    /// assert(v[2] == false);
    /// v[2] = true;
    /// assert(v[2] == true);
    /// assert(v.to_string() == "0010000000");
    /// ```
    constexpr bool operator[](usize index) const { return get(index); }

    /// Returns `true` if the first bit element is set, `false` otherwise.
    ///
    /// @note In debug mode the method panics of the store is empty.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<>::ones(10);
    /// assert_eq(v.front(), true);
    /// v.set_all(false);
    /// assert_eq(v.front(), false);
    /// ```
    constexpr bool front() const {
        gf2_debug_assert(!is_empty(), "The store is empty!");
        return get(0);
    }

    /// Returns `true` if the last bit element is set, `false` otherwise.
    ///
    /// @note In debug mode the method panics of the store is empty.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<>::ones(10);
    /// assert_eq(v.back(), true);
    /// v.set_all(false);
    /// assert_eq(v.back(), false);
    /// ```
    constexpr bool back() const {
        gf2_debug_assert(!is_empty(), "The store is empty!");
        return get(size() - 1);
    }

    /// @}
    /// @name Individual bit mutators:
    /// @{

    /// Sets the bit-element at the given `index` to the specified boolean `value` & returns this for chaining.
    /// The default value for `value` is `true`.
    ///
    /// @note In debug mode the index is bounds-checked.
    ///
    /// # Example
    /// ```
    /// BitVec v{10};
    /// assert_eq(v.get(0), false);
    /// v.set(0);
    /// assert_eq(v.get(0), true);
    /// ```
    auto set(usize index, bool value = true) {
        gf2_debug_assert(index < size(), "Index {} is out of bounds for a bit store of length {}", index, size());
        auto [word_index, mask] = index_and_mask<word_type>(index);
        auto word_value = word(word_index);
        auto bit_value = (word_value & mask) != 0;
        if (bit_value != value) set_word(word_index, word_value ^ mask);
        return *this;
    }

    /// Returns a "reference" to the bit element at `index`.
    ///
    /// The returned object is a `BitRef` reference for the bit element at `index` rather than a true reference.
    ///
    /// @note In debug mode the index `i` is bounds-checked.
    ///
    /// # Example
    /// ```
    /// BitVec v{10};
    /// v[2] = true;
    /// assert(v.to_string() == "0010000000");
    /// auto w = BitVec<>::ones(10);
    /// v[3] = w[3];
    /// assert(v.to_string() == "0011000000");
    /// v[4] |= w[4];
    /// assert(v.to_string() == "0011100000");
    /// ```
    constexpr auto operator[](usize index) { return BitRef<Store>{this, index}; }

    /// Flips the value of the bit-element at the given `index` and returns this for chaining/
    ///
    /// @note In debug mode the index is bounds-checked.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(10);
    /// v.flip(0);
    /// assert_eq(v.to_string(), "0111111111");
    /// v.flip(1);
    /// assert_eq(v.to_string(), "0011111111");
    /// v.flip(9);
    /// assert_eq(v.to_string(), "0011111110");
    /// ```
    auto flip(usize index) {
        gf2_debug_assert(index < size(), "Index {} is out of bounds for a bit store of length {}", index, size());
        auto [word_index, mask] = index_and_mask<word_type>(index);
        set_word(word_index, word(word_index) ^ mask);
        return *this;
    }

    /// Swaps the bits in the bit-store at indices `i0` and `i1` and returns this for chaining.
    ///
    /// @note In debug mode, panics if either of the indices is out of bounds.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<>::zeros(10);
    /// v.set(0);
    /// assert_eq(v.to_string(), "1000000000");
    /// v.swap(0, 1);
    /// assert_eq(v.to_string(), "0100000000");
    /// v.swap(0, 1);
    /// assert_eq(v.to_string(), "1000000000");
    /// v.swap(0, 9);
    /// assert_eq(v.to_string(), "0000000001");
    /// v.swap(0, 9);
    /// assert_eq(v.to_string(), "1000000000");
    /// ```
    constexpr auto swap(usize i0, usize i1) {
        gf2_debug_assert(i0 < size(), "index {} is out of bounds for a store of length {}", i0, size());
        gf2_debug_assert(i1 < size(), "index {} is out of bounds for a store of length {}", i1, size());
        if (i0 != i1) {
            auto [w0, mask0] = index_and_mask<word_type>(i0);
            auto [w1, mask1] = index_and_mask<word_type>(i1);
            auto word0 = word(w0);
            auto word1 = word(w1);
            auto val0 = (word0 & mask0) != 0;
            auto val1 = (word1 & mask1) != 0;
            if (val0 != val1) {
                if (w0 == w1) {
                    // Both bits are in the same word
                    set_word(w0, word0 ^ mask0 ^ mask1);
                } else {
                    // BitsIter are in different words
                    set_word(w0, word0 ^ mask0);
                    set_word(w1, word1 ^ mask1);
                }
            }
        }
        return *this;
    }

    /// @}
    /// @name Store queries:
    /// @{

    /// Returns `true` if the store is empty, `false` otherwise.
    ///
    /// # Example
    /// ```
    /// BitVec v;
    /// assert_eq(v.is_empty(), true);
    /// BitVec u{10};
    /// assert_eq(u.is_empty(), false);
    /// ```
    constexpr bool is_empty() const { return size() == 0; }

    /// Returns `true` if at least one bit in the store is set, `false` otherwise.
    ///
    /// @note  Empty stores have no set bits (logical connective for `any` is `OR` with identity `false`).
    ///
    /// # Example
    /// ```
    /// BitVec v{10};
    /// assert_eq(v.any(), false);
    /// v.set(0);
    /// assert_eq(v.any(), true);
    /// ```
    constexpr bool any() const {
        for (auto i = 0uz; i < words(); ++i)
            if (word(i) != 0) return true;
        return false;
    }

    /// Returns `true` if all bits in the store are set, `false` otherwise.
    ///
    /// @note  Empty stores have no set bits (logical connective for `all` is `AND` with identity `true`).
    ///
    /// # Example
    /// ```
    /// BitVec v{3};
    /// assert_eq(v.all(), false);
    /// v.set(0);
    /// v.set(1);
    /// v.set(2);
    /// assert_eq(v.all(), true);
    /// ```
    constexpr bool all() const {
        auto num_words = words();
        if (num_words == 0) { return true; }

        // Check the fully occupied words ...
        for (auto i = 0uz; i < num_words - 1; ++i) {
            if (word(i) != MAX<word_type>) return false;
        }

        // Last word might not be fully occupied ...
        auto unused_bits = bits_per_word - (size() % bits_per_word);
        auto last_word_max = MAX<word_type> >> unused_bits;
        if (word(num_words - 1) != last_word_max) return false;

        return true;
    }

    /// Returns `true` if no bits in the store are set, `false` otherwise.
    ///
    /// @note  Empty store have no set bits (logical connective for `none` is `AND` with identity `true`).
    ///
    /// # Example
    /// ```
    /// BitVec v{10};
    /// assert_eq(v.none(), true);
    /// v.set(0);
    /// assert_eq(v.none(), false);
    /// ```
    constexpr bool none() const { return !any(); }

    /// @}
    /// @name Store mutators:
    /// @{

    /// Sets the bits in the store to the boolean `value` and returns a reference to this for chaining.
    ///
    /// By default, all bits are set to `true`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::zeros(10);
    /// v.set_all();
    /// assert_eq(v.to_string(), "1111111111");
    /// ```
    auto set_all(bool value = true) {
        auto word_value = value ? MAX<word_type> : word_type{0};
        for (auto i = 0uz; i < words(); ++i) set_word(i, word_value);
        return *this;
    }

    /// Flips the value of the bits in the store and returns a reference to this for chaining.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::zeros(10);
    /// v.flip_all();
    /// assert_eq(v.to_string(), "1111111111");
    /// ```
    auto flip_all() {
        for (auto i = 0uz; i < words(); ++i) set_word(i, static_cast<word_type>(~word(i)));
        return *this;
    }

    /// @}
    /// @name Store fills:
    /// @{

    /// Copies the bits from an unsigned integral `src` value and returns a reference to this for chaining.
    ///
    /// # Notes:
    /// 1. The size of the store *must* match the number of bits in the source type.
    /// 2. We allow *any* unsigned integral source, e.g. copying a single `u64` into a `BitVec<u8>` of size 64.
    /// 3. The least-significant bit of the source becomes the bit at index 0 in the store.
    ///
    /// # Example
    /// ```
    /// BitVec<u8> v{16};
    /// u16 src = 0b1010101010101010;
    /// v.copy(src);
    /// assert_eq(v.to_string(), "0101010101010101");
    /// BitVec<u32> w{16};
    /// w.copy(src);
    /// assert_eq(w.to_string(), "0101010101010101");
    /// ```
    template<unsigned_word Src>
    auto copy(Src src) {
        constexpr auto src_bits = BITS<Src>;
        gf2_assert(size() == src_bits, "Lengths do not match: {} != {}.", size(), src_bits);

        if constexpr (src_bits <= bits_per_word) {
            // The source fits into a single one of our words
            set_word(0, static_cast<word_type>(src));
        } else {
            // The source spans some number of our words (which we assume is an integer number).
            constexpr auto num_words = src_bits / bits_per_word;
            for (auto i = 0uz; i < num_words; ++i) {
                auto shift = i * bits_per_word;
                auto word_value = static_cast<word_type>((src >> shift) & MAX<word_type>);
                set_word(i, word_value);
            }
        }
        return *this;
    }

    /// Copies the bits from an equal-sized `src` store and returns a reference to this for chaining.
    ///
    /// # Note:
    /// This is one of the few methods in the library that *doesn't* require the two stores to have the same
    /// `word_type`. You can use it to convert between different `word_type` stores (e.g., from `BitVec<u32>` to
    /// `BitVec<u8>`) as long as the sizes match.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u64>::ones(10);
    /// assert_eq(v.to_string(), "1111111111");
    /// v.copy(BitVec<u8>::alternating(10));
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    template<typename SrcStore>
    auto copy(const BitStore<SrcStore>& src) {
        // The sizes must match ...
        gf2_assert(size() == src.size(), "Lengths do not match: {} != {}.", size(), src.size());

        // What is the source word type (without any const qualifier)?
        using src_word_type = std::remove_const_t<typename store_word<SrcStore>::word_type>;

        // Numbers of bits per respective words
        constexpr auto word_bits = BITS<word_type>;
        constexpr auto src_word_bits = BITS<src_word_type>;

        // What we do next depends on the size of the respective words.
        if constexpr (word_bits == src_word_bits) {
            // Easiest case: Same sized words -- we can copy whole words directly ...
            for (auto i = 0uz; i < words(); ++i) {
                auto src_word = static_cast<word_type>(src.word(i));
                set_word(i, src_word);
            }
        } else if constexpr (word_bits > src_word_bits) {
            // Our word type is larger -- some number of source words fit into each one of ours.
            // We assume that our word size is an integer multiple of the source word size.
            constexpr auto ratio = word_bits / src_word_bits;

            auto src_words = src.words();
            for (auto i = 0uz; i < words(); ++i) {

                // Combine `ratio` source words into a single destination word being careful not to run off the end ...
                word_type dst_word = 0;
                for (auto j = 0uz; j < ratio; ++j) {
                    auto      src_index = i * ratio + j;
                    word_type src_word = 0;
                    if (src_index < src_words) src_word = static_cast<word_type>(src.word(src_index));
                    dst_word |= (src_word << (j * src_word_bits));
                }
                set_word(i, dst_word);
            }
        } else {
            // Our word size is smaller -- each source word becomes multiple destination words.
            // We assume that the source word size is an integer multiple of our word size.
            constexpr auto ratio = src_word_bits / word_bits;

            auto dst_words = words();
            for (auto src_index = 0uz; src_index < src.words(); ++src_index) {
                auto src_word = src.word(src_index);

                // Split the source word into `ratio` destination words ...
                for (auto j = 0uz; j < ratio; ++j) {
                    auto dst_index = src_index * ratio + j;
                    if (dst_index >= dst_words) break;
                    auto shift = j * word_bits;
                    auto dst_word = static_cast<word_type>((src_word >> shift) & MAX<word_type>);
                    set_word(dst_index, dst_word);
                }
            }
        }
        return *this;
    }

    /// Copies the bits of an equal-sized `std::bitset` and returns a reference to this for chaining.
    ///
    /// @note `std::bitset` prints its bit elements in *bit-order* which is the reverse of our convention.
    ///
    /// # Example
    /// ```
    /// std::bitset<10> src{0b1010101010};
    /// BitVec v{10};
    /// v.copy(src);
    /// assert_eq(v.to_string(), "0101010101");
    /// ```
    template<usize N>
    auto copy(const std::bitset<N>& src) {
        // The sizes must match
        gf2_assert(size() == N, "Lengths do not match: {} != {}.", size(), N);

        // Doing this the dumb way as there is no standard access to the bitset's underlying store
        set_all(false);
        for (auto i = 0uz; i < N; ++i)
            if (src[i]) set(i);
        return *this;
    }

    /// Fill the store by repeatedly calling `f(i)` and returns a reference to this for chaining.
    ///
    /// # Example
    /// ```
    /// BitVec v{10};
    /// v.fill([](usize i) { return i % 2 == 0; });
    /// assert_eq(v.size(), 10);
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    auto fill(std::invocable<usize> auto f) {
        set_all(false);
        for (auto i = 0uz; i < size(); ++i)
            if (f(i) == true) set(i);
        return *this;
    }

    /// Fill the store with random bits and returns a reference to this for chaining.
    ///
    /// The default call `random_fill()` sets each bit to 1 with probability 0.5 (fair coin).
    ///
    /// @param p The probability of the elements being 1 (defaults to a fair coin, i.e. 50-50).
    /// @param seed The seed to use for the random number generator (defaults to 0, which means use entropy).
    ///
    /// @note If `p < 0` then the fill is all zeros, if `p > 1` then the fill is all ones.
    ///
    /// # Example
    /// ```
    /// BitVec u{10}, v{10};
    /// u64 seed = 1234567890;
    /// u.random_fill(0.5, seed);
    /// v.random_fill(0.5, seed);
    /// assert(u == v);
    /// ```
    auto random_fill(double p = 0.5, u64 seed = 0) {
        // Keep a single static RNG per thread for all calls to this method, seeded with entropy on the first call.
        thread_local RNG rng;

        // Edge case handling ...
        if (p < 0) return set_all(false);

        // Scale p by 2^64 to remove floating point arithmetic from the main loop below.
        // If we determine p rounds to 1 then we can just set all elements to 1 and return early.
        p = p * 0x1p64 + 0.5;
        if (p >= 0x1p64) return set_all();

        // p does not round to 1 so we use a 64-bit URNG and check each draw against the 64-bit scaled p.
        auto scaled_p = static_cast<u64>(p);

        // If a seed was provided, set the RNG's seed to it. Otherwise, we carry on from where we left off.
        u64 old_seed = rng.seed();
        if (seed != 0) rng.set_seed(seed);

        set_all(false);
        for (auto i = 0uz; i < size(); ++i)
            if (rng.u64() < scaled_p) set(i);

        // Restore the old seed if necessary.
        if (seed != 0) rng.set_seed(old_seed);
        return *this;
    }

    /// @}
    /// @name Bit counts:
    /// @{

    /// Returns the number of set bits in the store.
    ///
    /// # Example
    /// ```
    /// BitVec v{10};
    /// assert_eq(v.count_ones(), 0);
    /// v.set(0);
    /// assert_eq(v.count_ones(), 1);
    /// ```
    constexpr usize count_ones() const {
        usize count = 0;
        for (auto i = 0uz; i < words(); ++i) count += static_cast<usize>(gf2::count_ones(word(i)));
        return count;
    }

    /// Returns the number of unset bits in the store.
    ///
    /// # Example
    /// ```
    /// BitVec v{10};
    /// assert_eq(v.count_zeros(), 10);
    /// v.set(0);
    /// assert_eq(v.count_zeros(), 9);
    /// ```
    constexpr usize count_zeros() const { return size() - count_ones(); }

    /// Returns the number of leading zeros in the store.
    ///
    /// # Example
    /// ```
    /// BitVec v{37};
    /// assert_eq(v.leading_zeros(), 37);
    /// v.set(27);
    /// assert_eq(v.leading_zeros(), 27);
    /// auto w = BitVec<u8>::ones(10);
    /// assert_eq(w.leading_zeros(), 0);
    /// ```
    constexpr usize leading_zeros() const {
        // Note: Even if the last word is not fully occupied, we know unused bits are set to 0.
        for (auto i = 0uz; i < words(); ++i) {
            auto w = word(i);
            if (w != 0) return i * bits_per_word + static_cast<usize>(gf2::trailing_zeros(w));
        }
        return size();
    }

    /// Returns the number of trailing zeros in the store.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::zeros(27);
    /// assert_eq(v.trailing_zeros(), 27);
    /// v.set(0);
    /// assert_eq(v.trailing_zeros(), 26);
    /// ```
    constexpr usize trailing_zeros() const {
        if (is_empty()) return 0;

        // The last word may not be fully occupied so we need to subtract the number of unused bits.
        auto unused_bits = bits_per_word - (size() % bits_per_word);
        auto num_words = words();
        auto i = num_words;
        while (i--) {
            auto w = word(i);
            if (w != 0) {
                auto whole_count = (num_words - i - 1) * bits_per_word;
                auto partial_count = static_cast<usize>(gf2::leading_zeros(w)) - unused_bits;
                return whole_count + partial_count;
            }
        }
        return size();
    }

    /// @}
    /// @name Set bit indices:
    /// @{

    /// Returns the index of the first set bit in the bit-store or `{}` if no bits are set.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::zeros(37);
    /// assert(v.first_set() == std::optional<usize>{});
    /// v.set(2);
    /// assert(v.first_set() == std::optional<usize>{2});
    /// v.set(2, false);
    /// assert(v.first_set() == std::optional<usize>{});
    /// v.set(27);
    /// assert(v.first_set() == std::optional<usize>{27});
    /// BitVec empty;
    /// assert(empty.first_set() == std::optional<usize>{});
    /// ```
    std::optional<usize> first_set() const {
        // Iterate forward looking for a word with a set bit and use the lowest of those ...
        // Remember that any unused bits in the final word are guaranteed to be unset.
        for (auto i = 0uz; i < words(); ++i) {
            if (auto loc = lowest_set_bit(word(i))) return i * bits_per_word + loc.value();
        }
        return {};
    }

    /// Returns the index of the last set bit in the bit-store or `{}` if no bits are set.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::zeros(37);
    /// assert(v.last_set() == std::optional<usize>{});
    /// v.set(2);
    /// assert(v.last_set() == std::optional<usize>{2});
    /// v.set(27);
    /// assert(v.last_set() == std::optional<usize>{27});
    /// BitVec empty;
    /// assert(empty.last_set() == std::optional<usize>{});
    /// ```
    std::optional<usize> last_set() const {
        // Iterate backward looking for a word with a set bit and use the highest of those ...
        // Remember that any unused bits in the final word are guaranteed to be unset.
        for (auto i = words(); i--;) {
            if (auto loc = highest_set_bit(word(i))) return i * bits_per_word + loc.value();
        }
        return {};
    }

    /// Returns the index of the next set bit after `index` in the store or `{}` if no more set bits exist.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::zeros(37);
    /// assert(v.next_set(0) == std::optional<usize>{});
    /// v.set(2);
    /// v.set(27);
    /// assert(v.next_set(0) == std::optional<usize>{2});
    /// assert(v.next_set(2) == std::optional<usize>{27});
    /// assert(v.next_set(27) == std::optional<usize>{});
    /// ```
    std::optional<usize> next_set(usize index) const {
        // Start our search at index + 1.
        index = index + 1;

        // Perhaps we are off the end? (This also handles the case of an empty type).
        if (index >= size()) return {};

        // Where is that starting index located in the word store?
        auto [word_index, bit] = index_and_offset<word_type>(index);

        // Iterate forward looking for a word with a new set bit and use the lowest one ...
        for (auto i = word_index; i < words(); ++i) {
            auto w = word(i);
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
    /// auto v = BitVec<u8>::zeros(37);
    /// assert(v.previous_set(36) == std::optional<usize>{});
    /// v.set(2);
    /// v.set(27);
    /// assert(v.previous_set(36) == std::optional<usize>{27});
    /// assert(v.previous_set(27) == std::optional<usize>{2});
    /// assert(v.previous_set(2)  == std::optional<usize>{});
    /// ```
    std::optional<usize> previous_set(usize index) const {
        // Edge case: If the store is empty or we are at the start, there are no previous set bits.
        if (is_empty() || index == 0) return {};

        // Silently fix large indices and also adjust the index down a slot.
        if (index > size()) index = size();
        index--;

        // Where is that starting index located in the word store?
        auto [word_index, bit] = index_and_offset<word_type>(index);

        // Iterate backwards looking for a word with a new set bit and use the highest one ...
        for (auto i = word_index + 1; i--;) {
            auto w = word(i);
            if (i == word_index) {
                // First word -- turn all higher bits after our starting bit into zeros so we don't see them as set.
                reset_bits(w, bit + 1, bits_per_word);
            }
            if (auto loc = highest_set_bit(w)) return i * bits_per_word + loc.value();
        }
        return {};
    }

    /// @}
    /// @name Unset bit indices:
    /// @{

    /// Returns the index of the first unset bit in the bit-store or `{}` if no bits are unset.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(37);
    /// assert(v.first_unset() == std::optional<usize>{});
    /// v.set(2, false);
    /// assert(v.first_unset() == std::optional<usize>{2});
    /// v.set(2);
    /// assert(v.first_unset() == std::optional<usize>{});
    /// v.set(27, false);
    /// assert(v.first_unset() == std::optional<usize>{27});
    /// BitVec empty;
    /// assert(empty.first_unset() == std::optional<usize>{});
    /// ```
    std::optional<usize> first_unset() const {
        // Iterate forward looking for a word with a unset bit and use the lowest of those ...
        for (auto i = 0uz; i < words(); ++i) {
            auto w = word(i);
            if (i == words() - 1) {
                // Final word may have some unused zero bits that we need to replace with ones.
                auto last_occupied_bit = bit_offset<word_type>(size() - 1);
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
    /// auto v = BitVec<u8>::ones(37);
    /// assert(v.last_unset() == std::optional<usize>{});
    /// v.set(2, false);
    /// assert(v.last_unset() == std::optional<usize>{2});
    /// v.set(2);
    /// assert(v.last_unset() == std::optional<usize>{});
    /// v.set(27, false);
    /// assert(v.last_unset() == std::optional<usize>{27});
    /// BitVec empty;
    /// assert(empty.last_unset() == std::optional<usize>{});
    /// ```
    std::optional<usize> last_unset() const {
        // Iterate backwards looking for a word with a unset bit and use the highest of those ...
        for (auto i = words(); i--;) {
            auto w = word(i);
            if (i == words() - 1) {
                // Final word may have some unused zero bits that we need to replace with ones.
                auto last_occupied_bit = bit_offset<word_type>(size() - 1);
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
    /// auto v = BitVec<u8>::ones(37);
    /// assert(v.next_unset(0) == std::optional<usize>{});
    /// v.set(2, false);
    /// v.set(27, false);
    /// assert(v.next_unset(0) == std::optional<usize>{2});
    /// assert(v.next_unset(2) == std::optional<usize>{27});
    /// assert(v.next_unset(27) == std::optional<usize>{});
    /// BitVec empty;
    /// assert(empty.next_unset(0) == std::optional<usize>{});
    /// ```
    std::optional<usize> next_unset(usize index) const {
        // Start our search at index + 1.
        index = index + 1;

        // Perhaps we are off the end? (This also handles the case of an empty type).
        if (index >= size()) return {};

        // Where is that starting index located in the word store?
        auto [word_index, bit] = index_and_offset<word_type>(index);

        // Iterate forward looking for a word with a new unset bit and use the lowest of those ...
        for (auto i = word_index; i < words(); ++i) {
            auto w = word(i);
            if (i == word_index) {
                // First word -- turn all the bits before our starting bit into ones so we don't see them as unset.
                set_bits(w, 0, bit);
            }
            if (i == words() - 1) {
                // Final word may have some unused zero bits that we need to replace with ones.
                auto last_occupied_bit = bit_offset<word_type>(size() - 1);
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
    /// auto v = BitVec<u8>::ones(37);
    /// assert(v.previous_unset(0) == std::optional<usize>{});
    /// v.set(2, false);
    /// v.set(27, false);
    /// assert(v.previous_unset(36) == std::optional<usize>{27});
    /// assert(v.previous_unset(27) == std::optional<usize>{2});
    /// assert(v.previous_unset(2) == std::optional<usize>{});
    /// BitVec empty;
    /// assert(empty.previous_unset(0) == std::optional<usize>{});
    /// ```
    std::optional<usize> previous_unset(usize index) const {
        // Edge case: If the store is empty or we are at the start, there are no previous set bits.
        if (is_empty() || index == 0) return {};

        // Silently fix large indices and also adjust the index down a slot.
        if (index > size()) index = size();
        index--;

        // Where is that starting index located in the word store?
        auto [word_index, bit] = index_and_offset<word_type>(index);

        // Iterate backwards looking for a word with a new unset bit and use the highest of those ...
        for (auto i = word_index + 1; i--;) {
            auto w = word(i);
            if (i == word_index) {
                // First word -- set all the bits after our starting bit into ones so we don't see them as unset.
                set_bits(w, bit + 1, bits_per_word);
            }
            if (i == words() - 1) {
                // Final word may have some unused zero bits that we need to replace with ones.
                auto last_occupied_bit = bit_offset<word_type>(size() - 1);
                set_bits(w, last_occupied_bit + 1, bits_per_word);
            }
            if (auto loc = highest_unset_bit(w)) return i * bits_per_word + loc.value();
        }
        return {};
    }

    /// @}
    /// @name Iterators:
    /// @{

    /// Returns a const iterator over the `bool` values of the bits in the const bit-store.
    ///
    /// You can use this iterator to iterate over the bits in the store and get the values of each bit as a `bool`.
    ///
    /// @note For the most part, try to avoid iterating through individual bits. It is much more efficient to use
    /// methods that work on whole words of bits at a time.
    ///
    /// # Example
    /// ```
    /// auto u = BitVec<u8>::ones(10);
    /// for (auto&& bit : u.bits()) assert_eq(bit, true);
    /// ```
    constexpr auto bits() const { return BitsIter<Store, true>{this, 0}; }

    /// Returns a non-const iterator over the values of the bits in the mutable bit-store.
    ///
    /// You can use this iterator to iterate over the bits in the store to get *or* set the value of each bit.
    ///
    /// @note For the most part, try to avoid iterating through individual bits. It is much more efficient to use
    /// methods that work on whole words of bits at a time.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::zeros(10);
    /// for (auto&& bit : v.bits()) bit = true;
    /// assert_eq(v.to_string(), "1111111111");
    /// ```
    constexpr auto bits() { return BitsIter<Store, false>{this, 0}; }

    /// Returns an iterator over the *indices* of any *set* bits in the bit-store.
    ///
    /// You can use this iterator to iterate over the set bits in the store and get the index of each bit.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::alternating(10);
    /// assert_eq(v.to_string(), "1010101010");
    /// auto indices = std::ranges::to<std::vector>(v.set_bit_indices());
    /// assert_eq(indices, (std::vector<usize>{0, 2, 4, 6, 8}));
    /// ```
    constexpr auto set_bit_indices() const { return SetBitsIter{this}; }

    /// Returns an iterator over the *indices* of any *unset* bits in the bit-store.
    ///
    /// You can use this iterator to iterate over the unset bits in the store and get the index of each bit.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::alternating(10);
    /// assert_eq(v.to_string(), "1010101010");
    /// auto indices = std::ranges::to<std::vector>(v.unset_bit_indices());
    /// assert_eq(indices, (std::vector<usize>{1, 3, 5, 7, 9}));
    /// ```
    constexpr auto unset_bit_indices() const { return UnsetBitsIter{this}; }

    /// Returns a const iterator over all the *words* underlying the bit-store.
    ///
    /// You can use this iterator to iterate over the words in the store and read the `Word` value of each word.
    /// Note that you cannot use this iterator to modify the words in the store.
    ///
    /// @note The words here may be a synthetic construct. The expectation is that the bit `0` in the store is
    /// located at the bit-location `0` of `word(0)`. That is always the case for bit-vectors but bit-slices typically
    /// synthesise "words" on the fly from adjacent pairs of bit-vector words. Nevertheless, almost all the methods
    /// in `BitStore` are implemented efficiently by operating on those words.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(10);
    /// assert_eq(v.to_string(), "1111111111");
    /// auto indices = std::ranges::to<std::vector>(v.store_words());
    /// assert_eq(indices, (std::vector<u8>{0b1111'1111, 0b0000'0011}));
    /// ```
    constexpr auto store_words() const { return WordsIter{this}; }

    /// Returns a copy of the words underlying this bit-store.
    ///
    /// @note The last word in the vector may not be fully occupied but unused slots will be all zeros.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(10);
    /// auto words = v.to_words();
    /// assert_eq(words, (std::vector<u8>{0b1111'1111, 0b0000'0011}));
    /// ```
    constexpr auto to_words() const { return std::ranges::to<std::vector>(store_words()); }

    /// @}
    /// @name Spans:
    /// @{

    /// Returns an *immutable* bit-span encompassing the bits in the half-open range `[begin, end)`.
    ///
    /// Immutability here is deep -- the interior pointer in the returned span is to *const* words.
    ///
    /// @note This method panics if the bit-store is empty or if the range is not valid.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<>::alternating(10);
    /// auto s = v.span(1,5);
    /// assert_eq(s.to_string(), "0101");
    /// ```
    constexpr auto span(usize begin, usize end) const {
        // Check that the range is correctly formed and non-empty.
        gf2_assert(begin < end, "bit-span [{}, {}) is invalid.", begin, end);

        // Check that the range does not extend beyond the end of the bit-store.
        gf2_assert(end <= size(), "bit-span end {} extends beyond the store end {}.", end, size());

        // Get the zero-based word index and bit offset in that word for the begin location.
        auto [index, bit_offset] = index_and_offset<word_type>(begin);

        // If the store is already a span over another store, we need to adjust those values.
        bit_offset += offset();
        if (bit_offset >= bits_per_word) {
            index += 1;
            bit_offset = bit_offset % bits_per_word;
        }

        // Return the bit-span over the underlying word data.
        return BitSpan<const word_type>(data() + index, bit_offset, end - begin);
    }

    /// Returns a mutable bit-span encompassing the bits in the half-open range `[begin, end)`.
    ///
    /// Mutability here is deep -- the interior pointer in the returned span is to *non-const* words.
    ///
    /// @note This method panics if the bit-vector is empty or if the range is not valid.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<>::alternating(10);
    /// auto s = v.span(1,5);
    /// assert_eq(s.to_string(), "0101");
    /// s.set_all();
    /// assert_eq(s.to_string(), "1111");
    /// assert_eq(v.to_string(), "1111101010");
    /// ```
    constexpr auto span(usize begin, usize end) {
        // Check that the range is correctly formed and non-empty.
        gf2_assert(begin < end, "bit-span [{}, {}) is invalid.", begin, end);

        // Check that the range does not extend beyond the end of the bit-store.
        gf2_assert(end <= size(), "bit-span end {} extends beyond the store end {}.", end, size());

        // Get the zero-based word index and bit offset in that word for the begin location.
        auto [index, bit_offset] = index_and_offset<word_type>(begin);

        // If the store is already a span over another store, we need to adjust those values.
        bit_offset += offset();
        if (bit_offset >= bits_per_word) {
            index += 1;
            bit_offset = bit_offset % bits_per_word;
        }

        // Return the bit-span over the underlying word data.
        return BitSpan<word_type>(data() + index, bit_offset, end - begin);
    }

    /// @}
    /// @name Sub-vectors:
    /// @{

    /// Returns a *clone* of the elements in the half-open range `[begin, end)` as a new bit-vector.
    ///
    /// @note This method panics if the bit-vector is empty or if the range is not valid.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<>::alternating(10);
    /// auto s = v.sub(1,5);
    /// assert_eq(s.to_string(), "0101");
    /// s.set_all();
    /// assert_eq(s.to_string(), "1111");
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    constexpr auto sub(usize begin, usize end) const { return BitVec<word_type>::from(span(begin, end)); }

    /// Views a bit-store as two parts containing the elements `[0, at)` and `[at, size())` respectively.
    ///
    /// Clones of the parts are stored in the passed bit-vectors `left` and `right`.
    ///
    /// On return, `left` contains the bits from the start of the bit-vector up to but not including `at` and `right`
    /// contains the bits from `at` to the end of the bit-vector. This bit-vector itself is not modified.
    ///
    /// This lets one reuse the `left` and `right` destinations without having to allocate new bit-vectors.
    /// This is useful when implementing iterative algorithms that need to split a bit-vector into two parts repeatedly.
    ///
    /// @note This method panics if the split point is beyond the end of the bit-vector.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<>::alternating(10);
    /// BitVec left, right;
    /// v.split_at(5, left, right);
    /// assert_eq(left.to_string(), "10101");
    /// assert_eq(right.to_string(), "01010");
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    constexpr void split_at(usize at, BitVec<word_type>& left, BitVec<word_type>& right) const {
        gf2_assert(at <= size(), "split point {} is beyond the end of the bit-vector", at);
        left.clear();
        right.clear();
        left.append(span(0, at));
        right.append(span(at, size()));
    }

    /// Views a bit-store as two parts containing the elements `[0, at)` and `[at, size())` respectively.
    ///
    /// Clones of the parts are returned as a pair of bit-vectors [`left`, `right`].
    ///
    /// On return, `left` is a clone of the bits from the start of the bit-vector up to but not including `at` and
    /// `right` contains the bits from `at` to the end of the bit-vector. This bit-vector itself is not modified.
    ///
    /// @note This method panics if the split point is beyond the end of the bit-vector.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<>::alternating(10);
    /// auto [left, right] = v.split_at(5);
    /// assert_eq(left.to_string(), "10101");
    /// assert_eq(right.to_string(), "01010");
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    constexpr auto split_at(usize at) const {
        BitVec<word_type> left, right;
        split_at(at, left, right);
        return std::pair{left, right};
    }

    /// @}
    /// @name Riffling:
    /// @{

    /// Interleaves the bits of this bit-store with zeros storing the result into the bit-vector `dst`.
    ///
    /// On return, `dst` will have the bits of this bit-store interleaved with zeros. For example, if this
    /// bit-store has the bits `abcde` then `dst` will have the bits `a0b0c0d0e`.
    ///
    /// @note There is no last zero bit in `dst`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(10);
    /// BitVec<u8> dst;
    /// v.riffle_into(dst);
    /// assert_eq(dst.to_string(), "1010101010101010101");
    /// ```
    constexpr void riffle_into(BitVec<word_type>& dst) const {
        // Nothing to do if the bit-vector is empty or has just one element.
        // With two bits `ab` we fill `dst` with `a0b`.
        if (size() <= 1) {
            dst.copy(*this);
            return;
        }

        // Make sure `dst` is large enough to hold the riffled bits (a bit too big but we fix that below).
        dst.resize(2 * size());
        auto dst_words = dst.words();

        // Riffle each word in `this` into two adjacent words in `dst`.
        for (auto i = 0uz; i < words(); ++i) {
            auto [lo, hi] = riffle(word(i));
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
    /// @note There is no last zero bit in `dst`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(10);
    /// auto dst = v.riffled();
    /// assert_eq(dst.to_string(), "1010101010101010101");
    /// ```
    constexpr auto riffled() const {
        BitVec<word_type> result;
        riffle_into(result);
        return result;
    }

    /// @}
    /// @name Bit shifts:
    /// @{

    /// In-place left shift of the bit-store by `shift` bits.
    ///
    /// Shifting is in *vector-order* so if `v = [v0,v1,v2,v3]` then `v <<= 1` is `[v1,v2,v3,0]` with zeros added to
    /// the right. Left shifting in vector-order is the same as right shifting in bit-order.
    ///
    /// @note Only accessible bits are affected by the shift.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(20);
    /// v <<= 8;
    /// assert_eq(v.to_string(), "11111111111100000000");
    /// ```
    constexpr void operator<<=(usize shift) {
        // Edge cases where there is nothing to do.
        if (shift == 0 || size() == 0) return;

        // Perhaps we are shifting all the bits out and are left with all zeros.
        if (shift >= size()) {
            set_all(false);
            return;
        }

        // For larger shifts, we can efficiently shift by whole words first.
        usize word_shift = shift / bits_per_word;
        usize end_word = words() - word_shift;

        // Do the whole word shifts first, pushing in zero words to fill the empty slots.
        if (word_shift > 0) {
            // Shift whole words -- starting at the beginning of the store.
            for (auto i = 0uz; i < end_word; ++i) set_word(i, word(i + word_shift));

            // Fill in the high order words with zeros.
            for (auto i = end_word; i < words(); ++i) set_word(i, 0);

            // How many bits are left to shift?
            shift -= word_shift * bits_per_word;
        }

        // Perhaps there are some partial word shifts left to do.
        if (shift != 0) {
            // Do the "interior" words where the shift moves bits from one word to the next.
            if (end_word > 0) {
                auto shift_complement = bits_per_word - shift;
                for (auto i = 0uz; i < end_word - 1; ++i) {
                    auto lo = static_cast<word_type>(word(i) >> shift);
                    auto hi = static_cast<word_type>(word(i + 1) << shift_complement);
                    set_word(i, lo | hi);
                }
            }

            // Do the last word.
            auto value = word(end_word - 1);
            set_word(end_word - 1, static_cast<word_type>(value >> shift));
        }
    }

    /// In-place right shift of the bit-store by `shift` bits.
    ///
    /// Shifting is in *vector-order* so if `v = [v0,v1,v2,v3]` then `v >>= 1` is `[0,v0,v1,v2]` with zeros added to
    /// the left. Right shifting in vector-order is the same as left shifting in bit-order.
    ///
    /// @note Only accessible bits are affected by the shift.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(20);
    /// v >>= 8;
    /// assert_eq(v.to_string(), "00000000111111111111");
    /// ```
    constexpr void operator>>=(usize shift) {
        // Edge cases where there is nothing to do.
        if (shift == 0 || size() == 0) return;

        // Perhaps we are shifting all the bits out and are left with all zeros.
        if (shift >= size()) {
            set_all(false);
            return;
        }

        // For larger shifts, we can efficiently shift by whole words first.
        usize word_shift = shift / bits_per_word;

        // Do the whole word shifts first, pushing in zero words to fill the empty slots.
        if (word_shift > 0) {
            // Shift whole words -- starting at the end of the store.
            for (auto i = words(); i-- > word_shift;) set_word(i, word(i - word_shift));

            // Fill in the low order words with zeros.
            for (auto i = 0uz; i < word_shift; ++i) set_word(i, 0);

            // How many bits are left to shift?
            shift -= word_shift * bits_per_word;
        }

        // Perhaps there are some partial word shifts left to do.
        if (shift != 0) {
            // Do the "interior" words where the shift moves bits from one word to the next.
            auto shift_complement = bits_per_word - shift;
            for (auto i = words(); i-- > word_shift + 1;) {
                auto lo = static_cast<word_type>(word(i - 1) >> shift_complement);
                auto hi = static_cast<word_type>(word(i) << shift);
                set_word(i, lo | hi);
            }
        }

        // Do the "first" word.
        auto value = word(word_shift);
        set_word(word_shift, static_cast<word_type>(value << shift));
    }

    /// Returns a new bit-vector that is `lhs` shifted left by `shift` bits.
    ///
    /// Shifting is in *vector-order* so if `v = [v0,v1,v2,v3]` then `v << 1` is `[v1,v2,v3,0]` with zeros added to
    /// the right. Left shifting in vector-order is the same as right shifting in bit-order.
    ///
    /// @note Only accessible bits are affected by the shift.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(20);
    /// auto w = v << 8;
    /// assert_eq(w.to_string(), "11111111111100000000");
    /// ```
    constexpr auto operator<<(usize shift) const {
        auto result = BitVec<word_type>::from(store());
        result <<= shift;
        return result;
    }

    /// Returns a new bit-vector that is `lhs` shifted right by `shift` bits.
    ///
    /// Shifting is in *vector-order* so if `v = [v0,v1,v2,v3]` then `v >>= 1` is `[0,v0,v1,v2]` with zeros added to
    /// the left. Right shifting in vector-order is the same as left shifting in bit-order.
    ///
    /// @note Only accessible bits are affected by the shift.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(20);
    /// auto w = v >> 8;
    /// assert_eq(w.to_string(), "00000000111111111111");
    /// ```
    constexpr auto operator>>(usize shift) const {
        auto result = BitVec<word_type>::from(store());
        result >>= shift;
        return result;
    }

    /// @}
    /// @name Bit-wise operations:
    /// @{

    /// In-place `XOR` with another equal-sized bit-store.
    ///
    /// @note In debug mode, this method panics if the lengths of the two bit-stores do not match.
    ///
    /// # Example
    /// ```
    /// auto v1 = BitVec<u8>::alternating(10);
    /// v1 ^= ~v1;
    /// assert_eq(v1.to_string(), "1111111111");
    /// ```
    template<typename Rhs>
        requires store_words_match<Store, Rhs>
    void operator^=(const BitStore<Rhs>& rhs) {
        gf2_assert(size() == rhs.size(), "Lengths do not match: {} != {}.", size(), rhs.size());
        for (auto i = 0uz; i < words(); ++i) set_word(i, word(i) ^ rhs.word(i));
    }

    /// In-place `AND` with another equal-sized bit-store.
    ///
    /// @note In debug mode, this method panics if the lengths of the two bit-stores do not match.
    ///
    /// # Example
    /// ```
    /// auto v1 = BitVec<u8>::alternating(10);
    /// v1 &= ~v1;
    /// assert_eq(v1.to_string(), "0000000000");
    /// ```
    template<typename Rhs>
        requires store_words_match<Store, Rhs>
    void operator&=(const BitStore<Rhs>& rhs) {
        gf2_assert(size() == rhs.size(), "Lengths do not match: {} != {}.", size(), rhs.size());
        for (auto i = 0uz; i < words(); ++i) set_word(i, word(i) & rhs.word(i));
    }

    /// In-place `OR` with another equal-sized bit-store.
    ///
    /// @note In debug mode, this method panics if the lengths of the two bit-stores do not match.
    ///
    /// # Example
    /// ```
    /// auto v1 = BitVec<u8>::alternating(10);
    /// v1 |= ~v1;
    /// assert_eq(v1.to_string(), "1111111111");
    /// ```
    template<typename Rhs>
        requires store_words_match<Store, Rhs>
    void operator|=(const BitStore<Rhs>& rhs) {
        gf2_assert(size() == rhs.size(), "Lengths do not match: {} != {}.", size(), rhs.size());
        for (auto i = 0uz; i < words(); ++i) set_word(i, word(i) | rhs.word(i));
    }

    /// Returns a new bit-vector that has the same bits as the current bit-store but with all the bits flipped.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::alternating(10);
    /// assert_eq(v.to_string(), "1010101010");
    /// auto w = ~v;
    /// assert_eq(w.to_string(), "0101010101");
    /// ```
    auto operator~() const {
        auto result = BitVec<word_type>::from(store());
        result.flip_all();
        return result;
    }

    /// Returns a new bit-vector that is the `XOR` of the current bit-store and another equal-sized bit-store.
    ///
    /// @note In debug mode, this method panics if the lengths of the two bit-stores do not match.
    ///
    /// # Example
    /// ```
    /// auto v1 = BitVec<u8>::alternating(10);
    /// auto v2 = ~v1;
    /// auto v3 = v1 ^ v2;
    /// assert_eq(v3.to_string(), "1111111111");
    /// ```
    template<typename Rhs>
        requires store_words_match<Store, Rhs>
    auto operator^(const BitStore<Rhs>& rhs) const {
        auto result = BitVec<word_type>::from(store());
        result ^= rhs;
        return result;
    }

    /// Returns a new bit-vector that is the `AND` of the current bit-store and another equal-sized bit-store.
    ///
    /// @note In debug mode, this method panics if the lengths of the two bit-stores do not match.
    ///
    /// # Example
    /// ```
    /// auto v1 = BitVec<u8>::alternating(10);
    /// auto v2 = ~v1;
    /// auto v3 = v1 & v2;
    /// assert_eq(v3.to_string(), "0000000000");
    /// ```
    template<typename Rhs>
        requires store_words_match<Store, Rhs>
    auto operator&(const BitStore<Rhs>& rhs) const {
        auto result = BitVec<word_type>::from(store());
        result &= rhs;
        return result;
    }

    /// Returns a new bit-vector that is the `OR` of the current bit-store and another equal-sized bit-store.
    ///
    /// @note In debug mode, this methods panics if the lengths of `lhs` and `rhs` do not match.
    ///
    /// # Example
    /// ```
    /// auto v1 = BitVec<u8>::alternating(10);
    /// auto v2 = ~v1;
    /// auto v3 = v1 | v2;
    /// assert_eq(v3.to_string(), "1111111111");
    /// ```
    template<typename Rhs>
        requires store_words_match<Store, Rhs>
    auto operator|(const BitStore<Rhs>& rhs) const {
        auto result = BitVec<word_type>::from(store());
        result |= rhs;
        return result;
    }

    /// @}
    /// @name Arithmetic operations:
    /// @{

    /// In-place addition of this bit-store with the an equal-sized `rhs` bit-store.
    ///
    /// In GF(2) addition is the same as `XOR`.
    ///
    /// @note In debug mode, this methods panics if the lengths of `lhs` and `rhs` do not match.
    ///
    /// # Example
    /// ```
    /// auto v1 = BitVec<u8>::alternating(10);
    /// v1 += ~v1;
    /// assert_eq(v1.to_string(), "1111111111");
    /// ```
    template<typename Rhs>
        requires store_words_match<Store, Rhs>
    void operator+=(const BitStore<Rhs>& rhs) {
        operator^=(rhs);
    }

    /// In-place subtraction of this bit-store with the an equal-sized `rhs` bit-store.
    ///
    /// In GF(2) subtraction is the same as `XOR`.
    ///
    /// @note In debug mode, this methods panics if the lengths of `lhs` and `rhs` do not match.
    ///
    /// # Example
    /// ```
    /// auto v1 = BitVec<u8>::alternating(10);
    /// v1 -= ~v1;
    /// assert_eq(v1.to_string(), "1111111111");
    /// ```
    template<typename Rhs>
        requires store_words_match<Store, Rhs>
    void operator-=(const BitStore<Rhs>& rhs) {
        operator^=(rhs);
    }

    /// Returns a new bit-vector that is the `+` of this store with the passed (equal-sized) `rhs` bit-store.
    ///
    /// In GF(2) subtraction is the same as `XOR`.
    ///
    /// @note In debug mode, this methods panics if the lengths of `lhs` and `rhs` do not match.
    ///
    /// # Example
    /// ```
    /// auto v1 = BitVec<u8>::alternating(10);
    /// auto v2 = ~v1;
    /// auto v3 = v1 + v2;
    /// assert_eq(v3.to_string(), "1111111111");
    /// ```
    template<typename Rhs>
        requires store_words_match<Store, Rhs>
    auto operator+(const BitStore<Rhs>& rhs) const {
        auto result = BitVec<word_type>::from(store());
        result += rhs;
        return result;
    }

    /// Returns a new bit-vector that is the `-` of this store with the passed (equal-sized) `rhs` bit-store.
    ///
    /// In GF(2) subtraction is the same as `XOR`.
    ///
    /// @note In debug mode, this methods panics if the lengths of `lhs` and `rhs` do not match.
    ///
    /// # Example
    /// ```
    /// auto v1 = BitVec<u8>::alternating(10);
    /// auto v2 = ~v1;
    /// auto v3 = v1 - v2;
    /// assert_eq(v3.to_string(), "1111111111");
    /// ```
    template<typename Rhs>
        requires store_words_match<Store, Rhs>
    auto operator-(const BitStore<Rhs>& rhs) const {
        auto result = BitVec<word_type>::from(store());
        result -= rhs;
        return result;
    }

    /// @}
    /// @name String representations:
    /// @{

    /// Returns a binary string representation of the store.
    ///
    /// The string is formatted as a sequence of `0`s and `1`s with the least significant bit on the right.
    ///
    /// @param sep The separator between bit elements which defaults to no separator.
    /// @param pre The prefix to add to the string which defaults to no prefix.
    /// @param post The postfix to add to the string which defaults to no postfix.
    ///
    /// # Example
    /// ```
    /// BitVec v{10};
    /// assert_eq(v.to_string(), "0000000000");
    /// v.set(0);
    /// assert_eq(v.to_string(), "1000000000");
    /// assert_eq(v.to_string(",", "[", "]"), "[1,0,0,0,0,0,0,0,0,0]");
    /// ```
    inline std::string to_string(std::string_view sep = "", std::string_view pre = "",
                                 std::string_view post = "") const {
        return to_binary_string(sep, pre, post);
    }

    /// Returns a "pretty" string representation of the store.
    ///
    /// The output is a string of 0's and 1's with spaces between each bit, and the whole thing enclosed in square
    /// brackets.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<>::alternating(10);
    /// assert_eq(v.to_pretty_string(), "[1,0,1,0,1,0,1,0,1,0]");
    /// BitVec empty;
    /// assert_eq(empty.to_pretty_string(), "[]");
    /// ```
    inline std::string to_pretty_string() const { return to_binary_string(",", "[", "]"); }

    /// Returns a binary string representation of the store.
    ///
    /// The string is formatted as a sequence of `0`s and `1`s with the least significant bit on the right.
    ///
    /// @param sep The separator between bit elements which defaults to no separator.
    /// @param pre The prefix to add to the string which defaults to no prefix.
    /// @param post The postfix to add to the string which defaults to no postfix.
    ///
    /// # Example
    /// ```
    /// BitVec v{10};
    /// assert_eq(v.to_binary_string(), "0000000000");
    /// v.set(0);
    /// assert_eq(v.to_binary_string(), "1000000000");
    /// assert_eq(v.to_binary_string(",", "[", "]"), "[1,0,0,0,0,0,0,0,0,0]");
    /// ```
    std::string to_binary_string(std::string_view sep = "", std::string_view pre = "",
                                 std::string_view post = "") const {
        // Edge case ...
        if (is_empty()) { return std::format("{}{}", pre, post); }

        // Start with the raw binary string by iterating over the words ...
        // We preallocate enough space to hold the entire string (actually this is usually a bit bigger than we need).
        auto        num_words = words();
        std::string binary_string;
        binary_string.reserve(num_words * bits_per_word);
        for (auto i = 0uz; i < num_words; ++i) {
            auto w = reverse_bits(word(i));
            binary_string.append(gf2::to_binary_string(w));
        }

        // The last word may not be fully occupied and we need to remove the spurious zeros
        binary_string.resize(size());

        // If there is no separator, return early ...
        if (sep.empty()) { return std::format("{}{}{}", pre, binary_string, post); }

        // Build `result` with separators between each character ...
        std::string result;
        result.reserve(pre.size() + size() * (sep.size() + 1) + post.size());
        result.append(pre);
        for (auto i = 0uz; i < size(); ++i) {
            result.push_back(binary_string[i]);
            if (i != size() - 1) { result.append(sep); }
        }
        result.append(post);
        return result;
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
    /// BitVec v0;
    /// assert_eq(v0.to_hex_string(), "");
    /// auto v1 = BitVec<>::ones(4);
    /// assert_eq(v1.to_hex_string(), "F");
    /// auto v2 = BitVec<>::ones(5);
    /// assert_eq(v2.to_hex_string(), "F1.2");
    /// auto v3 = BitVec<>::alternating(8);
    /// assert_eq(v3.to_binary_string(), "10101010");
    /// assert_eq(v3.to_hex_string(), "AA");
    /// ```
    std::string to_hex_string() const {
        // Edge case: No bits in the store, return the empty string.
        if (is_empty()) return "";

        // The number of digits in the output string. Generally hexadecimal but the last may be to a lower base.
        auto digits = (size() + 3) / 4;

        // Preallocate space allowing for a possible lower base on the last digit such as "_2".
        std::string result;
        result.reserve(digits + 2);

        //  Reverse the bits in each word to vector-order and get the fully padded hex string representation.
        for (auto i = 0uz; i < words(); ++i) {
            auto w = reverse_bits(word(i));
            result.append(gf2::to_hex_string<word_type>(w));
        }

        // Last word may not be fully occupied and padded with spurious zeros so we truncate the output string.
        result.resize(digits);

        // Every four elements is encoded by a single hex digit but `size()` may not be a multiple of 4.
        auto k = size() % 4;
        if (k != 0) {
            // That last hex digit should really be encoded to a lower base -- 2, 4 or 8.
            // We compute the number represented by the trailing `k` elements in the bit-vector.
            int num = 0;
            for (auto i = 0uz; i < k; ++i)
                if (get(size() - 1 - i)) num |= 1 << i;

            // Convert that number to hex & use it to *replace* the last hex digit in our `result` string.
            // Also append the appropriate base to the output string so that the last digit can be interpreted properly.
            result.resize(result.size() - 1);
            result.append(std::format("{:X}.{}", num, 1 << k));
        }
        return result;
    }

    /// Returns a multi-line string describing the bit-vector in some detail.
    ///
    /// This method is useful for debugging but you should not rely on the output format which may change.
    std::string describe() const {
        std::string result;
        result.reserve(1000);
        result += std::format("binary format:        {}\n", to_binary_string());
        result += std::format("hex format:           {}\n", to_hex_string());
        result += std::format("number of bits:       {}\n", size());
        result += std::format("number of set bits:   {}\n", count_ones());
        result += std::format("number of unset bits: {}\n", count_zeros());
        result += std::format("bits per word:        {}\n", bits_per_word);
        result += std::format("word count:           {}\n", words());
        auto words_vec = to_words();
        result += std::format("words in hex:         {::#x}\n", words_vec);
        ;
        return result;
    }

    // The usual output stream operator for a bit-store
    constexpr std::ostream& operator<<(std::ostream& s) const { return s << to_string(); }
};

// --------------------------------------------------------------------------------------------------------------------
// Equality operator ...
// -------------------------------------------------------------------------------------------------------------------

/// Checks that any pair of bit-stores are equal in content.
///
/// Equality here means that the two bit-stores have identical content and *also* the same underlying word_type word.
///
// # Example
/// ```
/// auto u = BitVec<u8>::ones(55);
/// auto v = BitVec<u8>::ones(55);
/// assert(u == v);
/// v.set(23, false);
/// assert(u != v);
/// ```
template<typename Lhs, typename Rhs>
constexpr bool
operator==(const BitStore<Lhs>& lhs, const BitStore<Rhs>& rhs) {
    if constexpr (!store_words_match<Lhs, Rhs>) return false;
    if (&lhs != &rhs) {
        if (lhs.size() != rhs.size()) return false;
        for (auto i = 0uz; i < lhs.words(); ++i)
            if (lhs.word(i) != rhs.word(i)) return false;
    }
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Other functions ...
// -------------------------------------------------------------------------------------------------------------------

/// Returns a new bit-vector formed by joining two bit-stores `lhs` and `rhs`.
///
/// This is one of the few methods in library that allows the operands to have different word types.
/// The returned bit-vector will use the same word type as `lhs`.
///
/// # Example
/// ```
/// auto lhs = BitVec<u16>::zeros(12);
/// auto rhs = BitVec<u8>::ones(12);
/// auto v = join(lhs, rhs);
/// assert_eq(v.to_string(), "000000000000111111111111");
/// ```
template<typename Lhs, typename Rhs>
auto
join(const BitStore<Lhs>& lhs, const BitStore<Rhs>& rhs) {
    using word_type = typename store_word<Lhs>::word_type;
    auto result = BitVec<word_type>::from(lhs);
    result.append(rhs);
    return result;
}

/// Returns the dot product of `lhs` and `rhs` as a boolean value.
///
/// For any pair of vector-like types, the dot product is the "sum" of the element-wise products of the two
/// operands $u * v = \sum_i u_i v_i $ where the sum is over all the indices so the two operands must have the same
/// length. In GF(2) the sum is modulo 2 and, by convention, the scalar output is a boolean value.
///
/// @note In debug mode, this methods panics if the lengths of `lhs` and `rhs` do not match.
///
/// # Example
/// ```
/// auto v1 = BitVec<u8>::alternating(10);
/// auto v2 = ~v1;
/// assert_eq(v1*v1, true);
/// assert_eq(v1*v2, false);
/// ```
template<typename Lhs, typename Rhs>
    requires store_words_match<Lhs, Rhs>
constexpr bool
dot(const BitStore<Lhs>& lhs, const BitStore<Rhs>& rhs) {
    gf2_debug_assert_eq(lhs.size(), rhs.size(), "Length mismatch {} != {}", lhs.size(), rhs.size());
    using word_type = typename store_word<Lhs>::word_type;
    auto sum = word_type{0};
    for (auto i = 0uz; i < lhs.words(); ++i) { sum ^= static_cast<word_type>(lhs.word(i) & rhs.word(i)); }
    return count_ones(sum) % 2 == 1;
}

/// Operator `*` is used for the dot product of `lhs` and `rhs` as a boolean value.
///
/// For any pair of vector-like types, the dot product is the "sum" of the element-wise products of the two
/// operands $u * v = \sum_i u_i v_i $ where the sum is over all the indices so the two operands must have the same
/// length. In GF(2) the sum is modulo 2 and, by convention, the scalar output is a boolean value.
///
/// @note In debug mode, this methods panics if the lengths of `lhs` and `rhs` do not match.
///
/// # Example
/// ```
/// auto v1 = BitVec<u8>::alternating(10);
/// auto v2 = ~v1;
/// assert_eq(v1*v1, true);
/// assert_eq(v1*v2, false);
/// ```
template<typename Lhs, typename Rhs>
    requires store_words_match<Lhs, Rhs>
constexpr bool
operator*(const BitStore<Lhs>& lhs, const BitStore<Rhs>& rhs) {
    return dot(lhs, rhs);
}

/// Returns the convolution of this store with the given `rhs` store as a new bit-vector.
///
/// The *convolution* of $u$ and $v$ is a vector with the elements $ (u * v)_k = \sum_j u_j v_{k-j+1} $
/// where the sum is taken over all $j$ such that the indices in the formula are valid.
///
/// # Example
/// ```
/// auto lhs = BitVec<>::ones(3);
/// auto rhs = BitVec<>::ones(2);
/// auto result = convolve(lhs, rhs);
/// assert_eq(result.to_string(), "1001");
/// ```
template<typename Lhs, typename Rhs>
    requires store_words_match<Lhs, Rhs>
auto
convolve(const BitStore<Lhs>& lhs, const BitStore<Rhs>& rhs) {
    using word_type = typename store_word<Lhs>::word_type;

    // Edge case: if either store is empty then the convolution is empty.
    if (lhs.is_empty() || rhs.is_empty()) return BitVec<word_type>{};

    // Generally the result will have length `self.size() + other.size() - 1` (could be all zeros).
    auto result = BitVec<word_type>::zeros(lhs.size() + rhs.size() - 1);

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
            auto left = static_cast<word_type>(prev >> (BITS<word_type> - 1));
            prev = result.word(j);
            result.set_word(j, static_cast<word_type>(prev << 1) | left);
        }

        if (lhs.get(i)) {
            for (auto j = 0uz; j < rhs_words_end; ++j) result.set_word(j, result.word(j) ^ rhs.word(j));
        }
    }
    return result;
}

} // namespace gf2

// --------------------------------------------------------------------------------------------------------------------
// Specialise `std::formatter` to handle bit-stores ...
// -------------------------------------------------------------------------------------------------------------------

/// Specialises `std::formatter` for our `BitStore<word_type>` type.
///
/// You can use the format specifier to alter the output:
/// - `{}` -> binary string (no formatting)
/// - `{:p}` -> pretty-string (with square brackets)
/// - `{:x}` -> hex string
///
/// # Example
/// ```
/// auto p = BitVec<u8>::alternating(10);
/// auto ps = std::format("{}", p);
/// auto pp = std::format("{:p}", p);
/// auto px = std::format("{:x}", p);
/// assert_eq(ps, "1010101010");
/// assert_eq(pp, "[1,0,1,0,1,0,1,0,1,0]");
/// assert_eq(px, "AA2.4");
/// ```
template<typename Store>
struct std::formatter<gf2::BitStore<Store>> {

    // Parse a bit-store format specifier where where we recognize {:p} and {:x}
    constexpr auto parse(const std::format_parse_context& ctx) {
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

    // Push out a formatted bit-vector using the various @c to_string(...) methods in the class.
    template<class FormatContext>
    auto format(const gf2::BitStore<Store>& rhs, FormatContext& ctx) const {
        // Was there a format specification error?
        if (m_error) return std::format_to(ctx.out(), "'UNRECOGNIZED FORMAT SPECIFIER FOR BIT-STORE'");

        // Special handling requested?
        if (m_hex) return std::format_to(ctx.out(), "{}", rhs.to_hex_string());
        if (m_pretty) return std::format_to(ctx.out(), "{}", rhs.to_pretty_string());

        // Default
        return std::format_to(ctx.out(), "{}", rhs.to_string());
    }

    bool m_hex = false;
    bool m_pretty = false;
    bool m_error = false;
};
