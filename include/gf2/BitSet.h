#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// Fixed-size vectors over GF(2) with compactly stored bit-elements in a standard array of primitive unsigned words.
/// <br> See the [BitSet](docs/pages/BitSet.md) page for more details.

#include <gf2/BitStore.h>
#include <gf2/BitSpan.h>

namespace gf2 {

/// A fixed-size vector over GF(2) with `N` bit elements compactly stored in a standard vector of primitive unsigned
/// words whose type is given by the template parameter `Word`. The elements in a bitset are initially all set to 0.
///
/// `BitSet<N>` is similar to `std::bitset<N>` but has a different interface and a richer set of functionality.
///
/// The `BitSet` class inherits from the base `gf2::BitStore` class. We implement a small number of required methods
/// and then inherit the majority of our functionality from that base.
///
/// @note Inheritance is done using the CRTP to avoid all runtime virtual method dispatch overhad..
template<usize N, unsigned_word Word = usize>
class BitSet : public BitStore<BitSet<N, Word>> {
private:
    // The bit elements are packed compactly into this standard array of unsigned words
    std::array<Word, words_needed<Word>(N)> m_data{};

public:
    /// The underlying unsigned word type used to store the bits.
    using word_type = Word;

    /// The number of bits per `Word`.
    static constexpr u8 bits_per_word = BITS<Word>;

    /// @name BitStore Required Methods.
    /// @{

    /// Returns the number of bit-elements in the bitset.
    ///
    /// # Example
    /// ```
    /// BitSet<10> v;
    /// assert_eq(v.size(), 10);
    /// ```
    constexpr usize size() const { return N; }

    /// Returns the number of words in the bitset's underlying word store.
    ///
    /// The bit-elements are packed into a standard array with this number of words.
    ///
    /// # Example
    /// ```
    /// BitSet<10, u8> v0;
    /// assert_eq(v0.words(), 2);
    /// BitSet<10, u16> v1;
    /// assert_eq(v1.words(), 1);
    /// ```
    constexpr usize words() const { return m_data.size(); }

    /// Returns word `i` from the bitset's underlying word store.
    ///
    /// The final word in the store may not be fully occupied but we guarantee that unused bits are set to 0.
    ///
    /// @note In debug mode the index is bounds checked.
    ///
    /// # Example
    /// ```
    /// BitSet<10, u8> v;
    /// assert_eq(v.to_string(), "0000000000");
    /// v.set_all();
    /// assert_eq(v.to_string(), "1111111111");
    /// assert_eq(v.words(), 2);
    /// assert_eq(v.word(0), 0b1111'1111);
    /// assert_eq(v.word(1), 0b0000'0011);
    /// ```
    constexpr Word word(usize i) const {
        gf2_debug_assert(i < words(), "Index {} is too large for a bitset with {} words.", i, words());
        return m_data[i];
    }

    /// Sets word `i` in the bitset's underlying word store to `value` (masked if necessary).
    ///
    /// The final word in the store may not be fully occupied but we ensure that unused bits remain set to 0.
    ///
    /// @note In debug mode the index is bounds checked.
    ///
    /// # Example
    /// ```
    /// BitSet<10, u8> v;
    /// assert_eq(v.to_string(), "0000000000");
    /// v.set_word(1, 0b1111'1111);
    /// assert_eq(v.to_string(), "0000000011");
    /// assert_eq(v.count_ones(), 2);
    /// ```
    constexpr void set_word(usize i, Word word) {
        gf2_debug_assert(i < words(), "Index {} is too large for a bitset with {} words.", i, words());

        // Set the value and if it's the last word, make sure it is kept clean
        m_data[i] = word;
        if (i == words() - 1) clean();
    }

    /// Returns a const pointer to the underlying store of words .
    ///
    /// # Example
    /// ```
    /// BitSet<10, u8> v;
    /// v.set_all();
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
    /// BitSet<10, u8> v;
    /// v.set_all();
    /// auto ptr = v.data();
    /// assert_eq(*ptr, 0b1111'1111);
    /// ```
    constexpr Word* data() { return m_data.data(); }

    /// Returns the offset (in bits) of the first bit in the store within the first word.
    ///
    /// This is always zero for `BitSet`.
    constexpr u8 offset() const { return 0; }

    /// @}
    /// @name Helper Methods:
    /// @{

    /// Sets any unused bits in the *last* occupied word to 0.
    ///
    /// This is used to enforce the guarantee that unused bits in the store are always set to 0.
    constexpr void clean() {
        // NOTE: This works fine even if `size() == 0`.
        auto shift = N % bits_per_word;
        if (shift != 0) m_data[words() - 1] &= Word(~(MAX<Word> << shift));
    }
    /// @}
};

} // namespace gf2

// --------------------------------------------------------------------------------------------------------------------
// Specialises `std::formatter` for `BitSet` -- defers to the version for `BitStore` ...
// -------------------------------------------------------------------------------------------------------------------
template<gf2::usize N, gf2::unsigned_word Word>
struct std::formatter<gf2::BitSet<N, Word>> : std::formatter<gf2::BitStore<gf2::BitSet<N, Word>>> {
    // Inherits all functionality from `BitStore` formatter
};
