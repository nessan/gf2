#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// Vectors over GF(2) with compactly stored bit-elements in a standard vector of primitive unsigned words. <br>
/// See the [BitVec](docs/pages/BitVec.md) page for more details.

#include <gf2/BitStore.h>
#include <gf2/Iterators.h>
#include <gf2/BitRef.h>
#include <gf2/BitSpan.h>

#include <charconv>

namespace gf2 {

/// A dynamically-sized vector over GF(2) with bit elements compactly stored in a standard vector of primitive unsigned
/// words whose type is given by the template parameter `Word`.
///
/// The `BitVec` class satisfies the `BitStore` concept.
template<Unsigned Word = usize>
class BitVec {
private:
    // The number of bit elements in the bit-vector.
    usize m_size;

    // The bit elements are packed compactly into this standard vector of unsigned words
    std::vector<Word> m_store;

public:
    /// The underlying unsigned word type used to store the bits.
    using word_type = Word;

    /// The number of bits per `Word`.
    static constexpr u8 bits_per_word = BITS<Word>;

    /// @name Required BitStore Concept Methods:
    /// @{

    /// Returns the number of bit-elements in the bit-vector.
    ///
    /// # Example
    /// ```
    /// BitVec v0;
    /// assert_eq(v0.size(), 0);
    /// BitVec<u8> v1(10);
    /// assert_eq(v1.size(), 10);
    /// ```
    constexpr usize size() const { return m_size; }

    /// Returns the number of words in the bit-vector's underlying word store.
    ///
    /// The bit-elements are packed into a standard vector with this number of words.
    ///
    /// # Example
    /// ```
    /// BitVec v0;
    /// assert_eq(v0.words(), 0);
    /// BitVec<u8> v1(10);
    /// assert_eq(v1.words(), 2);
    /// ```
    constexpr usize words() const { return m_store.size(); }

    /// Returns word `i` from the bit-vector's underlying word store.
    ///
    /// The final word in the store may not be fully occupied but we guarantee that unused bits are set to 0.
    ///
    /// @note In debug mode the index is bounds checked.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(10);
    /// assert_eq(v.to_string(), "1111111111");
    /// assert_eq(v.words(), 2);
    /// assert_eq(v.word(0), 0b1111'1111);
    /// assert_eq(v.word(1), 0b0000'0011);
    /// ```
    constexpr Word word(usize i) const {
        gf2_debug_assert(i < words(), "Index {} is too large for a bit-vector with {} words.", i, words());
        return m_store[i];
    }

    /// Sets word `i` in the bit-vector's underlying word store to `value` (masked if necessary).
    ///
    /// The final word in the store may not be fully occupied but we ensure that unused bits remain set to 0.
    ///
    /// @note In debug mode the index is bounds checked.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::zeros(10);
    /// assert_eq(v.to_string(), "0000000000");
    /// v.set_word(1, 0b1111'1111);
    /// assert_eq(v.to_string(), "0000000011");
    /// assert_eq(v.count_ones(), 2);
    /// ```
    constexpr void set_word(usize i, Word word) {
        gf2_debug_assert(i < words(), "Index {} is too large for a bit-vector with {} words.", i, words());

        // Set the value and if it's the last word, make sure it is kept clean
        m_store[i] = word;
        if (i == words() - 1) clean();
    }

    /// Returns a const pointer to the underlying store of words .
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(10);
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
    /// auto v = BitVec<u8>::ones(10);
    /// auto ptr = v.store();
    /// assert_eq(*ptr, 0b1111'1111);
    /// ```
    constexpr Word* store() { return m_store.data(); }

    /// Returns the offset (in bits) of the first bit in the store within the first word.
    ///
    /// This is always zero for `BitVec`.
    constexpr u8 offset() const { return 0; }

    /// @}
    /// @name Constructors:
    /// @{

    /// Constructs a bit-vector of length `n` with all the bit elements set to 0.
    ///
    /// @note The default constructor returns the empty bit-vector with no elements.
    ///
    /// # Example
    /// ```
    /// BitVec u;
    /// assert_eq(u.to_string(), "");
    /// BitVec<u8> v{10};
    /// assert_eq(v.to_string(), "0000000000");
    /// ```
    explicit constexpr BitVec(usize len = 0) : m_size(len), m_store(gf2::words_needed<Word>(len)) {
        // Empty body -- we now have an underlying vector of words all initialized to 0.
        // Note: We avoided using uniform initialization on the `std::vector` data member.
    }

    /// Constructs a bit-vector with `len` elements by repeatedly copying all the bits from `word`.
    ///
    /// You specify the length `len` of the bit-vector which means the final copy of `word` may be truncated and padded
    /// with zeros (unused bit slots are always set to zero in this library).
    ///
    /// # Example
    /// ```
    /// BitVec<u8> v{10, u8{0b0101'0101}};
    /// assert_eq(v.size(), 10);
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    explicit constexpr BitVec(usize len, Word word) : m_size(len), m_store(gf2::words_needed<Word>(len), word) {
        // Make sure any excess bits are set to 0.
        clean();
    }

    /// @}
    /// @name Factory Constructors:
    /// @{

    /// Factory method to construct an empty bit-vector with at least the specified capacity.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<>::with_capacity(10);
    /// assert_eq(v.size(), 0);
    /// assert(v.capacity() >=10);
    /// ```
    static constexpr BitVec with_capacity(usize capacity) {
        auto result = BitVec<Word>{};
        result.m_store.reserve(gf2::words_needed<Word>(capacity));
        return result;
    }

    /// Factory method to generate a bit-vector of length `n` where the elements are all 0.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<>::zeros(10);
    /// assert_eq(v.to_string(), "0000000000");
    /// ```
    static constexpr BitVec zeros(usize n) { return BitVec{n}; }

    /// Factory method to generate a bit-vector of length `n` where the elements are all 1.
    ///
    /// # Example
    /// ```
    /// assert_eq(BitVec<>::ones(10).to_string(), "1111111111");
    /// ```
    static constexpr BitVec ones(usize n) { return BitVec{n, MAX<Word>}; }

    /// Factory method to generate a bit-vector of length `n` where the elements are set to `value`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<>::constant(10, true);
    /// assert_eq(v.to_string(), "1111111111");
    /// auto w = BitVec<>::constant(10, false);
    /// assert_eq(w.to_string(), "0000000000");
    /// ```
    static constexpr BitVec constant(usize n, bool value) { return BitVec{n, value ? MAX<Word> : Word{0}}; }

    /// Factory method to generate a "unit" bit-vector of length `n`  where only element `i` is set.
    ///
    /// This method panics if the condition `i < n` is not met.
    ///
    /// # Example
    /// ```
    /// assert_eq(BitVec<>::unit(10, 0).to_string(), "1000000000");
    /// assert_eq(BitVec<>::unit(10, 9).to_string(), "0000000001");
    /// ```
    static constexpr BitVec unit(usize n, usize i) {
        gf2_assert(i < n, "Unit axis i = {} should be less than the bit-vector size n = {}", i, n);
        BitVec result{n};
        result.set(i);
        return result;
    }

    /// Factory method to generate a bit-vector of length `n` looking like `101010...`
    ///
    /// # Example
    /// ```
    /// assert_eq(BitVec<u8>::alternating(10).to_string(), "1010101010");
    /// ```
    static constexpr BitVec alternating(usize n) { return BitVec{n, gf2::ALTERNATING<Word>}; }

    /// Factory method to construct a bit-vector by copying *all* the bits from *any* `Unsigned` instance.
    /// The resulting bit-vector will have the same size as the number of bits in the `src` unsigned integer.
    ///
    /// # Example
    /// ```
    /// u8 s8 = 0b01010101;
    /// auto u = BitVec<u8>::from(s8);
    /// assert_eq(u.size(), 8);
    /// assert_eq(u.to_string(), "10101010");
    /// u16 s16 = 0b0101010101010101;
    /// auto v = BitVec<u8>::from(s16);
    /// assert_eq(v.size(), 16);
    /// assert_eq(v.to_string(), "1010101010101010");
    /// auto w = BitVec<u32>::from(s8);
    /// assert_eq(w.size(), 8);
    /// assert_eq(w.to_string(), "10101010");
    /// ```
    template<Unsigned Src>
    static constexpr BitVec from(Src src) {
        BitVec result{gf2::BITS<Src>};
        result.copy(src);
        return result;
    }

    /// Factory method to construct a bit-vector by copying *all* the bits from *any* bit-store instance.
    ///
    /// # Note
    /// Generally, we do not support interactions between bit-stores that use different underlying unsigned word types.
    /// This method is an exception -- the `src` bit-store may use a different unsigned type from the one used here.
    /// It makes it possible to convert between bit-vector types which is occasionally useful.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::from(BitVec<u8>::ones(10));
    /// assert_eq(v.size(), 10);
    /// assert_eq(v.to_string(), "1111111111");
    /// auto w = BitVec<u8>::from(BitVec<u16>::ones(20));
    /// assert_eq(w.size(), 20);
    /// assert_eq(w.to_string(), "11111111111111111111");
    /// auto x = BitVec<u8>::from(BitVec<u32>::zeros(10));
    /// assert_eq(x.size(), 10);
    /// assert_eq(x.to_string(), "0000000000");
    /// ```
    template<BitStore Src>
    static constexpr BitVec from(Src const& src) {
        BitVec result{src.size()};
        result.copy(src);
        return result;
    }

    /// Factory method to construct a bit-vector from the bits of a `std::bitset`.
    ///
    /// @note `std::bitset` prints its bit elements in *bit-order*  ...b2b1b0., we print in *vector-order* b0b1b2...
    ///
    /// # Example
    /// ```
    /// std::bitset<10> src{0b1010101010};
    /// auto v = BitVec<>::from(src);
    /// assert_eq(v.to_string(), "0101010101");
    /// ```
    template<usize N>
    static constexpr BitVec from(std::bitset<N> const& src) {
        BitVec result{N};
        result.copy(src);
        return result;
    }

    /// Factory method to construct a bit-vector by repeatedly calling `f(i)` for `i` in `[0, len)`.
    ///
    /// @param len The length of the bit-vector to generate.
    /// @param f The function to call for each index `i` in `[0, len)`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::from(10, [](usize i) { return i % 2 == 0; });
    /// assert_eq(v.size(), 10);
    /// assert_eq(v.to_string(), "1010101010");
    /// ```
    static constexpr BitVec from(usize len, std::invocable<usize> auto f) {
        BitVec result{len};
        result.copy(f);
        return result;
    }

    /// @}
    /// @name Random Bit-Vector Constructors:
    /// @{

    /// Factory method to generate a bit-vector of size `len` where the elements are picked at random.
    ///
    /// The default call `BitVec<>::random(len)` produces a random bit-vector with each bit being 1 with probability
    /// 0.5 and where the RNG is seeded from entropy.
    ///
    /// @param len The length of the bit-vector to generate.
    /// @param p The probability of the elements being 1 (defaults to a fair coin, i.e. 50-50).
    /// @param seed The seed to use for the random number generator (defaults to 0, which means use entropy).
    ///
    /// @note If `p < 0` then the bit-vector is all zeros, if `p > 1` then the bit-vector is all ones.
    ///
    /// # Example
    /// ```
    /// u64 seed = 1234567890;
    /// auto u = BitVec<>::random(10, 0.5, seed);
    /// auto v = BitVec<>::random(10, 0.5, seed);
    /// assert(u == v);
    /// ```
    static BitVec random(usize len, double p = 0.5, u64 seed = 0) {
        BitVec result{len};
        result.fill_random(p, seed);
        return result;
    }

    /// Factory method to generate a bit-vector of size `len` where the elements are from independent fair
    /// coin flips generated from an RNG seeded with the given `seed`.
    ///
    /// This allows one to have reproducible random bit-vectors, which is useful for testing and debugging.
    ///
    /// @param len The length of the bit-vector to generate.
    /// @param seed The seed to use for the random number generator (if you set this to 0 then entropy is used).
    ///
    /// @note If `p < 0` then the bit-vector is all zeros, if `p > 1` then the bit-vector is all ones.
    ///
    /// # Example
    /// ```
    /// u64 seed = 1234567890;
    /// auto u = BitVec<>::seeded_random(10, seed);
    /// auto v = BitVec<>::seeded_random(10, seed);
    /// assert(u == v);
    /// ```
    static BitVec seeded_random(usize len, u64 seed) { return random(len, 0.5, seed); }

    /// Factory method to generate a bit-vector of size `len` where the elements are from independent fair
    /// coin flips and where each bit is 1 with probability `p`.
    ///
    /// @param len The length of the bit-vector to generate.
    /// @param p The probability of the elements being 1.
    ///
    /// # Example
    /// ```
    /// auto u = BitVec<>::biased_random(10, 0.3);
    /// auto v = BitVec<>::biased_random(10, 0.3);
    /// assert_eq(u.size(), v.size());
    /// ```
    static BitVec biased_random(usize len, double p) { return random(len, p, 0); }

    /// @}
    /// @name Constructors from Strings:
    /// @{

    /// Factory method to construct a bit-vector from a string `s`, returning `std::nullopt` on failure.
    ///
    /// The string can contain whitespace, commas, single quotes, and underscores and optionally a "0b", "0x", or "0X"
    /// prefix. If there is no prefix, and the string only contains '0' and '1' characters, we assume the string is
    /// binary. To force getting it interpreted as a hex string, add a prefix of "0x" or "0X".
    ///
    /// A hex string can have a suffix of ".2", ".4", or ".8" to indicate the base of the last digit/character.
    /// This allows for bit-vectors of any length as opposed to just a multiple of 4.
    ///
    /// # Example
    /// ```
    /// auto v1 = BitVec<>::from_string("0b1010_1010_10").value();
    /// assert_eq(v1.to_string(), "1010101010");
    /// auto v2 = BitVec<>::from_string("AA").value();
    /// assert_eq(v2.to_string(), "10101010");
    /// auto v3 = BitVec<>::from_string("1010'1010").value();
    /// assert_eq(v3.to_string(), "10101010");
    /// auto v4 = BitVec<>::from_string("0x1.8").value();
    /// assert_eq(v4.to_string(), "001");
    /// ```
    static std::optional<BitVec> from_string(std::string_view sv) {
        // Edge case ...
        if (sv.empty()) return BitVec<Word>{};

        // Remove any whitespace, commas, single quotes, or underscores characters.
        std::string s{sv};
        std::erase_if(s, [](char c) { return c == ' ' || c == ',' || c == '\'' || c == '_'; });
        bool no_punctuation = true;

        // Check for a binary prefix.
        if (s.starts_with("0b")) return from_binary_string(s, no_punctuation);

        // Check for a hex prefix.
        if (s.starts_with("0x") || s.starts_with("0X")) return from_hex_string(s, no_punctuation);

        // No prefix, but perhaps the string only contains '0' and '1' characters.
        if (std::ranges::all_of(s, [](char c) { return c == '0' || c == '1'; })) {
            return BitVec::from_binary_string(s, no_punctuation);
        }

        // Last gasp -- try hex ...
        return BitVec::from_hex_string(s, no_punctuation);
    }

    /// Factory method to construct a bit-vector from a binary string, returning `std::nullopt` on failure.
    ///
    /// The string can contain whitespace, commas, single quotes, and underscores.
    /// If the second argument is true, then the string is assumed to have none of the above characters.
    /// There can always be a "0b" prefix.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::from_binary_string("0b1010'1010'10").value();
    /// assert_eq(v.to_string(), "1010101010");
    /// auto u = BitVec<u8>::from_binary_string("").value();
    /// assert_eq(u.to_string(), "");
    /// ```
    static std::optional<BitVec> from_binary_string(std::string_view sv, bool no_punctuation = false) {
        // Edge case ...
        if (sv.empty()) return BitVec<Word>{};

        // Convert to a string & remove the "0b" prefix if it exists.
        std::string s{sv};
        if (s.starts_with("0b")) s = s.substr(2);

        // If necessary, also remove any whitespace, commas, single quotes, or underscores.
        if (!no_punctuation) std::erase_if(s, [](char c) { return c == ' ' || c == ',' || c == '\'' || c == '_'; });

        // The string should now be a sequence of 0's and 1's.
        if (std::ranges::any_of(s, [](char c) { return c != '0' && c != '1'; })) return std::nullopt;

        // Construct the bit-vector.
        auto result = BitVec::zeros(s.size());
        for (auto i = 0uz; i < s.size(); ++i)
            if (s[i] == '1') result.set(i);
        return result;
    }

    /// Factory method to construct a bit-vector from a hex string, returning `std::nullopt` on failure.
    ///
    /// The hex string should consist of the characters 0-9, A-F, a-f, with an optional prefix "0x" or "0X".
    /// The string may also have a suffix of the form ".base" where `base` is one of 2, 4 or 8 which indicates that
    /// the last digit should be interpreted as a base `base` number. This allows for bit-vectors whose length is
    /// not a multiple of 4.
    ///
    /// # Example
    /// ```
    /// auto v1 = gf2::BitVec<>::from_hex_string("0xAA").value();
    /// assert_eq(v1.to_string(), "10101010");
    /// auto v2 = gf2::BitVec<>::from_hex_string("0x1").value();
    /// assert_eq(v2.to_string(), "0001");
    /// auto v3 = gf2::BitVec<>::from_hex_string("0x1.8").value();
    /// assert_eq(v3.to_string(), "001");
    /// auto v4 = gf2::BitVec<>::from_hex_string("0x1.4").value();
    /// assert_eq(v4.to_string(), "01");
    /// auto v5 = gf2::BitVec<>::from_hex_string("0x1.2").value();
    /// assert_eq(v5.to_string(), "1");
    /// ```
    static std::optional<BitVec> from_hex_string(std::string_view sv, bool no_punctuation = false) {
        // Edge case ...
        if (sv.empty()) return BitVec<Word>{};

        // Convert to a string and remove the optional "0x" prefix if it exists.
        std::string s{sv};
        if (s.starts_with("0x") || s.starts_with("0X")) s = s.substr(2);

        // By default, the base of the last digit is 16 just like all the others.
        // However, there may be a suffix of the form ".base" where `base` is one of 2, 4 or 8.
        int last_digit_base = 16;
        if (s.ends_with(".2"))
            last_digit_base = 2;
        else if (s.ends_with(".4"))
            last_digit_base = 4;
        else if (s.ends_with(".8"))
            last_digit_base = 8;

        // Remove the suffix if it exists.
        if (last_digit_base != 16) s = s.substr(0, s.size() - 2);

        // If necessary, also remove any whitespace, commas, single quotes, or underscores.
        if (!no_punctuation) std::erase_if(s, [](char c) { return c == ' ' || c == ',' || c == '_'; });

        // The string should now be a sequence of 0-9, A-F, a-f.
        if (std::ranges::any_of(s, [](char c) { return !std::isxdigit(c); })) return std::nullopt;

        // Construct the bit-vector where we allow all the characters to be hex digits.
        auto result = BitVec::with_capacity(s.size() * 4);

        // Push all but the last character -- those are hex digits for sure.
        for (auto i = 0uz; i < s.size() - 1; ++i) result.append_hex_digit(s[i]);

        // Push the last character -- it may be a hex digit or a base indicator.
        result.append_digit(s[s.size() - 1], last_digit_base);

        return result;
    }

    /// @}
    /// @name Size and Capacity:
    /// @{

    /// Returns the current capacity of the bit-vector.
    ///
    /// This is the total number of bits that the bit-vector can hold without allocating more memory.
    /// The number *includes* the number of bits already in use.
    ///
    /// # Example
    /// ```
    /// BitVec v0;
    /// assert_eq(v0.capacity(), 0);
    /// BitVec<u64> v1(10);
    /// assert_eq(v1.capacity(), 64);
    /// ```
    constexpr usize capacity() const { return bits_per_word * m_store.capacity(); }

    /// Returns the number of *additional* elements we can store in the bit-vector without reallocating.
    ///
    /// # Example
    /// ```
    /// BitVec<u64> v1(10);
    /// assert_eq(v1.remaining_capacity(), 54);
    /// ```
    constexpr usize remaining_capacity() const { return capacity() - size(); }

    /// Shrinks the bit-vector's capacity as much as possible.
    ///
    /// This method may do nothing.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(1000);
    /// v.resize(15);
    /// v.shrink_to_fit();
    /// assert_eq(v.capacity(), 16);
    /// ```
    constexpr BitVec& shrink_to_fit() {
        m_store.resize(gf2::words_needed<Word>(size()));
        m_store.shrink_to_fit();
        return *this;
    }

    /// Removes all elements from the bit-vector so @c size()==0.
    ///
    /// The capacity is not changed by this operation.
    constexpr BitVec& clear() {
        m_store.clear();
        m_size = 0;
        return *this;
    }

    /// Resize the bit-vector so that its `size()` is `n`.
    ///
    /// - If `n` is greater than the bit-vector's current size, then the new elements are set to 0.
    /// - If `n` is less than the bit-vector's current size, then the bit-vector is truncated.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(1000);
    /// v.resize(10);
    /// assert_eq(v.to_string(), "1111111111");
    /// v.resize(15);
    /// assert_eq(v.to_string(), "111111111100000");
    /// ```
    constexpr BitVec& resize(usize n) {
        if (n != size()) {
            m_store.resize(gf2::words_needed<Word>(n), 0);
            auto old_size = size();
            m_size = n;

            // If we truncated, we need to clean up the last occupied word if necessary.
            if (n < old_size) clean();
        }
        return *this;
    }

    /// Sets any unused bits in the *last* occupied word to 0.
    ///
    /// This method can be used to enforce the guarantee that unused bits in the store are always set to 0.
    constexpr void clean() {
        // NOTE: This works fine even if `size() == 0`.
        auto shift = m_size % bits_per_word;
        if (shift != 0) m_store[words() - 1] &= Word(~(MAX<Word> << shift));
    }

    /// @}
    /// @name Push/pop Single Elements:
    /// @{

    /// Pushes a single bit `b` onto the bit-vector.
    ///
    /// Returns a reference to the current object for chaining.
    ///
    /// # Example
    /// ```
    /// BitVec v;
    /// v.push(1);
    /// assert(v.to_string() == "1");
    /// v.push(0);
    /// assert(v.to_string() == "10");
    /// ```
    constexpr BitVec& push(bool b) {
        resize(size() + 1);
        if (b) this->set(size() - 1, true);
        return *this;
    }

    /// Removes the last bit from the bit-vector and returns it or `std::nullopt` if the bit-vector is empty.
    ///
    /// # Example
    /// ```
    /// BitVec v;
    /// v.push(1);
    /// v.push(0);
    /// assert(v.to_string() == "10");
    /// auto b1 = v.pop();
    /// assert_eq(*b1, false);
    /// assert(v.to_string() == "1");
    /// auto b2 = v.pop();
    /// assert_eq(*b2, true);
    /// assert(v.to_string() == "");
    /// auto b3 = v.pop();
    /// assert(b3 == std::nullopt);
    /// ```
    constexpr std::optional<bool> pop() {
        if (m_size == 0) return std::nullopt;
        bool b = this->back();
        resize(m_size - 1);
        return b;
    }

    /// @}
    /// @name Appending from Various Sources:
    /// @{

    /// Appends all the bits from any unsigned integral `src` value and returns a reference to this for chaining.
    ///
    /// @note We allow *any* unsigned integral source, e.g. appending a single `u16` into a `BitVec<u8>`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(4);
    /// u16 src = 0b1010101010101010;
    /// v.append(src);
    /// assert_eq(v.to_string(), "11110101010101010101");
    /// auto w = BitVec<u32>::ones(4);
    /// w.append(src);
    /// assert_eq(w.to_string(), "11110101010101010101");
    /// ```
    template<Unsigned Src>
    auto append(Src src) {
        auto old_size = size();
        resize(old_size + BITS<Src>);
        this->span(old_size, size()).copy(src);
        return *this;
    }

    /// Appends all the bits from *any* `BitStore` `src` onto the end of the bit-vector and returns this for chaining.
    ///
    /// @note Generally, we do not support interactions between bit-stores that use different underlying unsigned word
    /// types. This method is an exception, and the `src` bit-store may use a different unsigned type from the one used
    /// here.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::zeros(10);
    /// auto w = BitVec<u16>::ones(10);
    /// v.append(w);
    /// assert_eq(v.size(), 20);
    /// assert_eq(v.to_string(), "00000000001111111111");
    /// ```
    template<BitStore Src>
    constexpr BitVec& append(Src const& src) {
        auto old_size = size();
        resize(old_size + src.size());
        this->span(old_size, size()).copy(src);
        return *this;
    }

    /// Appends all the bits from a `std::bitset` onto the end of the bit-vector and returns this for chaining.
    ///
    /// # Example
    /// ```
    /// std::bitset<10> src{0b1010101010};
    /// BitVec v;
    /// v.append(src);
    /// assert_eq(v.to_string(), "0101010101");
    /// ```
    template<usize N>
    auto append(std::bitset<N> const& src) {
        auto old_size = size();
        resize(old_size + src.size());
        this->span(old_size, size()).copy(src);
        return *this;
    }

    /// Appends a single character `c` onto the end of bit-vector and returns this for chaining
    ///
    /// The character is interpreted as a base `base` number where`base` must be one of 2, 4, 8, 16.
    ///
    /// @note This method does nothing if the base or character is not recognized.
    ///
    /// # Example
    /// ```
    /// BitVec v;
    /// v.append_digit('A', 16);
    /// assert_eq(v.to_string(), "1010");
    /// v.append_digit('X', 16);
    /// assert_eq(v.to_string(), "1010");
    /// v.append_digit('1', 8);
    /// assert_eq(v.to_string(), "1010001");
    /// v.append_digit('1', 4);
    /// assert_eq(v.to_string(), "101000101");
    /// v.append_digit('1', 2);
    /// assert_eq(v.to_string(), "1010001011");
    /// ```
    constexpr BitVec& append_digit(char c, int base) {
        // The known bases are 2, 4, 8, and 16.
        static constexpr std::array<int, 4> known_bases = {2, 4, 8, 16};
        if (std::ranges::contains(known_bases, base)) {
            // Try to convert the character to an integer.
            usize x;
            if (std::from_chars(&c, &c + 1, x, base).ec == std::errc{}) {
                // Success! Push the digits onto the bit-vector.
                auto digits = static_cast<usize>(std::log2(base));
                auto old_len = size();
                resize(old_len + digits);
                for (auto i = 0uz; i < digits; ++i) this->set(old_len + i, (x >> (digits - i - 1)) & 1);
            }
        }
        return *this;
    }

    /// Appends a single hex digit character `c` onto the end of bit-vector and returns this for chaining
    ///
    /// @note This method does nothing if the character is not a hex digit.
    ///
    /// This is the same as `append_digit(c, 16)` but we push hex digits more often than other bases and want to skip
    /// some checks for efficiency.
    ///
    /// # Example
    /// ```
    /// BitVec v;
    /// v.append_hex_digit('F');
    /// assert_eq(v.to_string(), "1111");
    /// v.append_hex_digit('X');
    /// assert_eq(v.to_string(), "1111");
    /// v.append_hex_digit('1');
    /// assert_eq(v.to_string(), "11110001");
    /// ```
    constexpr BitVec& append_hex_digit(char c) {
        // Try to convert the hex digit to an integer.
        int x;
        if (std::from_chars(&c, &c + 1, x, 16).ec == std::errc{}) {
            // Success! Push the digits onto the bit-vector.
            auto old_len = size();
            resize(old_len + 4);
            for (auto i = 0uz; i < 4; ++i) this->set(old_len + i, (x >> (3 - i)) & 1);
        }
        return *this;
    }

    /// @}
    /// @name Removing Elements:
    /// @{

    /// Splits a bit-vector into two at the given index, returning a new `BitVec`.
    ///
    /// The returned bit-vector contains the bits from `at` to the end of the bit-vector.
    /// The bit-vector is resized to only contain the bits in the half-open range `[0, at)`.
    ///
    /// @note This method panics if the split point is beyond the end of the bit-vector.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<>::alternating(10);
    /// auto w = v.split_off(5);
    /// assert_eq(v.to_string(), "10101");
    /// assert_eq(w.to_string(), "01010");
    /// ```
    constexpr BitVec<Word> split_off(usize at) {
        BitVec result;
        split_off(at, result);
        return result;
    }

    /// Splits a bit-vector into two at the given index, returning the second part in `dst`.
    ///
    /// On return, `dst` contains the bits from `at` to the end of the bit-vector.
    /// The bit-vector is resized to only contain the bits in the half-open range `[0, at)`.
    ///
    /// @note This method panics if the split point is beyond the end of the bit-vector.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<>::alternating(10);
    /// BitVec dst;
    /// v.split_off(5, dst);
    /// assert_eq(v.to_string(), "10101");
    /// assert_eq(dst.to_string(), "01010");
    /// ```
    constexpr void split_off(usize at, BitVec<Word>& dst) {
        gf2_assert(at <= m_size, "split point {} is beyond the end of the bit-vector", at);
        dst.clear();
        dst.append(this->span(at, m_size));
        resize(at);
    }

    /// Split off a single arbitrary sized unsigned integer off the end of the bit-vector and returns it or
    /// `std::nullopt` if the bit-vector is empty.
    ///
    /// @note You can split off a primitive unsigned integer type of *any* size from the end of a non-empty bit-vector.
    ///
    /// For example, if `v` is a `BitVec<u8>`with 22 elements, then you can split off a `u16` value from the end of `v`
    /// by calling `v.split_off_unsigned<u16>()`. This leaves the bit-vector with 6 elements.
    ///
    /// # Examples
    /// ```
    /// auto v = BitVec<u8>::ones(22);
    /// auto x16 = v.split_off_unsigned<u16>();
    /// assert_eq(*x16, 0b1111'1111'1111'1111);
    /// assert_eq(v.size(), 6);
    /// assert_eq(v.to_string(), "111111");
    /// auto w = BitVec<u8>::alternating(24);
    /// auto x8 = w.split_off_unsigned<u8>();
    /// assert_eq(*x8, 0b0101'0101);
    /// assert_eq(w.size(), 16);
    /// assert_eq(w.to_string(), "1010101010101010");
    /// w.append(*x8);
    /// assert_eq(w.to_string(), "101010101010101010101010");
    /// ```
    template<Unsigned Dst = Word>
    constexpr std::optional<Dst> split_off_unsigned() {
        // Edge case ...
        if (size() == 0) return std::nullopt;

        // Perhaps we can safely cast an `Word` into a `Dst` without any loss of information.
        constexpr usize bits_per_dst = BITS<Dst>;
        if constexpr (bits_per_dst <= bits_per_word) {
            // Easy cases when we can safely cast an `Word` into a `Dst` without any loss of information.
            // 1. Just one word in the store & we know that all unused bits in that word are zeros.
            // 2. More than one word in the store but the last word is fully occupied.
            auto n_words = words();
            auto shift = size() % bits_per_word;
            if (n_words == 1 || shift == 0) {
                auto result = static_cast<Dst>(m_store[n_words - 1]);
                resize(size() - bits_per_dst);
                return result;
            }

            // The last word is not fully occupied so we need to combine it with the next to last word to form a
            // value that can be cast to a `Dst`.
            auto lo = m_store[n_words - 2] >> shift;
            auto hi = m_store[n_words - 1] << (bits_per_word - shift);
            auto result = static_cast<Dst>(lo | hi);
            resize(size() - bits_per_dst);
            return result;
        } else {
            // `Dst` is larger than `Word` -- it should be an integer multiple of `bits_per_word` is size.
            static_assert(bits_per_dst % bits_per_word == 0, "sizeof(Dst) % sizeof(Word) != 0");

            // Pop an appropriate number of `Word` sized chunks off the end of the bit-vector.
            auto              words_per_dst = bits_per_dst / bits_per_word;
            std::vector<Word> chunks(words_per_dst);
            for (auto i = 0uz; i < words_per_dst; ++i) chunks[i] = *split_off_unsigned<Word>();

            // Combine the chunks into a `Dst` value.
            Dst result = 0;
            for (auto i = words_per_dst; i-- > 0;) result |= static_cast<Dst>(chunks[i] << (i * bits_per_word));

            return result;
        }
    }

    /// @}
    /// @name Bit Accessors:
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
    constexpr bool get(usize i) const { return gf2::get(*this, i); }

    /// Returns the boolean value of the bit element `i`.
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
    constexpr bool operator[](usize i) const { return gf2::get(*this, i); }

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
    constexpr bool front() const { return gf2::front(*this); }

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
    /// BitVec v{10};
    /// assert_eq(v.get(0), false);
    /// v.set(0);
    /// assert_eq(v.get(0), true);
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
    /// BitVec v{10};
    /// v[2] = true;
    /// assert(v.to_string() == "0010000000");
    /// auto w = BitVec<>::ones(10);
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
    /// auto v = BitVec<u8>::ones(10);
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
    constexpr auto swap(usize i0, usize i1) { return gf2::swap(*this, i0, i1); }

    /// @}
    /// @name Store Queries:
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
    constexpr bool is_empty() const { return gf2::is_empty(*this); }

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
    constexpr bool any() const { return gf2::any(*this); }

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
    constexpr bool all() const { return gf2::all(*this); }

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
    /// auto v = BitVec<u8>::zeros(10);
    /// v.set_all();
    /// assert_eq(v.to_string(), "1111111111");
    /// ```
    auto set_all(bool value = true) { return gf2::set_all(*this, value); }

    /// Flips the value of the bits in the store and returns a reference to this for chaining.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::zeros(10);
    /// v.flip_all();
    /// assert_eq(v.to_string(), "1111111111");
    /// ```
    auto flip_all() { return gf2::flip_all(*this); }

    /// @}
    /// @name Copying into the BitVec:
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
    template<Unsigned Src>
    auto copy(Src src) {
        return gf2::copy(src, *this);
    }

    /// Copies the bits from an equal-sized `src` store and returns a reference to this for chaining.
    ///
    /// @note This is one of the few methods in the library that *doesn't* require the two stores to have the same
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
    /// BitVec v{10};
    /// v.copy(src);
    /// assert_eq(v.to_string(), "0101010101");
    /// ```
    template<usize N>
    auto copy(std::bitset<N> const& src) {
        return gf2::copy(src, *this);
    }

    /// @}
    /// @name Store Fills:
    /// @{

    /// Fills the store by repeatedly calling `f(i)` and returns a reference to this for chaining.
    ///
    /// # Example
    /// ```
    /// BitVec v{10};
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
    /// BitVec u{10}, v{10};
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
    /// BitVec v{10};
    /// assert_eq(v.count_ones(), 0);
    /// v.set(0);
    /// assert_eq(v.count_ones(), 1);
    /// ```
    constexpr usize count_ones() const { return gf2::count_ones(*this); }

    /// Returns the number of unset bits in the store.
    ///
    /// # Example
    /// ```
    /// BitVec v{10};
    /// assert_eq(v.count_zeros(), 10);
    /// v.set(0);
    /// assert_eq(v.count_zeros(), 9);
    /// ```
    constexpr usize count_zeros() const { return gf2::count_zeros(*this); }

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
    constexpr usize leading_zeros() const { return gf2::leading_zeros(*this); }

    /// Returns the number of trailing zeros in the store.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::zeros(27);
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
    constexpr std::optional<usize> first_set() const { return gf2::first_set(*this); }

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
    constexpr std::optional<usize> last_set() const { return gf2::last_set(*this); }

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
    constexpr std::optional<usize> next_set(usize index) const { return gf2::next_set(*this, index); }

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
    constexpr std::optional<usize> previous_set(usize index) const { return gf2::previous_set(*this, index); }

    /// @}
    /// @name Unset-bit Indices:
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
    constexpr std::optional<usize> first_unset() const { return gf2::first_unset(*this); }

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
    constexpr std::optional<usize> last_unset() const { return gf2::last_unset(*this); }

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
    constexpr std::optional<usize> next_unset(usize index) const { return gf2::next_unset(*this, index); }

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
    /// auto u = BitVec<u8>::ones(10);
    /// for (auto&& bit : u.bits()) assert_eq(bit, true);
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
    /// auto v = BitVec<u8>::zeros(10);
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
    /// auto v = BitVec<u8>::alternating(10);
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
    /// auto v = BitVec<u8>::alternating(10);
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
    /// synthesise "words" on the fly from adjacent pairs of bit-vector words. Nevertheless, almost all the methods
    /// in `BitStore` are implemented efficiently by operating on those words.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(10);
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
    /// auto v = BitVec<u8>::ones(10);
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
    /// auto v = BitVec<>::alternating(10);
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
    /// auto v = BitVec<>::alternating(10);
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

    /// Returns a *clone* of the elements in the half-open range `[begin, end)` as a new bit-vector.
    ///
    /// @note This method panics if the range is not valid.
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
    constexpr auto sub(usize begin, usize end) const { return gf2::sub(*this, begin, end); }

    /// @}
    /// @name Splits:
    /// @{

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
        return gf2::split(*this, at, left, right);
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
    constexpr auto split_at(usize at) const { return gf2::split(*this, at); }

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
    /// v.riffled(dst);
    /// assert_eq(dst.to_string(), "1010101010101010101");
    /// ```
    constexpr void riffled(BitVec<word_type>& dst) const { return gf2::riffle(*this, dst); }

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
    /// BitVec v{10};
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
    /// BitVec v{10};
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
    /// auto v = BitVec<>::alternating(10);
    /// assert_eq(v.to_pretty_string(), "[1,0,1,0,1,0,1,0,1,0]");
    /// BitVec empty;
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
    std::string to_hex_string() const { return gf2::to_hex_string(*this); }

    /// Returns a multi-line string describing the bit-store in some detail.
    ///
    /// This method is useful for debugging but you should not rely on the output format which may change.
    std::string describe() const { return gf2::describe(*this); }

    /// @}
};

} // namespace gf2
