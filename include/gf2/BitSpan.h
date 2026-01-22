#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// Non-owning *views* of a range of bits from some underlying contiguous unsigned words. <br>
/// See the [BitSpan](docs/pages/BitSpan.md) page for more details.

#include <gf2/BitStore.h>
#include <gf2/Iterators.h>
#include <gf2/BitRef.h>
#include <gf2/BitVector.h>

namespace gf2 {

/// A *bit_span* is a non-owning view of contiguous bits in a bit-store. <br>
///
/// Bit-spans created from const bit-stores will have a type like `BitSpan<const u64>`, while bit-spans from
/// non-const bit-stores will have a type like `BitSpan<u64>`. The existence or absence of that `const` qualifier in
/// the template paramater is important as it allows us to maintain const-correctness for the underlying container.
/// The `BitSpan<const u64>` type is read-only, the `BitSpan<u64>` type is read-write.
///
/// What matters is the *interior* const-ness of the span. In fact, you can generally pass a bit-span by value and not
/// worry about passing a reference/const reference -- the type is small enough to pass on the stack.
/// This is similar in spirit to the `std::span<T>` template class.
///
/// The `BitSpan` class satisfies the `BitStore` concept so is itself a bit-span. This means you can take spans
/// of spans, pass them to methods that accept bit-stores, and so forth.
template<Unsigned Word = usize>
class BitSpan {
private:
    // A pointer to the first word containing bits in the span (it may be partially occupied).
    Word* m_store = nullptr;

    // The bit-span begins at this bit-offset inside `m_store[0]`.
    u8 m_offset = 0;

    // The number of bits in the span.
    usize m_size = 0;

    // The number of words (synthesised) needed to hold the bits in the span (cached for efficiency).
    usize m_words = 0;

public:
    // The underlying span uses words of this type (where we remove any `const` qualifier).
    using word_type = std::remove_const_t<Word>;

    /// A constant -- the number of bits in a `Word`.
    static constexpr u8 bits_per_word = BITS<Word>;

    /// @name Constructors
    /// @{

    /// Constructs a non-owning view over bits stored in contiguous words --- a bit-span.
    ///
    /// @param data   A pointer to the first word that holds the span's bits.
    /// @param offset The bit-offset within `data[0]` where the span begins.
    /// @param size   The number of bits in the span.
    ///
    /// A `BitSpan` is *non-owning* view into a contiguous span of `size` bits which are assumed to be stored in
    /// contiguous words starting at the `offset` bit of the word `data[0]`.
    ///
    /// Generally bit-spans are constructed using the `span` method of any `gf2::BitStore` compatible object.
    ///
    /// @note It is the responsibility of the caller to ensure that the underlying store of unsigneds continues to
    /// exist for as long as the `BitSpan` is in use.
    ///
    /// # Example
    /// ```
    /// std::vector<u8> words{0b1010'1010, 0b1100'1100};
    /// BitSpan s{words.data(), 0, 16};
    /// assert_eq(s.to_string(), "0101010100110011");
    /// ```
    BitSpan(Word* data, u8 offset, usize size) {

        // Check that offset is valid.
        gf2_debug_assert(offset < bits_per_word, "Span begins at bit-offset {} > bits in a word ({})!", offset,
                         bits_per_word);

        // Set the member variables.
        m_store = data;
        m_offset = offset;
        m_size = size;
        m_words = gf2::words_needed<Word>(m_size);
    }

    /// @}
    /// @name Required BitStore Concept Methods:
    /// @{

    /// Returns the number of bits in the span.
    ///
    /// # Example
    /// ```
    /// std::vector<u8> words{0b1010'1010, 0b1100'1100};
    /// BitSpan s{words.data(), 0, 16};
    /// assert_eq(s.size(), 16);
    /// ```
    constexpr usize size() const { return m_size; }

    /// Returns the *minimum* number of words needed to hold the bits in the span.
    ///
    /// These spans are views into a contiguous range of bits from some underlying array of unsigned words.
    /// Generally a bit-span is not aligned with the word boundaries of that array. However, the bit-span can synthesise
    /// words *as if* it copied the bits and shifted them down so that element 0 is at bit-position zero in synthetic
    /// word number 0. This function returns the number of such words.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::ones(18);
    /// assert_eq(v.words(), 3);
    /// auto s = v.span(4, 12);     // The span covers words 0 and 1 in the span underlying v.
    /// assert_eq(s.words(), 1);    // However, it can be fitted into a single synthetic word!
    /// ```
    constexpr usize words() const { return m_words; }

    /// Returns a "word"'s worth of bits from the bit-span.
    ///
    /// These spans are views into a contiguous range of bits from some underlying array of unsigned words.
    /// Generally a bit-span is not aligned with the word boundaries of that array. However, the bit-span can synthesise
    /// words *as if* it copied the bits and shifted them down so that element 0 is at bit-position zero in synthetic
    /// word number 0. This function returns those words by combining bits from pairs of adjacent words in the true
    /// underlying array.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::from_string("0000'0000'1111'1111").value();
    /// auto s = v.span(4, 12);     // The span covers words 0 and 1 in the span underlying v.
    /// assert_eq(s.words(), 1);    // However, it can be fitted into a single synthetic word!
    /// assert_eq(s.word(0), 0b1111'0000);
    /// ```
    constexpr word_type word(usize i) const {
        // Get the recipe for the "word" at index `i` (we may need two real words to synthesise it).
        auto [w0_bits, w1_bits] = recipe_for_word(i);

        // Our return value starts with all unset bits.
        word_type result = 0;

        // Replace `w0_bits` low-order bits in `result` with the same number of high-order bits from `w0`
        Word w0 = m_store[i];
        gf2::replace_bits(result, 0, w0_bits, w0 >> m_offset);

        // Perhaps our result spans two words.
        if (w1_bits > 0) {
            // Replace `w1_bits` high-order bits in result with the same number of low-order bits from `w1`
            Word w1 = m_store[i + 1];
            gf2::replace_bits(result, w0_bits, w0_bits + w1_bits, w1 << w0_bits);
        }
        return result;
    }

    /// Sets a "word"'s worth of bits in the bit-span.
    ///
    /// These spans are views into a contiguous range of bits from some underlying array of unsigned words.
    /// Generally a bit-span is not aligned with the word boundaries of that array. However, the bit-span can synthesise
    /// words *as if* it copied the bits and shifted them down so that element 0 is at bit-position zero in synthetic
    /// word number 0. This function sets those words by altering bits in pairs of adjacent words in the underlying
    /// array.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::from_string("0000_0000_1111_1111").value();
    /// auto s = v.span(4, 12);     // The span covers words 0 and 1 in the span underlying v.
    /// assert_eq(s.words(), 1);    // However, it can be fitted into a single synthetic word!
    /// assert_eq(s.word(0), 0b1111'0000);
    /// s.set_word(0, 0b1111'1111);
    /// assert_eq(v.to_string(), "0000111111111111");
    /// ```
    constexpr void set_word(usize i, word_type value) {
        if constexpr (std::is_const_v<Word>) {
            // This should never get called for const bit-spans.
            throw std::logic_error("Cannot set a word in a const bit-span.");
        } else {
            // Get the recipe for the "word" at index `i` (it may be a synthesis of two real words).
            auto [w0_bits, w1_bits] = recipe_for_word(i);

            // Replace `w0_bits` low-order bits in `w0` with the same number of high-order bits from `value`
            // Shift `value` to the left to align its bits with the start of `w0`.
            gf2::replace_bits(m_store[i], m_offset, m_offset + w0_bits, value << m_offset);

            // Perhaps we also need to overwrite some bits in the second word.
            if (w1_bits > 0) {
                // Replace `w1_bits` low-order bits in `w1` with the same number of high-order bits from `value`
                gf2::replace_bits(m_store[i + 1], 0, w1_bits, value >> w0_bits);
            }
        }
    }

    /// Returns a pointer to the first span word in some underlying span of words (const version).
    constexpr const Word* store() const { return m_store; }

    /// Returns a pointer to the first span word in some underlying span of words (non-const version).
    constexpr Word* store() { return m_store; }

    /// Returns the offset (in bits) of the first bit in the span within the first span word.
    constexpr u8 offset() const { return m_offset; }

    /// @}
    /// @name Bit Accessors:
    /// @{

    /// Returns `true` if the bit at the given index `i` is set, `false` otherwise.
    ///
    /// @note In debug mode the index `i` is bounds-checked.
    ///
    /// # Example
    /// ```
    /// BitVector u{10};
    /// auto v = u.span(0,2);
    /// assert_eq(v.get(0), false);
    /// u.set(0);
    /// assert_eq(v.get(0), true);
    /// ```
    constexpr bool get(usize i) const { return gf2::get(*this, i); }

    /// Returns the boolean value of the bit element `i`.
    ///
    /// @note In debug mode the index is bounds-checked.
    ///
    /// # Example
    /// ```
    /// BitVector u{10};
    /// auto v = u.span(1, 4);
    /// assert_eq(v[0], false);
    /// ```
    constexpr bool operator[](usize i) const { return gf2::get(*this, i); }

    /// Returns `true` if the first bit element is set, `false` otherwise.
    ///
    /// @note In debug mode the method panics of the span is empty.
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<>::ones(10);
    /// auto v = u.span(0,3);
    /// assert_eq(v.front(), true);
    /// ```
    constexpr bool front() const { return gf2::front(*this); }

    /// Returns `true` if the last bit element is set, `false` otherwise.
    ///
    /// @note In debug mode the method panics of the span is empty.
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<>::ones(10);
    /// auto v = u.span(0,3);
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
    /// auto u = BitVector<>::zeros(10);
    /// auto v = u.span(0,3);
    /// assert_eq(v.get(0), false);
    /// v.set(0);
    /// assert_eq(v.get(0), true);
    /// assert_eq(u.get(0), true);
    /// ```
    auto set(usize i, bool value = true) { return gf2::set(*this, i, value); }

    /// Returns a "reference" to the bit element `i`.
    ///
    /// The returned object is a `BitRef` reference for the bit element at `index` rather than a true reference.
    ///
    /// @note The referenced bit-span must continue to exist while the `BitRef` is in use.
    /// @note In debug mode the index `i` is bounds-checked.
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<>::zeros(10);
    /// auto v = u.span(0,5);
    /// v[2] = true;
    /// assert_eq(v.to_string(), "00100");
    /// assert_eq(u.to_string(), "0010000000");
    /// auto w = BitVector<>::ones(10);
    /// v[3] = w[3];
    /// assert_eq(v.to_string(), "00110");
    /// v[4] |= w[4];
    /// assert_eq(v.to_string(), "00111");
    /// assert_eq(u.to_string(), "0011100000")
    /// ```
    constexpr auto operator[](usize i) { return gf2::ref(*this, i); }

    /// Flips the value of the bit-element `i` and returns this for chaining.
    ///
    /// @note In debug mode the index is bounds-checked.
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<>::ones(10);
    /// auto v = u.span(1,5);
    /// assert_eq(v.to_string(), "1111");
    /// v.flip(0);
    /// assert_eq(v.to_string(), "0111");
    /// assert_eq(u.to_string(), "1011111111");
    /// ```
    auto flip(usize i) { return gf2::flip(*this, i); }

    /// Swaps the bits in the bit-span at indices `i0` and `i1` and returns this for chaining.
    ///
    /// @note In debug mode, panics if either of the indices is out of bounds.
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<>::zeros(10);
    /// auto v = u.span(1,5);
    /// v.set(0);
    /// assert_eq(v.to_string(), "1000");
    /// assert_eq(u.to_string(), "0100000000");
    /// v.swap(0, 1);
    /// assert_eq(v.to_string(), "0100");
    /// assert_eq(u.to_string(), "0010000000");
    /// ```
    constexpr auto swap(usize i0, usize i1) { return gf2::swap(*this, i0, i1); }

    /// @}
    /// @name Span Queries:
    /// @{

    /// Returns `true` if the span is empty, `false` otherwise.
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<>::zeros(10);
    /// auto v = u.span(1,1);
    /// assert_eq(v.is_empty(), true);
    /// auto w = u.span(0,1);
    /// assert_eq(w.is_empty(), false);
    /// ```
    constexpr bool is_empty() const { return gf2::is_empty(*this); }

    /// Returns `true` if at least one bit in the span is set, `false` otherwise.
    ///
    /// @note  Empty stores have no set bits (logical connective for `any` is `OR` with identity `false`).
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<>::zeros(10);
    /// auto v = u.span(1,3);
    /// assert_eq(v.any(), false);
    /// v.set(0);
    /// assert_eq(v.any(), true);
    /// ```
    constexpr bool any() const { return gf2::any(*this); }

    /// Returns `true` if all bits in the span are set, `false` otherwise.
    ///
    /// @note  Empty stores have no set bits (logical connective for `all` is `AND` with identity `true`).
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<>::zeros(10);
    /// auto v = u.span(1,3);
    /// assert_eq(v.all(), false);
    /// v.set(0);
    /// v.set(1);
    /// assert_eq(v.all(), true);
    /// assert_eq(u.all(), false);
    /// ```
    constexpr bool all() const { return gf2::all(*this); }

    /// Returns `true` if no bits in the span are set, `false` otherwise.
    ///
    /// @note  Empty span have no set bits (logical connective for `none` is `AND` with identity `true`).
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<>::zeros(10);
    /// auto v = u.span(1,3);
    /// assert_eq(v.none(), true);
    /// v.set(0);
    /// assert_eq(v.none(), false);
    /// ```
    constexpr bool none() const { return gf2::none(*this); }

    /// @}
    /// @name Span Mutators:
    /// @{

    /// Sets the bits in the span to the boolean `value` and returns a reference to this for chaining.
    ///
    /// By default, all bits are set to `true`.
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<>::zeros(10);
    /// auto v = u.span(5,10);
    /// v.set_all();
    /// assert_eq(v.to_string(), "11111");
    /// assert_eq(u.to_string(), "0000011111");
    /// ```
    auto set_all(bool value = true) { return gf2::set_all(*this, value); }

    /// Flips the value of the bits in the span and returns a reference to this for chaining.
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<>::zeros(10);
    /// auto v = u.span(5,10);
    /// v.flip_all();
    /// assert_eq(v.to_string(), "11111");
    /// assert_eq(u.to_string(), "0000011111");
    /// ```
    auto flip_all() { return gf2::flip_all(*this); }

    /// @}
    /// @name Copying into the Span:
    /// @{

    /// Copies the bits from an unsigned integral `src` value and returns a reference to this for chaining.
    ///
    /// # Notes:
    /// 1. The size of the span *must* match the number of bits in the source type.
    /// 2. We allow *any* unsigned integral source, e.g. copying a single `u64` into a `BitVector<u8>` of size 64.
    /// 3. The least-significant bit of the source becomes the bit at index 0 in the span.
    ///
    /// # Example
    /// ```
    /// BitVector<u8> v{26};
    /// auto s = v.span(3, 19);
    /// u16 src = 0b1010101010101010;
    /// s.copy(src);
    /// assert_eq(s.to_string(), "0101010101010101");
    /// assert_eq(v.to_string(), "00001010101010101010000000");
    /// BitVector<u32> w{26};
    /// auto t = w.span(3, 19);
    /// t.copy(src);
    /// assert_eq(t.to_string(), "0101010101010101");
    /// assert_eq(w.to_string(), "00001010101010101010000000");
    /// ```
    template<Unsigned Src>
    auto copy(Src src) {
        return gf2::copy(src, *this);
    }

    /// Copies the bits from an equal-sized `src` span and returns a reference to this for chaining.
    ///
    /// @note This is one of the few methods in the library that *doesn't* require the two stores to have the same
    /// `word_type`. You can use it to convert between different `word_type` stores (e.g., from `BitVector<u32>` to
    /// `BitVector<u8>`) as long as the sizes match.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::ones(16);
    /// assert_eq(v.to_string(), "1111111111111111");
    /// auto s = v.span(0,10);
    /// s.copy(BitVector<u8>::alternating(10));
    /// assert_eq(v.to_string(), "1010101010111111");
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
    /// auto v = BitVector<u8>::ones(16);
    /// auto s = v.span(0,10);
    /// s.copy(src);
    /// assert_eq(s.to_string(), "0101010101");
    /// assert_eq(v.to_string(), "0101010101111111");
    /// ```
    template<usize N>
    auto copy(std::bitset<N> const& src) {
        return gf2::copy(src, *this);
    }

    /// @}
    /// @name Fills:
    /// @{

    /// Fill the span by repeatedly calling `f(i)` and returns a reference to this for chaining.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::ones(16);
    /// auto s = v.span(0,10);
    /// s.copy([](usize i) { return i % 2 == 0; });
    /// assert_eq(s.to_string(), "1010101010");
    /// assert_eq(v.to_string(), "1010101010111111");
    /// ```
    auto copy(std::invocable<usize> auto f) { return gf2::copy(*this, f); }

    /// Fill the span with random bits and returns a reference to this for chaining.
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
    /// BitVector u{10}, v{10};
    /// auto s = u.span(2,10);
    /// auto t = v.span(2,10);
    /// u64 seed = 1234567890;
    /// s.fill_random(0.5, seed);
    /// t.fill_random(0.5, seed);
    /// assert(u == v);
    /// ```
    auto fill_random(double p = 0.5, u64 seed = 0) { return gf2::fill_random(*this, p, seed); }

    /// @}
    /// @name Bit Counts:
    /// @{

    /// Returns the number of set bits in the span.
    ///
    /// # Example
    /// ```
    /// BitVector v{10};
    /// auto s = v.span(2,10);
    /// assert_eq(s.count_ones(), 0);
    /// v.set(2);
    /// assert_eq(s.count_ones(), 1);
    /// ```
    constexpr usize count_ones() const { return gf2::count_ones(*this); }

    /// Returns the number of unset bits in the span.
    ///
    /// # Example
    /// ```
    /// BitVector v{10};
    /// auto s = v.span(2,10);
    /// assert_eq(s.count_zeros(), 8);
    /// v.set(2);
    /// assert_eq(s.count_zeros(), 7);
    /// ```
    constexpr usize count_zeros() const { return gf2::count_zeros(*this); }

    /// Returns the number of leading zeros in the span.
    ///
    /// # Example
    /// ```
    /// BitVector v{37};
    /// auto s = v.span(2,10);
    /// assert_eq(s.leading_zeros(), 8);
    /// v.set(11);
    /// assert_eq(s.leading_zeros(), 8);
    /// v.set(2);
    /// assert_eq(s.leading_zeros(), 0);
    /// ```
    constexpr usize leading_zeros() const { return gf2::leading_zeros(*this); }

    /// Returns the number of trailing zeros in the span.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::zeros(27);
    /// auto s = v.span(2,10);
    /// assert_eq(s.trailing_zeros(), 8);
    /// v.set(11);
    /// assert_eq(s.trailing_zeros(), 8);
    /// v.set(2);
    /// assert_eq(s.trailing_zeros(), 7);
    /// ```
    constexpr usize trailing_zeros() const { return gf2::trailing_zeros(*this); }

    /// @}
    /// @name Set-bit Indices:
    /// @{

    /// Returns the index of the first set bit in the bit-span or `{}` if no bits are set.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::zeros(37);
    /// auto s = v.span(4,12);
    /// assert(s.first_set() == std::optional<usize>{});
    /// v.set(2);
    /// assert(s.first_set() == std::optional<usize>{});
    /// v.set(4);
    /// assert(s.first_set() == std::optional<usize>{0});
    /// ```
    constexpr std::optional<usize> first_set() const { return gf2::first_set(*this); }

    /// Returns the index of the last set bit in the bit-span or `{}` if no bits are set.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::zeros(37);
    /// auto s = v.span(4,12);
    /// assert(s.last_set() == std::optional<usize>{});
    /// v.set(27);
    /// assert(s.last_set() == std::optional<usize>{});
    /// v.set(11);
    /// assert_eq(s.to_string(), "00000001");
    /// assert(s.last_set() == std::optional<usize>{7});
    /// ```
    constexpr std::optional<usize> last_set() const { return gf2::last_set(*this); }

    /// Returns the index of the next set bit after `index` in the span or `{}` if no more set bits exist.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::zeros(37);
    /// auto s = v.span(4,12);
    /// assert(s.next_set(0) == std::optional<usize>{});
    /// v.set(5);
    /// v.set(6);
    /// assert(s.next_set(0) == std::optional<usize>{1});
    /// assert(s.next_set(1) == std::optional<usize>{2});
    /// ```
    constexpr std::optional<usize> next_set(usize index) const { return gf2::next_set(*this, index); }

    /// Returns the index of the previous set bit before `index` in the span or `{}` if there are none.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::zeros(37);
    /// auto s = v.span(4,12);
    /// assert(s.previous_set(s.size()) == std::optional<usize>{});
    /// v.set(5);
    /// v.set(6);
    /// assert(s.previous_set(s.size()) == std::optional<usize>{2});
    /// assert(s.previous_set(2) == std::optional<usize>{1});
    /// ```
    constexpr std::optional<usize> previous_set(usize index) const { return gf2::previous_set(*this, index); }

    /// @}
    /// @name Unset-bit Indices:
    /// @{

    /// Returns the index of the first unset bit in the bit-span or `{}` if no bits are unset.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::ones(37);
    /// auto s = v.span(4,12);
    /// assert(s.first_unset() == std::optional<usize>{});
    /// v.set(2, false);
    /// assert(s.first_unset() == std::optional<usize>{});
    /// v.set(4, false);
    /// assert(s.first_unset() == std::optional<usize>{0});
    /// ```
    constexpr std::optional<usize> first_unset() const { return gf2::first_unset(*this); }

    /// Returns the index of the last unset bit in the bit-span or `{}` if no bits are unset.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::ones(37);
    /// auto s = v.span(4,12);
    /// assert(s.last_unset() == std::optional<usize>{});
    /// v.set(27);
    /// assert(s.last_unset() == std::optional<usize>{});
    /// v.set(4, false);
    /// assert_eq(s.to_string(), "01111111");
    /// assert(s.last_unset() == std::optional<usize>{0});
    /// ```
    constexpr std::optional<usize> last_unset() const { return gf2::last_unset(*this); }

    /// Returns the index of the next unset bit after `index` in the span or `{}` if no more unset bits exist.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::ones(37);
    /// auto s = v.span(4,12);
    /// assert(s.last_unset() == std::optional<usize>{});
    /// v.set(27);
    /// assert(s.last_unset() == std::optional<usize>{});
    /// v.set(11, false);
    /// assert_eq(s.to_string(), "11111110");
    /// assert(s.last_unset() == std::optional<usize>{7});
    /// ```
    constexpr std::optional<usize> next_unset(usize index) const { return gf2::next_unset(*this, index); }

    /// Returns the index of the previous unset bit before `index` in the span or `{}` if no more unset bits exist.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::ones(37);
    /// auto s = v.span(4,12);
    /// assert(s.previous_unset(s.size()) == std::optional<usize>{});
    /// v.set(5, false);
    /// v.set(6, false);
    /// assert(s.previous_unset(s.size()) == std::optional<usize>{2});
    /// assert(s.previous_unset(2) == std::optional<usize>{1});
    /// ```
    constexpr std::optional<usize> previous_unset(usize index) const { return gf2::previous_unset(*this, index); }

    /// @}
    /// @name Iterators:
    /// @{

    /// Returns a const iterator over the `bool` values of the bits in the const bit-span.
    ///
    /// You can use this iterator to iterate over the bits in the span and get the values of each bit as a `bool`.
    ///
    /// @note For the most part, try to avoid iterating through individual bits. It is much more efficient to use
    /// methods that work on whole words of bits at a time.
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<u8>::ones(14);
    /// auto s = u.span(4,12);
    /// for (auto&& bit : s.bits()) assert_eq(bit, true);
    /// ```
    constexpr auto bits() const { return gf2::bits(*this); }

    /// Returns a non-const iterator over the values of the bits in the mutable bit-span.
    ///
    /// You can use this iterator to iterate over the bits in the span to get *or* set the value of each bit.
    ///
    /// @note For the most part, try to avoid iterating through individual bits. It is much more efficient to use
    /// methods that work on whole words of bits at a time.
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<u8>::ones(14);
    /// auto s = u.span(4,12);
    /// for (auto&& bit : s.bits()) bit = false;
    /// assert_eq(s.to_string(), "00000000")
    /// assert_eq(u.to_string(), "11110000000011");
    /// ```
    constexpr auto bits() { return gf2::bits(*this); }

    /// Returns an iterator over the *indices* of any *set* bits in the bit-span.
    ///
    /// You can use this iterator to iterate over the set bits in the span and get the index of each bit.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::alternating(19);
    /// auto s = v.span(4,12);
    /// assert_eq(s.to_string(), "10101010");
    /// auto indices = std::ranges::to<std::vector>(s.set_bits());
    /// assert_eq(indices, (std::vector<usize>{0, 2, 4, 6}));
    /// ```
    constexpr auto set_bits() const { return gf2::set_bits(*this); }

    /// Returns an iterator over the *indices* of any *unset* bits in the bit-span.
    ///
    /// You can use this iterator to iterate over the unset bits in the span and get the index of each bit.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::alternating(19);
    /// auto s = v.span(4,12);
    /// assert_eq(s.to_string(), "10101010");
    /// auto indices = std::ranges::to<std::vector>(s.unset_bits());
    /// assert_eq(indices, (std::vector<usize>{1, 3, 5, 7}));
    /// ```
    constexpr auto unset_bits() const { return gf2::unset_bits(*this); }

    /// Returns a const iterator over all the *words* underlying the bit-span.
    ///
    /// You can use this iterator to iterate over the words in the span and read the `Word` value of each word.
    /// You **cannot** use this iterator to modify the words in the span.
    ///
    /// @note The words here may be a synthetic construct. The expectation is that the bit `0` in the span is
    /// located at the bit-location `0` of `word(0)`. That is always the case for bit-vectors but bit-slices typically
    /// synthesise "words" on the fly from adjacent pairs of bit-vector words. Nevertheless, almost all the methods
    /// in `BitStore` are implemented efficiently by operating on those words.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::ones(100);
    /// auto s = v.span(4,14);
    /// assert_eq(s.to_string(), "1111111111");
    /// auto words = std::ranges::to<std::vector>(s.store_words());
    /// assert_eq(words, (std::vector<u8>{0b1111'1111, 0b0000'0011}));
    /// ```
    constexpr auto store_words() const { return gf2::store_words(*this); }

    /// Returns a copy of the words underlying this bit-span.
    ///
    /// @note The last word in the vector may not be fully occupied but unused slots will be all zeros.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::ones(100);
    /// auto s = v.span(4,14);
    /// assert_eq(s.to_string(), "1111111111");
    /// auto words = s.to_words();
    /// assert_eq(words, (std::vector<u8>{0b1111'1111, 0b0000'0011}));
    /// ```
    constexpr auto to_words() const { return gf2::to_words(*this); }

    /// @}
    /// @name Spans:
    /// @{

    /// Returns an immutable sub-span encompassing the span's bits in the half-open range `[begin, end)`.
    ///
    /// Span mutability is deep -- the interior pointer in the returned sub-span is to *const* words.
    ///
    /// @note This method panics if the sub-span range is not valid.
    ///
    /// # Example
    /// ```
    /// const auto v = BitVector<u8>::alternating(14);
    /// const auto s0 = v.span(4,12);
    /// assert_eq(s0.to_string(), "10101010");
    /// auto s1 = s0.span(4,8);
    /// assert_eq(s1.to_string(), "1010")
    /// ```
    constexpr auto span(usize begin, usize end) const { return gf2::span(*this, begin, end); }

    /// Returns an mutable sub-span encompassing the span's bits in the half-open range `[begin, end)`.
    ///
    /// Mutability here is deep -- the interior pointer in the returned sub-span is to *non-const* words.
    ///
    /// @note This method panics if the sub-span range is not valid.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::alternating(14);
    /// auto s0 = v.span(4,12);
    /// assert_eq(s0.to_string(), "10101010");
    /// auto s1 = s0.span(4,8);
    /// assert_eq(s1.to_string(), "1010")
    /// s1.set_all();
    /// assert_eq(s1.to_string(), "1111")
    /// assert_eq(s0.to_string(), "10101111");
    /// assert_eq(v.to_string(), "10101010111110");
    /// ```
    constexpr auto span(usize begin, usize end) { return gf2::span(*this, begin, end); }

    /// @}
    /// @name Sub-vectors:
    /// @{

    /// Returns a *clone* of the span elements in the half-open range `[begin, end)` as a new bit-vector.
    ///
    /// @note This method panics if the range is not valid.
    ///
    /// # Example
    /// ```
    /// auto u = BitVector<>::alternating(14);
    /// auto s = u.span(4,12);
    /// auto v = s.sub(0,4);
    /// assert_eq(v.to_string(), "1010");
    /// s.set_all();
    /// assert_eq(s.to_string(), "11111111");
    /// assert_eq(u.to_string(), "10101111111110");
    /// assert_eq(v.to_string(), "1010");
    /// ```
    constexpr auto sub(usize begin, usize end) const { return gf2::sub(*this, begin, end); }

    /// @}
    /// @name Splits:
    /// @{

    /// Views a bit-span as two parts containing the elements `[0, at)` and `[at, size())` respectively.
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
    /// auto u = BitVector<>::alternating(14);
    /// auto s = u.span(4,12);
    /// BitVector left, right;
    /// s.split_at(5, left, right);
    /// assert_eq(left.to_string(), "10101");
    /// assert_eq(right.to_string(), "010");
    /// assert_eq(u.to_string(), "10101010101010");
    /// ```
    constexpr void split_at(usize at, BitVector<word_type>& left, BitVector<word_type>& right) const {
        return gf2::split(*this, at, left, right);
    }

    /// Views a bit-span as two parts containing the elements `[0, at)` and `[at, size())` respectively.
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
    /// auto u = BitVector<>::alternating(14);
    /// auto s = u.span(4,12);
    /// auto [left, right] = s.split_at(5);
    /// assert_eq(left.to_string(), "10101");
    /// assert_eq(right.to_string(), "010");
    /// assert_eq(u.to_string(), "10101010101010");
    /// ```
    constexpr auto split_at(usize at) const { return gf2::split(*this, at); }

    /// @}
    /// @name Riffling:
    /// @{

    /// Interleaves the bits of this bit-span with zeros storing the result into the bit-vector `dst`.
    ///
    /// On return, `dst` will have the bits of this bit-span interleaved with zeros. For example, if this
    /// bit-span has the bits `abcde` then `dst` will have the bits `a0b0c0d0e`.
    ///
    /// @note There is no last zero bit in `dst`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::ones(20);
    /// BitVector<u8> dst;
    /// v.span(4,14).riffled(dst);
    /// assert_eq(dst.to_string(), "1010101010101010101");
    /// ```
    constexpr void riffled(BitVector<word_type>& dst) const { return gf2::riffle(*this, dst); }

    /// Returns a new bit-vector that is the result of riffling the bits in this bit-span with zeros.
    ///
    /// If bit-span has the bits `abcde` then the output bit-vector will have the bits `a0b0c0d0e`.
    ///
    /// @note There is no last zero bit in `dst`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::ones(20);
    /// auto dst = v.span(4,14).riffled();
    /// assert_eq(dst.to_string(), "1010101010101010101");
    /// ```
    constexpr auto riffled() const { return gf2::riffle(*this); }

    /// @}
    /// @name String Representations:
    /// @{

    /// Returns a binary string representation of the span.
    ///
    /// The string is formatted as a sequence of `0`s and `1`s with the least significant bit on the right.
    ///
    /// @param sep The separator between bit elements which defaults to no separator.
    /// @param pre The prefix to add to the string which defaults to no prefix.
    /// @param post The postfix to add to the string which defaults to no postfix.
    ///
    /// # Example
    /// ```
    /// BitVector v{14};
    /// auto s = v.span(2,12);
    /// assert_eq(s.to_binary_string(), "0000000000");
    /// s.set(0);
    /// assert_eq(s.to_binary_string(), "1000000000");
    /// assert_eq(v.to_binary_string(",", "[", "]"), "[0,0,1,0,0,0,0,0,0,0,0,0,0,0]");
    /// ```
    std::string to_binary_string(std::string_view sep = "", std::string_view pre = "",
                                 std::string_view post = "") const {
        return gf2::to_binary_string(*this, sep, pre, post);
    }

    /// Returns a binary string representation of the span.
    ///
    /// The string is formatted as a sequence of `0`s and `1`s with the least significant bit on the right.
    ///
    /// @param sep The separator between bit elements which defaults to no separator.
    /// @param pre The prefix to add to the string which defaults to no prefix.
    /// @param post The postfix to add to the string which defaults to no postfix.
    ///
    /// # Example
    /// ```
    /// BitVector v{14};
    /// auto s = v.span(2,12);
    /// assert_eq(s.to_binary_string(), "0000000000");
    /// s.set(0);
    /// assert_eq(s.to_binary_string(), "1000000000");
    /// assert_eq(v.to_binary_string(",", "[", "]"), "[0,0,1,0,0,0,0,0,0,0,0,0,0,0]");
    /// ```
    std::string to_string(std::string_view sep = "", std::string_view pre = "", std::string_view post = "") const {
        return gf2::to_string(*this, sep, pre, post);
    }

    /// Returns a "pretty" string representation of the span.
    ///
    /// The output is a string of 0's and 1's with spaces between each bit, and the whole thing enclosed in square
    /// brackets.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<u8>::alternating(14);
    /// auto s = v.span(2,12);
    /// assert_eq(s.to_pretty_string(), "[1,0,1,0,1,0,1,0,1,0]");
    /// auto empty = v.span(3,3);
    /// assert_eq(empty.to_pretty_string(), "[]");
    /// ```
    std::string to_pretty_string() const { return gf2::to_pretty_string(*this); }

    /// Returns the "hex" string representation of the bits in the bit-span
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
    /// auto v = BitVector<u8>::ones(20);
    /// assert_eq(v.span(4,4).to_hex_string(), "");
    /// assert_eq(v.span(4,8).to_hex_string(), "F");
    /// assert_eq(v.span(4,9).to_hex_string(), "F1.2");
    /// ```
    std::string to_hex_string() const { return gf2::to_hex_string(*this); }

    /// Returns a multi-line string describing the bit-span in some detail.
    ///
    /// This method is useful for debugging but you should not rely on the output format which may change.
    std::string describe() const { return gf2::describe(*this); }

    /// @}

private:
    // A helper method to get the recipe used to "bake" the (generally synthetic) `word(i)`.
    //
    // Spans may not be aligned with the underlying unsigneds so we need to synthesise "words" as needed.
    // To synthesise word `i`, we use two contiguous words from the underlying vector of words, `w0`, and `w1`,
    // where `w0` is always m_words[i] and `w1` is always m_words[i + 1].
    //
    // We copy `w0_bits` high-order bits from `w0` and they become the low-order bits in the span "word": `word(i)`.
    // We copy `w1_bits` low-order bits from `w1` and they become the high-order bits in the span "word": `word(i)`.
    //
    // This "recipe" method returns the pair: `(w0_bits, w1_bits)` which tells you the numbers of needed bits.
    //
    // The only tricky part is that the last span word may not be fully occupied so we need to handle it differently
    // from the others.
    //
    // In debug mode we also check that the index `i` is valid.
    constexpr auto recipe_for_word(usize i) const {
        gf2_debug_assert(i < m_words, "Index {} should be less than {}", i, m_words);

        // The default recipe (these sum to a whole word of bits).
        u8 w0_bits = bits_per_word - m_offset;
        u8 w1_bits = m_offset;

        // The last word in the span may not be fully occupied so we need to adjust the bits accordingly.
        if (i == m_words - 1) {
            auto last_offset = static_cast<u8>((m_offset + m_size - 1) % bits_per_word);
            if (last_offset < m_offset) {
                // Still need to use two words but `w1-bits` may shrink.
                w1_bits = static_cast<u8>(last_offset + 1);
            } else {
                // Only need to use one word so no need for `w1_bits`.
                w0_bits = static_cast<u8>(last_offset - m_offset + 1);
                w1_bits = 0;
            }
        }
        return std::pair{w0_bits, w1_bits};
    }
};

} // namespace gf2
