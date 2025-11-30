#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// Vectors over GF(2) with compactly stored bit-elements in a standard vector of primitive unsigned words. <br>
/// See the [BitVec](docs/pages/BitVec.md) page for more details.

#include <gf2/BitStore.h>
#include <gf2/BitSpan.h>

#include <algorithm>
#include <array>
#include <bitset>
#include <charconv>
#include <cmath>
#include <concepts>
#include <cctype>
#include <cstdint>
#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace gf2 {

/// A dynamically-sized vector over GF(2) with bit elements compactly stored in a standard vector of primitive unsigned
/// words whose type is given by the template parameter `Word`.
///
/// The `BitVec` class inherits from the base `gf2::BitStore` class. We implement a small number of required methods
/// and then inherit the majority of our functionality from that base.
///
/// @note Inheritance is done using the CRTP to avoid all runtime virtual method dispatch overhad..
template<unsigned_word Word = usize>
class BitVec : public BitStore<BitVec<Word>> {
private:
    // The number of bit elements in the bit-vector.
    usize m_size;

    // The bit elements are packed compactly into this standard vector of unsigned words
    std::vector<Word> m_data;

public:
    /// The underlying unsigned word type used to store the bits.
    using word_type = Word;

    /// The number of bits per `Word`.
    static constexpr u8 bits_per_word = BITS<Word>;

    /// @name BitStore Required Methods.
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
    constexpr usize words() const { return m_data.size(); }

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
        return m_data[i];
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
        m_data[i] = word;
        if (i == words() - 1) clean();
    }

    /// Returns a const pointer to the underlying store of words .
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(10);
    /// auto ptr = v.data();
    /// assert_eq(*ptr, 0b1111'1111);
    /// ```
    constexpr const Word* data() const { return m_data.data(); }

    /// Returns a pointer to the underlying store of words.
    ///
    /// @note The pointer is non-const but you should be careful about using it to modify the words in the store.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(10);
    /// auto ptr = v.data();
    /// assert_eq(*ptr, 0b1111'1111);
    /// ```
    constexpr Word* data() { return m_data.data(); }

    /// Returns the offset (in bits) of the first bit in the store within the first word.
    ///
    /// This is always zero for `BitVec`.
    constexpr u8 offset() const { return 0; }

    /// @}
    /// @name Constructors
    /// @{

    /// Constructs a bit-vector of length `n` with all the bit elements set to 0.
    ///
    /// The bit elements are compactly packed into a a standard vector of unsigned words.
    /// The template parameter `Word` sets the type of those words.
    /// It has a reasonable default type that works for most cases.
    ///
    /// @note The default constructor returns the empty bit-vector with no elements.
    ///
    /// # Example
    /// ```
    /// BitVec v0;
    /// assert_eq(v0.to_string(), "");
    /// BitVec<u8> v1{10};
    /// assert_eq(v1.to_string(), "0000000000");
    /// ```
    explicit constexpr BitVec(usize len = 0) : m_size(len), m_data(gf2::words_needed<Word>(len)) {
        // Empty body -- we now have an underlying vector of words all initialized to 0.
        // Note how we avoided using uniform initialization on the `std::vector` data member.
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
    explicit constexpr BitVec(usize len, Word word) : m_size(len), m_data(gf2::words_needed<Word>(len), word) {
        // Make sure any excess bits are set to 0.
        clean();
    }

    /// @}
    /// @name Factory Constructors
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
        result.m_data.reserve(gf2::words_needed<Word>(capacity));
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

    /// Factory method to construct a bit-vector by copying *all* the bits from *any* `BitStore` instance.
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
    template<typename Src>
    static constexpr BitVec from(const BitStore<Src>& src) {
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
    static constexpr BitVec from(const std::bitset<N>& src) {
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
        result.fill(f);
        return result;
    }

    /// @}
    /// @name Random Bit-Vector Constructors
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
        result.random_fill(p, seed);
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
    /// @name Constructors from Strings
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
    /// auto v = BitVec<u8>::from_binary_string("0b1010'1010'10");
    /// assert_eq(v->to_string(), "1010101010");
    /// auto u = BitVec<u8>::from_binary_string("");
    /// assert_eq(u->to_string(), "");
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
    /// @name Size and Capacity
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
    constexpr usize capacity() const { return bits_per_word * m_data.capacity(); }

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
        m_data.resize(gf2::words_needed<Word>(size()));
        m_data.shrink_to_fit();
        return *this;
    }

    /// Removes all elements from the bit-vector so @c size()==0.
    ///
    /// The capacity is not changed by this operation.
    constexpr BitVec& clear() {
        m_data.clear();
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
            m_data.resize(gf2::words_needed<Word>(n), 0);
            auto old_size = size();
            m_size = n;

            // If we truncated, we need to clean up the last occupied word if necessary.
            if (n < old_size) clean();
        }
        return *this;
    }

    /// Sets any unused bits in the *last* occupied word to 0.
    ///
    /// This is used to enforce the guarantee that unused bits in the store are always set to 0.
    constexpr void clean() {
        // NOTE: This works fine even if `size() == 0`.
        auto shift = m_size % bits_per_word;
        if (shift != 0) m_data[words() - 1] &= Word(~(MAX<Word> << shift));
    }

    /// @}
    /// @name Appending BitsIter
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
        this->set(size() - 1, b);
        return *this;
    }

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
    template<unsigned_word Src>
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
    template<typename Src>
    constexpr BitVec& append(const BitStore<Src>& src) {
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
    auto append(const std::bitset<N>& src) {
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
    /// @name Removing BitsIter
    /// @{

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
        auto b = this->get(m_size - 1);
        resize(m_size - 1);
        return b;
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
    template<unsigned_word Dst = Word>
    constexpr std::optional<Dst> split_off_unsigned() {
        // Edge case ...
        if (this->size() == 0) return std::nullopt;

        // Perhaps we can safely cast an `Word` into a `Dst` without any loss of information.
        constexpr usize bits_per_dst = BITS<Dst>;
        if constexpr (bits_per_dst <= bits_per_word) {
            // Easy cases when we can safely cast an `Word` into a `Dst` without any loss of information.
            // 1. Just one word in the store & we know that all unused bits in that word are zeros.
            // 2. More than one word in the store but the last word is fully occupied.
            auto n_words = this->words();
            auto shift = this->size() % bits_per_word;
            if (n_words == 1 || shift == 0) {
                auto result = static_cast<Dst>(m_data[n_words - 1]);
                resize(this->size() - bits_per_dst);
                return result;
            }

            // The last word is not fully occupied so we need to combine it with the next to last word to form a
            // value that can be cast to a `Dst`.
            auto lo = m_data[n_words - 2] >> shift;
            auto hi = m_data[n_words - 1] << (bits_per_word - shift);
            auto result = static_cast<Dst>(lo | hi);
            resize(this->size() - bits_per_dst);
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
};

} // namespace gf2

// --------------------------------------------------------------------------------------------------------------------
// Specialises `std::formatter` for `BitVec` -- defers to the version for `BitStore` ...
// -------------------------------------------------------------------------------------------------------------------
template<gf2::unsigned_word Word>
struct std::formatter<gf2::BitVec<Word>> : std::formatter<gf2::BitStore<gf2::BitVec<Word>>> {
    // Inherits all functionality from `BitStore` formatter
};
