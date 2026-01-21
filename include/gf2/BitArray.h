#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// Fixed-size vectors over GF(2) with compactly stored bit-elements in a standard array of primitive unsigned words.
/// <br> See the [BitArray](docs/pages/BitArray.md) page for more details.

#include <gf2/BitStore.h>
#include <gf2/Iterators.h>
#include <gf2/BitRef.h>
#include <gf2/BitSpan.h>
#include <gf2/BitArray.h>

namespace gf2 {

/// A fixed-size vector over GF(2) with `N` bit elements compactly stored in a standard array of primitive unsigned
/// words whose type is given by the template parameter `Word`. The elements in a bitset are initially all set to 0.
///
/// `BitArray<N>` is similar to `std::bitset<N>` but has a richer set of functionality.
///
/// The `BitArray` class satisfies the `BitStore` concept.
template<usize N, Unsigned Word = usize>
class BitArray {
private:
    // The bit elements are packed compactly into this standard array of unsigned words
    std::array<Word, words_needed<Word>(N)> m_store{};

public:
    /// The underlying unsigned word type used to store the bits.
    using word_type = Word;

    /// The number of bits per `Word`.
    static constexpr u8 bits_per_word = BITS<Word>;

    /// @name Required BitStore Concept Methods.
    /// @{

    /// Returns the number of bit-elements in the bitset.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// assert_eq(v.size(), 10);
    /// ```
    constexpr usize size() const { return N; }

    /// Returns the number of words in the bitset's underlying word store.
    ///
    /// The bit-elements are packed into a standard array with this number of words.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v0;
    /// assert_eq(v0.words(), 2);
    /// BitArray<10, u16> v1;
    /// assert_eq(v1.words(), 1);
    /// ```
    constexpr usize words() const { return m_store.size(); }

    /// Returns word `i` from the bitset's underlying word store.
    ///
    /// The final word in the store may not be fully occupied but we guarantee that unused bits are set to 0.
    ///
    /// @note In debug mode the index is bounds checked.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v;
    /// assert_eq(v.to_string(), "0000000000");
    /// v.set_all();
    /// assert_eq(v.to_string(), "1111111111");
    /// assert_eq(v.words(), 2);
    /// assert_eq(v.word(0), 0b1111'1111);
    /// assert_eq(v.word(1), 0b0000'0011);
    /// ```
    constexpr Word word(usize i) const {
        gf2_debug_assert(i < words(), "Index {} is too large for a bitset with {} words.", i, words());
        return m_store[i];
    }

    /// Sets word `i` in the bitset's underlying word store to `value` (masked if necessary).
    ///
    /// The final word in the store may not be fully occupied but we ensure that unused bits remain set to 0.
    ///
    /// @note In debug mode the index is bounds checked.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v;
    /// assert_eq(v.to_string(), "0000000000");
    /// v.set_word(1, 0b1111'1111);
    /// assert_eq(v.to_string(), "0000000011");
    /// assert_eq(v.count_ones(), 2);
    /// ```
    constexpr void set_word(usize i, Word word) {
        gf2_debug_assert(i < words(), "Index {} is too large for a bitset with {} words.", i, words());

        // Set the value and if it's the last word, make sure it is kept clean
        m_store[i] = word;
        if (i == words() - 1) clean();
    }

    /// Returns a const pointer to the underlying store of words .
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v;
    /// v.set_all();
    /// auto ptr = v.store();
    /// assert_eq(*ptr, 0b1111'1111);
    /// ```
    constexpr const Word* store() const { return m_store.data(); }

    /// Returns a pointer to the underlying store of words.
    ///
    /// @note The pointer is non-const but you should be careful about using it to modify the words in the store.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v;
    /// v.set_all();
    /// auto ptr = v.store();
    /// assert_eq(*ptr, 0b1111'1111);
    /// ```
    constexpr Word* store() { return m_store.data(); }

    /// Returns the offset (in bits) of the first bit in the store within the first word.
    ///
    /// This is always zero for `BitArray`.
    constexpr u8 offset() const { return 0; }

    /// @}
    /// @name Constructors:
    /// @{

    /// Constructs a bit-array of length `N` with all the bit elements set to 0.
    ///
    /// # Example
    /// ```
    /// BitArray<0> u;
    /// assert_eq(u.to_string(), "");
    /// BitArray<10, u8> v;
    /// assert_eq(v.to_string(), "0000000000");
    /// ```
    explicit constexpr BitArray() {
        for (usize i = 0; i < words(); ++i) m_store[i] = 0;
    }

    /// Constructs a bit-array with `N` elements by repeatedly copying all the bits from `word`.
    ///
    /// The final copy of `word` may be truncated and padded with zeros (unused bit slots are always set to zero).
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v{u8{0b0101'0101}};
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    explicit constexpr BitArray(Word word) {
        // Fill the store with the given word & then clean up any excess bits
        for (usize i = 0; i < words(); ++i) m_store[i] = word;
        clean();
    }

    /// @}
    /// @name Factory Constructors:
    /// @{

    /// Factory method to generate a bit-array of length `N` where the elements are all 0.
    ///
    /// # Example
    /// ```
    /// auto v = BitArray<10, u8>::zeros();
    /// assert_eq(v.to_string(), "0000000000");
    /// ```
    static constexpr BitArray zeros() { return BitArray{}; }

    /// Factory method to generate a bit-array of length `N` where the elements are all 1.
    ///
    /// # Example
    /// ```
    /// auto v = BitArray<10, u8>::ones();
    /// assert_eq(v.to_string(), "1111111111");
    /// ```
    static constexpr BitArray ones() { return BitArray{MAX<Word>}; }

    /// Factory method to generate a bit-array of length `N` where the elements are set to `value`.
    ///
    /// # Example
    /// ```
    /// auto v = BitArray<10>::constant(true);
    /// assert_eq(v.to_string(), "1111111111");
    /// auto w = BitArray<10>::constant(false);
    /// assert_eq(w.to_string(), "0000000000");
    /// ```
    static constexpr BitArray constant(bool value) { return BitArray{value ? MAX<Word> : Word{0}}; }

    /// Factory method to generate a "unit" bit-array of length `N`  where only element `i` is set.
    ///
    /// This method panics if the condition `i < N` is not met.
    ///
    /// # Example
    /// ```
    /// assert_eq(BitArray<10>::unit(0).to_string(), "1000000000");
    /// assert_eq(BitArray<10>::unit(9).to_string(), "0000000001");
    /// ```
    static constexpr BitArray unit(usize i) {
        gf2_assert(i < N, "Unit axis i = {} should be less than the bit-array size n = {}", i, N);
        BitArray result{};
        result.set(i);
        return result;
    }

    /// Factory method to generate a bit-array of length `N` looking like `101010...`
    ///
    /// # Example
    /// ```
    /// auto v = BitArray<10, u8>::alternating();
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    static constexpr BitArray alternating() { return BitArray{gf2::ALTERNATING<Word>}; }

    /// @}
    /// @name Helper Method:
    /// @{

    /// Sets any unused bits in the *last* occupied word to 0.
    ///
    /// You can use this helper method to enforce the guarantee that unused bits in the store are always set to 0.
    constexpr void clean() {
        // NOTE: This works fine even if `size() == 0`.
        auto shift = N % bits_per_word;
        if (shift != 0) m_store[words() - 1] &= Word(~(MAX<Word> << shift));
    }

    /// Factory method to construct a bit-array from the bits of a `std::bitset`.
    ///
    /// @note `std::bitset` prints its bit elements in *bit-order*  ...b2b1b0., we print in *vector-order* b0b1b2...
    ///
    /// # Example
    /// ```
    /// std::bitset<10> src{0b1010101010};
    /// auto v = BitArray<10>::from(src);
    /// assert_eq(v.to_string(), "0101010101");
    /// ```
    static constexpr BitArray from(std::bitset<N> const& src) {
        BitArray result{};
        result.copy(src);
        return result;
    }

    /// Factory method to construct a bit-array by repeatedly calling `f(i)` for `i` in `[0, N)`.
    ///
    /// @param f The function to call for each index `i` in `[0, N)`.
    ///
    /// # Example
    /// ```
    /// auto v = BitArray<10, u8>::from([](usize i) { return i % 2 == 0; });
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    static constexpr BitArray from(std::invocable<usize> auto f) {
        BitArray result{};
        result.copy(f);
        return result;
    }

    /// @}
    /// @name Random BitArray Constructors:
    /// @{

    /// Factory method to generate a bit-array of size `N` where the elements are picked at random.
    ///
    /// The default call `BitArray<>::random()` produces a random bit-array with each bit being 1 with probability
    /// 0.5 and where the RNG is seeded from entropy.
    ///
    /// @param p The probability of the elements being 1 (defaults to a fair coin, i.e. 50-50).
    /// @param seed The seed to use for the random number generator (defaults to 0, which means use entropy).
    ///
    /// @note If `p < 0` then the bit-array is all zeros, if `p > 1` then the bit-array is all ones.
    ///
    /// # Example
    /// ```
    /// u64 seed = 1234567890;
    /// auto u = BitArray<10>::random(0.5, seed);
    /// auto v = BitArray<10>::random(0.5, seed);
    /// assert(u == v);
    /// ```
    static BitArray random(double p = 0.5, u64 seed = 0) {
        BitArray result{};
        result.fill_random(p, seed);
        return result;
    }

    /// Factory method to generate a bit-array of size `N` where the elements are from independent fair
    /// coin flips generated from an RNG seeded with the given `seed`.
    ///
    /// This allows one to have reproducible random bit-vectors, which is useful for testing and debugging.
    ///
    /// @param seed The seed to use for the random number generator (if you set this to 0 then entropy is used).
    ///
    /// @note If `p < 0` then the bit-array is all zeros, if `p > 1` then the bit-array is all ones.
    ///
    /// # Example
    /// ```
    /// u64 seed = 1234567890;
    /// auto u = BitArray<10>::seeded_random(seed);
    /// auto v = BitArray<10>::seeded_random(seed);
    /// assert(u == v);
    /// ```
    static BitArray seeded_random(u64 seed) { return random(0.5, seed); }

    /// Factory method to generate a bit-array of size `N` where the elements are from independent fair
    /// coin flips and where each bit is 1 with probability `p`.
    ///
    /// @param p The probability of the elements being 1.
    ///
    /// # Example
    /// ```
    /// auto u = BitArray<10>::biased_random(0.3);
    /// auto v = BitArray<10>::biased_random(0.3);
    /// assert_eq(u.size(), v.size());
    /// ```
    static BitArray biased_random(double p) { return random(p, 0); }

    /// @}
    /// @name Bit Accessors:
    /// @{

    /// Returns `true` if the bit at the given index `i` is set, `false` otherwise.
    ///
    /// @note In debug mode the index `i` is bounds-checked.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// assert_eq(v.get(0), false);
    /// v.set(0);
    /// assert_eq(v.get(0), true);
    /// ```
    constexpr bool get(usize i) const { return gf2::get(*this, i); }

    /// Returns the boolean value of the bit element `i`.
    ///
    /// @note In debug mode the index is bounds-checked.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// assert(v[2] == false);
    /// v[2] = true;
    /// assert(v[2] == true);
    /// assert(v.to_string() == "0010000000");
    /// ```
    constexpr bool operator[](usize i) const { return gf2::get(*this, i); }

    /// Returns `true` if the first bit element is set, `false` otherwise.
    ///
    /// @note In debug mode the method panics of the store is empty.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// assert_eq(v.front(), false);
    /// v.set_all();
    /// assert_eq(v.front(), true);
    /// ```
    constexpr bool front() const { return gf2::front(*this); }

    /// Returns `true` if the last bit element is set, `false` otherwise.
    ///
    /// @note In debug mode the method panics of the store is empty.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// assert_eq(v.back(), false);
    /// v.set_all();
    /// assert_eq(v.back(), true);
    /// ```
    constexpr bool back() const { return gf2::back(*this); }

    /// @}
    /// @name Bit Mutators:
    /// @{

    /// Sets the bit-element `i` to the specified boolean `value` & returns this for chaining.
    /// The default value for `value` is `true`.
    ///
    /// @note In debug mode the index is bounds-checked.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// assert_eq(v[0], false);
    /// v.set(0);
    /// assert_eq(v[0], true);
    /// ```
    auto set(usize i, bool value = true) { return gf2::set(*this, i, value); }

    /// Returns a "reference" to the bit element `i`.
    ///
    /// The returned object is a `BitRef` reference for the bit element at `index` rather than a true reference.
    ///
    /// @note The referenced bit-store must continue to exist while the `BitRef` is in use.
    /// @note In debug mode the index `i` is bounds-checked.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// v[2] = true;
    /// assert(v.to_string() == "0010000000");
    /// BitArray<10> w;
    /// w.set_all();
    /// v[3] = w[3];
    /// assert(v.to_string() == "0011000000");
    /// v[4] |= w[4];
    /// assert(v.to_string() == "0011100000");
    /// ```
    constexpr auto operator[](usize i) { return gf2::ref(*this, i); }

    /// Flips the value of the bit-element `i` and returns this for chaining.
    ///
    /// @note In debug mode the index is bounds-checked.
    ///
    /// # Example
    /// ```
    /// auto v = BitArray<10, u8>::ones();
    /// v.flip(0);
    /// assert_eq(v.to_string(), "0111111111");
    /// v.flip(1);
    /// assert_eq(v.to_string(), "0011111111");
    /// v.flip(9);
    /// assert_eq(v.to_string(), "0011111110");
    /// ```
    auto flip(usize i) { return gf2::flip(*this, i); }

    /// Swaps the bits in the bit-store at indices `i0` and `i1` and returns this for chaining.
    ///
    /// @note In debug mode, panics if either of the indices is out of bounds.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
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
    constexpr auto swap(usize i0, usize i1) { return gf2::swap(*this, i0, i1); }

    /// @}
    /// @name Store Queries:
    /// @{

    /// Returns `true` if the store is empty, `false` otherwise.
    ///
    /// # Example
    /// ```
    /// BitArray<0> v;
    /// assert_eq(v.is_empty(), true);
    /// BitArray<10> u;
    /// assert_eq(u.is_empty(), false);
    /// ```
    constexpr bool is_empty() const { return gf2::is_empty(*this); }

    /// Returns `true` if at least one bit in the store is set, `false` otherwise.
    ///
    /// @note  Empty stores have no set bits (logical connective for `any` is `OR` with identity `false`).
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// assert_eq(v.any(), false);
    /// v.set(0);
    /// assert_eq(v.any(), true);
    /// ```
    constexpr bool any() const { return gf2::any(*this); }

    /// Returns `true` if all bits in the store are set, `false` otherwise.
    ///
    /// @note  Empty stores have no set bits (logical connective for `all` is `AND` with identity `true`).
    ///
    /// # Example
    /// ```
    /// BitArray<3> v;
    /// assert_eq(v.all(), false);
    /// v.set(0);
    /// v.set(1);
    /// v.set(2);
    /// assert_eq(v.all(), true);
    /// ```
    constexpr bool all() const { return gf2::all(*this); }

    /// Returns `true` if no bits in the store are set, `false` otherwise.
    ///
    /// @note  Empty store have no set bits (logical connective for `none` is `AND` with identity `true`).
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// assert_eq(v.none(), true);
    /// v.set(0);
    /// assert_eq(v.none(), false);
    /// ```
    constexpr bool none() const { return gf2::none(*this); }

    /// @}
    /// @name Store Mutators:
    /// @{

    /// Sets the bits in the store to the boolean `value` and returns a reference to this for chaining.
    ///
    /// By default, all bits are set to `true`.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// v.set_all();
    /// assert_eq(v.to_string(), "1111111111");
    /// ```
    auto set_all(bool value = true) { return gf2::set_all(*this, value); }

    /// Flips the value of the bits in the store and returns a reference to this for chaining.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// v.flip_all();
    /// assert_eq(v.to_string(), "1111111111");
    /// ```
    auto flip_all() { return gf2::flip_all(*this); }

    /// @}
    /// @name Copying into the Store:
    /// @{

    /// Copies the bits from an unsigned integral `src` value and returns a reference to this for chaining.
    ///
    /// # Notes:
    /// 1. The size of the store *must* match the number of bits in the source type.
    /// 2. We allow *any* unsigned integral source, e.g. copying a single `u64` into a `BitArray<u8>` of size 64.
    /// 3. The least-significant bit of the source becomes the bit at index 0 in the store.
    ///
    /// # Example
    /// ```
    /// BitArray<16, u8> v;
    /// u16 src = 0b1010101010101010;
    /// v.copy(src);
    /// assert_eq(v.to_string(), "0101010101010101");
    /// BitArray<16, u32> w;
    /// w.copy(src);
    /// assert_eq(w.to_string(), "0101010101010101");
    /// ```
    template<Unsigned Src>
    auto copy(Src src) {
        return gf2::copy(src, *this);
    }

    /// Copies the bits from an equal-sized `src` store and returns a reference to this for chaining.
    ///
    /// @note This is one of the few methods in the library that *doesn't* require the two stores to have the same
    /// `word_type`. You can use it to convert between different `word_type` stores (e.g., from `BitArray<u32>` to
    /// `BitArray<u8>`) as long as the sizes match.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u64> v;
    /// assert_eq(v.to_string(), "0000000000");
    /// v.copy(BitVec<u8>::alternating(10));
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    template<BitStore Src>
    auto copy(Src const& src) {
        return gf2::copy(src, *this);
    }

    /// Copies the bits of an equal-sized `std::bitset` and returns a reference to this for chaining.
    ///
    /// @note `std::bitset` prints its bit elements in *bit-order* which is the reverse of our convention.
    ///
    /// # Example
    /// ```
    /// std::bitset<10> src{0b1010101010};
    /// BitArray<10> v;
    /// v.copy(src);
    /// assert_eq(v.to_string(), "0101010101");
    /// ```
    auto copy(std::bitset<N> const& src) { return gf2::copy(src, *this); }

    /// @}
    /// @name Store Fills:
    /// @{

    /// Fill the store by repeatedly calling `f(i)` and returns a reference to this for chaining.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// v.copy([](usize i) { return i % 2 == 0; });
    /// assert_eq(v.size(), 10);
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    auto copy(std::invocable<usize> auto f) { return gf2::copy(*this, f); }

    /// Fill the store with random bits and returns a reference to this for chaining.
    ///
    /// The default call `fill_random()` sets each bit to 1 with probability 0.5 (fair coin).
    ///
    /// @param p The probability of the elements being 1 (defaults to a fair coin, i.e. 50-50).
    /// @param seed The seed to use for the random number generator (defaults to 0, which means use entropy).
    ///
    /// @note If `p < 0` then the fill is all zeros, if `p > 1` then the fill is all ones.
    ///
    /// # Example
    /// ```
    /// BitArray<10> u, v;
    /// u64 seed = 1234567890;
    /// u.fill_random(0.5, seed);
    /// v.fill_random(0.5, seed);
    /// assert(u == v);
    /// ```
    auto fill_random(double p = 0.5, u64 seed = 0) { return gf2::fill_random(*this, p, seed); }

    /// @}
    /// @name Bit Counts:
    /// @{

    /// Returns the number of set bits in the store.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// assert_eq(v.count_ones(), 0);
    /// v.set(0);
    /// assert_eq(v.count_ones(), 1);
    /// ```
    constexpr usize count_ones() const { return gf2::count_ones(*this); }

    /// Returns the number of unset bits in the store.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// assert_eq(v.count_zeros(), 10);
    /// v.set(0);
    /// assert_eq(v.count_zeros(), 9);
    /// ```
    constexpr usize count_zeros() const { return gf2::count_zeros(*this); }

    /// Returns the number of leading zeros in the store.
    ///
    /// # Example
    /// ```
    /// BitArray<37, u8> v;
    /// assert_eq(v.leading_zeros(), 37);
    /// v.set(27);
    /// assert_eq(v.leading_zeros(), 27);
    /// BitArray<10, u8> w;
    /// w.set_all();
    /// assert_eq(w.leading_zeros(), 0);
    /// ```
    constexpr usize leading_zeros() const { return gf2::leading_zeros(*this); }

    /// Returns the number of trailing zeros in the store.
    ///
    /// # Example
    /// ```
    /// BitArray<27, u8> v;
    /// assert_eq(v.trailing_zeros(), 27);
    /// v.set(0);
    /// assert_eq(v.trailing_zeros(), 26);
    /// ```
    constexpr usize trailing_zeros() const { return gf2::trailing_zeros(*this); }

    /// @}
    /// @name Set-bit Indices:
    /// @{

    /// Returns the index of the first set bit in the bit-store or `{}` if no bits are set.
    ///
    /// # Example
    /// ```
    /// BitArray<37, u8> v;
    /// assert(v.first_set() == std::optional<usize>{});
    /// v.set(2);
    /// assert(v.first_set() == std::optional<usize>{2});
    /// v.set(2, false);
    /// assert(v.first_set() == std::optional<usize>{});
    /// v.set(27);
    /// assert(v.first_set() == std::optional<usize>{27});
    /// BitArray<0> empty;
    /// assert(empty.first_set() == std::optional<usize>{});
    /// ```
    constexpr std::optional<usize> first_set() const { return gf2::first_set(*this); }

    /// Returns the index of the last set bit in the bit-store or `{}` if no bits are set.
    ///
    /// # Example
    /// ```
    /// BitArray<37, u8> v;
    /// assert(v.last_set() == std::optional<usize>{});
    /// v.set(2);
    /// assert(v.last_set() == std::optional<usize>{2});
    /// v.set(27);
    /// assert(v.last_set() == std::optional<usize>{27});
    /// BitArray<0> empty;
    /// assert(empty.last_set() == std::optional<usize>{});
    /// ```
    constexpr std::optional<usize> last_set() const { return gf2::last_set(*this); }

    /// Returns the index of the next set bit after `index` in the store or `{}` if no more set bits exist.
    ///
    /// # Example
    /// ```
    /// BitArray<37, u8> v;
    /// assert(v.next_set(0) == std::optional<usize>{});
    /// v.set(2);
    /// v.set(27);
    /// assert(v.next_set(0) == std::optional<usize>{2});
    /// assert(v.next_set(2) == std::optional<usize>{27});
    /// assert(v.next_set(27) == std::optional<usize>{});
    /// ```
    constexpr std::optional<usize> next_set(usize index) const { return gf2::next_set(*this, index); }

    /// Returns the index of the previous set bit before `index` in the store or `{}` if there are none.
    ///
    /// # Example
    /// ```
    /// BitArray<37, u8> v;
    /// assert(v.previous_set(36) == std::optional<usize>{});
    /// v.set(2);
    /// v.set(27);
    /// assert(v.previous_set(36) == std::optional<usize>{27});
    /// assert(v.previous_set(27) == std::optional<usize>{2});
    /// assert(v.previous_set(2)  == std::optional<usize>{});
    /// ```
    constexpr std::optional<usize> previous_set(usize index) const { return gf2::previous_set(*this, index); }

    /// @}
    /// @name Unset-bit Indices:
    /// @{

    /// Returns the index of the first unset bit in the bit-store or `{}` if no bits are unset.
    ///
    /// # Example
    /// ```
    /// BitArray<37, u8> v; v.set_all();
    /// assert(v.first_unset() == std::optional<usize>{});
    /// v.set(2, false);
    /// assert(v.first_unset() == std::optional<usize>{2});
    /// v.set(2);
    /// assert(v.first_unset() == std::optional<usize>{});
    /// v.set(27, false);
    /// assert(v.first_unset() == std::optional<usize>{27});
    /// BitArray<0> empty;
    /// assert(empty.first_unset() == std::optional<usize>{});
    /// ```
    constexpr std::optional<usize> first_unset() const { return gf2::first_unset(*this); }

    /// Returns the index of the last unset bit in the bit-store or `{}` if no bits are unset.
    ///
    /// # Example
    /// ```
    /// BitArray<37, u8> v; v.set_all();
    /// assert(v.last_unset() == std::optional<usize>{});
    /// v.set(2, false);
    /// assert(v.last_unset() == std::optional<usize>{2});
    /// v.set(2);
    /// assert(v.last_unset() == std::optional<usize>{});
    /// v.set(27, false);
    /// assert(v.last_unset() == std::optional<usize>{27});
    /// BitArray<0> empty;
    /// assert(empty.last_unset() == std::optional<usize>{});
    /// ```
    constexpr std::optional<usize> last_unset() const { return gf2::last_unset(*this); }

    /// Returns the index of the next unset bit after `index` in the store or `{}` if no more unset bits exist.
    ///
    /// # Example
    /// ```
    /// BitArray<37, u8> v; v.set_all();
    /// assert(v.next_unset(0) == std::optional<usize>{});
    /// v.set(2, false);
    /// v.set(27, false);
    /// assert(v.next_unset(0) == std::optional<usize>{2});
    /// assert(v.next_unset(2) == std::optional<usize>{27});
    /// assert(v.next_unset(27) == std::optional<usize>{});
    /// BitArray<0> empty;
    /// assert(empty.next_unset(0) == std::optional<usize>{});
    /// ```
    constexpr std::optional<usize> next_unset(usize index) const { return gf2::next_unset(*this, index); }

    /// Returns the index of the previous unset bit before `index` in the store or `{}` if no more unset bits exist.
    ///
    /// # Example
    /// ```
    /// BitArray<37, u8> v; v.set_all();
    /// assert(v.previous_unset(0) == std::optional<usize>{});
    /// v.set(2, false);
    /// v.set(27, false);
    /// assert(v.previous_unset(36) == std::optional<usize>{27});
    /// assert(v.previous_unset(27) == std::optional<usize>{2});
    /// assert(v.previous_unset(2) == std::optional<usize>{});
    /// BitArray<0> empty;
    /// assert(empty.previous_unset(0) == std::optional<usize>{});
    /// ```
    constexpr std::optional<usize> previous_unset(usize index) const { return gf2::previous_unset(*this, index); }

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
    /// BitArray<37, u8> v; v.set_all();
    /// for (auto&& bit : v.bits()) assert_eq(bit, true);
    /// ```
    constexpr auto bits() const { return gf2::bits(*this); }

    /// Returns a non-const iterator over the values of the bits in the mutable bit-store.
    ///
    /// You can use this iterator to iterate over the bits in the store to get *or* set the value of each bit.
    ///
    /// @note For the most part, try to avoid iterating through individual bits. It is much more efficient to use
    /// methods that work on whole words of bits at a time.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v;
    /// for (auto&& bit : v.bits()) bit = true;
    /// assert_eq(v.to_string(), "1111111111");
    /// ```
    constexpr auto bits() { return gf2::bits(*this); }

    /// Returns an iterator over the *indices* of any *set* bits in the bit-store.
    ///
    /// You can use this iterator to iterate over the set bits in the store and get the index of each bit.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v;
    /// v.copy([](usize i) { return i % 2 == 0; });
    /// assert_eq(v.to_string(), "1010101010");
    /// auto indices = std::ranges::to<std::vector>(v.set_bits());
    /// assert_eq(indices, (std::vector<usize>{0, 2, 4, 6, 8}));
    /// ```
    constexpr auto set_bits() const { return gf2::set_bits(*this); }

    /// Returns an iterator over the *indices* of any *unset* bits in the bit-store.
    ///
    /// You can use this iterator to iterate over the unset bits in the store and get the index of each bit.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v;
    /// v.copy([](usize i) { return i % 2 == 0; });
    /// assert_eq(v.to_string(), "1010101010");
    /// auto indices = std::ranges::to<std::vector>(v.unset_bits());
    /// assert_eq(indices, (std::vector<usize>{1, 3, 5, 7, 9}));
    /// ```
    constexpr auto unset_bits() const { return gf2::unset_bits(*this); }

    /// Returns a const iterator over all the *words* underlying the bit-store.
    ///
    /// You can use this iterator to iterate over the words in the store and read the `Word` value of each word.
    /// You **cannot** use this iterator to modify the words in the store.
    ///
    /// @note The words here may be a synthetic construct. The expectation is that the bit `0` in the store is
    /// located at the bit-location `0` of `word(0)`. That is always the case for bit-vectors but bit-slices typically
    /// synthesise "words" on the fly from adjacent pairs of bit-array words. Nevertheless, almost all the methods
    /// in `BitStore` are implemented efficiently by operating on those words.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v; v.set_all();
    /// assert_eq(v.to_string(), "1111111111");
    /// auto words = std::ranges::to<std::vector>(v.store_words());
    /// assert_eq(words, (std::vector<u8>{0b1111'1111, 0b0000'0011}));
    /// ```
    constexpr auto store_words() const { return gf2::store_words(*this); }

    /// Returns a copy of the words underlying this bit-store.
    ///
    /// @note The last word in the vector may not be fully occupied but unused slots will be all zeros.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v; v.set_all();
    /// auto words = v.to_words();
    /// assert_eq(words, (std::vector<u8>{0b1111'1111, 0b0000'0011}));
    /// ```
    constexpr auto to_words() const { return gf2::to_words(*this); }

    /// @}
    /// @name Spans:
    /// @{

    /// Returns an *immutable* bit-span encompassing the store's bits in the half-open range `[begin, end)`.
    ///
    /// Immutability here is deep -- the interior pointer in the returned span is to *const* words.
    ///
    /// @note This method panics if the span range is not valid.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v;
    /// v.copy([](usize i) { return i % 2 == 0; });
    /// auto s = v.span(1,5);
    /// assert_eq(s.to_string(), "0101");
    /// ```
    constexpr auto span(usize begin, usize end) const { return gf2::span(*this, begin, end); }

    /// Returns a mutable bit-span encompassing the bits in the half-open range `[begin, end)`.
    ///
    /// Mutability here is deep -- the interior pointer in the returned span is to *non-const* words.
    ///
    /// @note This method panics if the span range is not valid.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// v.copy([](usize i) { return i % 2 == 0; });
    /// auto s = v.span(1,5);
    /// assert_eq(s.to_string(), "0101");
    /// s.set_all();
    /// assert_eq(s.to_string(), "1111");
    /// assert_eq(v.to_string(), "1111101010");
    /// ```
    constexpr auto span(usize begin, usize end) { return gf2::span(*this, begin, end); }

    /// @}
    /// @name Sub-vectors:
    /// @{

    /// Returns a *clone* of the elements in the half-open range `[begin, end)` as a new bit-array.
    ///
    /// @note This method panics if the range is not valid.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// v.copy([](usize i) { return i % 2 == 0; });
    /// auto s = v.sub(1,5);
    /// assert_eq(s.to_string(), "0101");
    /// s.set_all();
    /// assert_eq(s.to_string(), "1111");
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    constexpr auto sub(usize begin, usize end) const { return gf2::sub(*this, begin, end); }

    /// @}
    /// @name Splits:
    /// @{

    /// Views a bit-store as two parts containing the elements `[0, at)` and `[at, size())` respectively.
    ///
    /// Clones of the parts are stored in the passed bit-vectors `left` and `right`.
    ///
    /// On return, `left` contains the bits from the start of the bit-array up to but not including `at` and `right`
    /// contains the bits from `at` to the end of the bit-array. This bit-array itself is not modified.
    ///
    /// This lets one reuse the `left` and `right` destinations without having to allocate new bit-vectors.
    /// This is useful when implementing iterative algorithms that need to split a bit-array into two parts repeatedly.
    ///
    /// @note This method panics if the split point is beyond the end of the bit-array.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v;
    /// v.copy([](usize i) { return i % 2 == 0; });
    /// BitVec<u8> left, right;
    /// v.split_at(5, left, right);
    /// assert_eq(left.to_string(), "10101");
    /// assert_eq(right.to_string(), "01010");
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    constexpr void split_at(usize at, BitVec<word_type>& left, BitVec<word_type>& right) const {
        return gf2::split(*this, at, left, right);
    }

    /// Views a bit-store as two parts containing the elements `[0, at)` and `[at, size())` respectively.
    ///
    /// Clones of the parts are returned as a pair of bit-vectors [`left`, `right`].
    ///
    /// On return, `left` is a clone of the bits from the start of the bit-array up to but not including `at` and
    /// `right` contains the bits from `at` to the end of the bit-array. This bit-array itself is not modified.
    ///
    /// @note This method panics if the split point is beyond the end of the bit-array.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v;
    /// v.copy([](usize i) { return i % 2 == 0; });
    /// auto [left, right] = v.split_at(5);
    /// assert_eq(left.to_string(), "10101");
    /// assert_eq(right.to_string(), "01010");
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    constexpr auto split_at(usize at) const { return gf2::split(*this, at); }

    /// @}
    /// @name Riffling:
    /// @{

    /// Interleaves the bits of this bit-store with zeros storing the result into the bit-array `dst`.
    ///
    /// On return, `dst` will have the bits of this bit-store interleaved with zeros. For example, if this
    /// bit-store has the bits `abcde` then `dst` will have the bits `a0b0c0d0e`.
    ///
    /// @note There is no last zero bit in `dst`.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v; v.set_all();
    /// BitVec<u8> dst;
    /// v.riffled(dst);
    /// assert_eq(dst.to_string(), "1010101010101010101");
    /// ```
    constexpr void riffled(BitVec<word_type>& dst) const { return gf2::riffle(*this, dst); }

    /// Returns a new bit-array that is the result of riffling the bits in this bit-store with zeros.
    ///
    /// If bit-store has the bits `abcde` then the output bit-array will have the bits `a0b0c0d0e`.
    ///
    /// @note There is no last zero bit in `dst`.
    ///
    /// # Example
    /// ```
    /// BitArray<10, u8> v; v.set_all();
    /// auto dst = v.riffled();
    /// assert_eq(dst.to_string(), "1010101010101010101");
    /// ```
    constexpr auto riffled() const { return gf2::riffle(*this); }

    /// @}
    /// @name String Representations:
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
    /// BitArray<10> v;
    /// assert_eq(v.to_binary_string(), "0000000000");
    /// v.set(0);
    /// assert_eq(v.to_binary_string(), "1000000000");
    /// assert_eq(v.to_binary_string(",", "[", "]"), "[1,0,0,0,0,0,0,0,0,0]");
    /// ```
    std::string to_binary_string(std::string_view sep = "", std::string_view pre = "",
                                 std::string_view post = "") const {
        return gf2::to_binary_string(*this, sep, pre, post);
    }

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
    /// BitArray<10, u8> v;
    /// assert_eq(v.to_string(), "0000000000");
    /// v.set(0);
    /// assert_eq(v.to_string(), "1000000000");
    /// assert_eq(v.to_string(",", "[", "]"), "[1,0,0,0,0,0,0,0,0,0]");
    /// ```
    std::string to_string(std::string_view sep = "", std::string_view pre = "", std::string_view post = "") const {
        return gf2::to_string(*this, sep, pre, post);
    }

    /// Returns a "pretty" string representation of the store.
    ///
    /// The output is a string of 0's and 1's with spaces between each bit, and the whole thing enclosed in square
    /// brackets.
    ///
    /// # Example
    /// ```
    /// BitArray<10> v;
    /// v.copy([](usize i) { return i % 2 == 0; });
    /// assert_eq(v.to_pretty_string(), "[1,0,1,0,1,0,1,0,1,0]");
    /// BitArray<0> empty;
    /// assert_eq(empty.to_pretty_string(), "[]");
    /// ```
    std::string to_pretty_string() const { return gf2::to_pretty_string(*this); }

    /// Returns the "hex" string representation of the bits in the bit-store
    ///
    /// The output is a string of hex characters without any spaces, commas, or other formatting.
    ///
    /// The string may have a two character *suffix* of the form ".base" where `base` is one of 2, 4 or 8. <br>
    /// All hex characters encode 4 bits: "0X0" -> `0b0000`, "0X1" -> `0b0001`, ..., "0XF" -> `0b1111`. <br>
    /// The three possible ".base" suffixes allow for bit-vectors whose length is not a multiple of 4. <br>
    /// Empty bit-vectors are represented as the empty string.
    ///
    /// - `0X1`   is the hex representation of the bit-array `0001` => length 4.
    /// - `0X1.8` is the hex representation of the bit-array `001`  => length 3.
    /// - `0X1.4` is the hex representation of the bit-array `01`   => length 2.
    /// - `0X1.2` is the hex representation of the bit-array `1`    => length 1.
    ///
    /// The output is in *vector-order*.
    /// If "h0" is the first hex digit in the output string, you can print it as four binary digits `v_0v_1v_2v_3`.
    /// For example, if h0 = "A" which is `1010` in binary, then v = 1010.
    ///
    /// # Example
    /// ```
    /// BitArray<0> v0;
    /// assert_eq(v0.to_hex_string(), "");
    /// BitArray<4> v1; v1.set_all();
    /// assert_eq(v1.to_hex_string(), "F");
    /// BitArray<5> v2; v2.set_all();
    /// assert_eq(v2.to_hex_string(), "F1.2");
    /// BitArray<8> v3;
    /// v3.copy([](usize i) { return i % 2 == 0; });
    /// assert_eq(v3.to_binary_string(), "10101010");
    /// assert_eq(v3.to_hex_string(), "AA");
    /// ```
    std::string to_hex_string() const { return gf2::to_hex_string(*this); }

    /// Returns a multi-line string describing the bit-store in some detail.
    ///
    /// This method is useful for debugging but you should not rely on the output format which may change.
    std::string describe() const { return gf2::describe(*this); }

    /// @}
};

} // namespace gf2
