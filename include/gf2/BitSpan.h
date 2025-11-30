#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// Non-owning *views* of a contiguous range of bits from some underlying store of unsigned words. <br>
/// See the [BitSpan](docs/pages/BitSpan.md) page for more details.

#include <gf2/assert.h>
#include <gf2/BitStore.h>

#include <print>

namespace gf2 {

/// A *bit_span* is a non-owning view of contiguous bits stored in some underlying store of unsigned `Word`s. <br>
/// It inherits from `gf2::BitStore`, which provides a rich API for manipulating the bits in the span.
///
/// A bit-span holds a pointer to some underlying contiguous span of unsigned words that "own" the bits in question.
/// They are created by calling the `span` method on any `gf2::BitStore` subclass (e.g., `BitVec`). <br>
/// That store *must* exist for as long as the `BitSpan` exists.
///
/// Bit-spans created from const bit-stores will have a type like `BitSpan<const u64>, while bit-spans from
/// non-const bit-stores will have a type like `BitSpan<u64>`. The existence or absence of that `const` qualifier in
/// the template paramater is important as it allows us to maintain const-correctness. The first type is read-only, the
/// second is read-write.
///
/// What matters is the *interior* const-ness of the span. In fact, you can generally pass a bit-span by value and not
/// worry about passing a reference or const reference -- the type is small enough to pass on the stack.
/// This is similar in spirit to the `std::span<T>` template class.
template<unsigned_word Word>
class BitSpan : public BitStore<BitSpan<Word>> {
private:
    Word* m_begin;  // A pointer to the first word containing bits in the span (it may be partially occupied).
    u8    m_offset; // The bit-span begins at this bit-offset inside `m_begin[0]`.
    usize m_size;   // The number of bits in the span..
    usize m_words;  // We cache the total number of synthesized words the span generates.

public:
    /// A constant -- the number of bits in a `Word`.
    static constexpr usize bits_per_word = std::numeric_limits<Word>::digits;

    // The underlying store really has words of this type (where we are remove any `const` qualifier).
    using word_type = std::remove_const_t<Word>;

    /// @name Constructors
    /// @{

    /// Constructs a non-owning bit-span from a pointer to a contiguous span of unsigneds etc.
    ///
    /// @param begin  A pointer to the first word in the underlying store that holds the bits for the span.
    /// @param offset The bit-offset within that first word where the span begins.
    /// @param size   The number of bits in the span.
    ///
    /// A `BitSpan` is *non-owning* view into a contiguous span of `size` bits which are assumed to be stored in
    /// contiguous words starting at the `offset` bit of the `*begin` word. We do not allow empty bit-spans.
    ///
    /// Generally bit-spans are constructed using the `span` methods in the `BitStore` class.
    ///
    /// @note It is the responsibility of the caller to ensure that the underlying store of unsigneds continues to
    /// exist for as long as the `BitSpan` exists.
    ///
    /// # Example
    /// ```
    /// std::vector<u8> words{0b1010'1010, 0b1100'1100};
    /// BitSpan s{words.data(), 0, 16};
    /// assert_eq(s.to_string(), "0101010100110011");
    /// ```
    BitSpan(Word* begin, u8 offset, usize size) {

        // Check that the span is non-empty and that the begin bit is valid.
        gf2_debug_assert(size > 0, "We do not allow empty bit-spans.");
        gf2_debug_assert(offset < bits_per_word, "Span begins at bit-offset {} > bits in a word ({})!", offset,
                         bits_per_word);

        // Set the member variables.
        m_begin = begin;
        m_offset = offset;
        m_size = size;

        // Cache the total number of synthesized words needed to store all the bits in the span.
        m_words = gf2::words_needed<Word>(m_size);
    }

    /// @}
    /// @name BitStore Required Methods.
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

    /// Returns the *minimum* number of words needed to store the bits in the span.
    ///
    /// These spans are views into a contiguous range of bits  from some underlying store of unsigned words.
    /// Generally a bit-span is not aligned with the word boundaries of the store. However, the bit-span can synthesise
    /// words *as if* it copied the bits and shifted them down so that element 0 is at bit-position zero in synthetic
    /// word number 0. This function returns the number of such words.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::ones(18);
    /// assert_eq(v.words(), 3);
    /// auto s = v.span(4, 12);             // The span covers words 0 and 1 in the store underlying v.
    /// assert_eq(s.words(), 1);   // However, it can be fitted into a single synthetic word!
    /// ```
    constexpr usize words() const { return m_words; }

    /// Returns a "word"'s worth of bits from the bit-span.
    ///
    /// These spans are views into a contiguous range of bits  from some underlying store of unsigned words.
    /// Generally a bit-span is not aligned with the word boundaries of the store. However, the bit-span can synthesise
    /// words *as if* it copied the bits and shifted them down so that element 0 is at bit-position zero in synthetic
    /// word number 0. This function returns those words by combining bits from pairs of adjacent words in the true
    /// underlying store.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::from_string("0000'0000'1111'1111").value();
    /// auto s = v.span(4, 12);                 // The span covers words 0 and 1 in the store underlying v.
    /// assert_eq(s.words(), 1);       // However, it can be fitted into a single synthetic word!
    /// assert_eq(s.word(0), 0b1111'0000);
    /// ```
    constexpr word_type word(usize i) const {
        // Get the recipe for the "word" at index `i` (we may need two real words to synthesise it).
        auto [w0_bits, w1_bits] = recipe_for_word(i);

        // Our return value starts with all unset bits.
        word_type result = 0;

        // Replace `w0_bits` low-order bits in `result` with the same number of high-order bits from `w0`
        Word w0 = m_begin[i];
        gf2::replace_bits(result, 0, w0_bits, w0 >> m_offset);

        // Perhaps our result spans two words.
        if (w1_bits > 0) {
            // Replace `w1_bits` high-order bits in result with the same number of low-order bits from `w1`
            Word w1 = m_begin[i + 1];
            gf2::replace_bits(result, w0_bits, w0_bits + w1_bits, w1 << w0_bits);
        }
        return result;
    }

    /// Sets a "word"'s worth of bits in the bit-span.
    ///
    /// These spans are views into a contiguous range of bits  from some underlying store of unsigned words.
    /// Generally a bit-span is not aligned with the word boundaries of the store. However, the bit-span can synthesise
    /// words *as if* it copied the bits and shifted them down so that element 0 is at bit-position zero in synthetic
    /// word number 0. This function sets those words by altering bits in pairs of adjacent words in the true
    /// underlying store.
    ///
    /// # Example
    /// ```
    /// auto v = BitVec<u8>::from_string("0000_0000_1111_1111").value();
    /// auto s = v.span(4, 12);                 // The span covers words 0 and 1 in the store underlying v.
    /// assert_eq(s.words(), 1);       // However, it can be fitted into a single synthetic word!
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
            gf2::replace_bits(m_begin[i], m_offset, m_offset + w0_bits, value << m_offset);

            // Perhaps we also need to overwrite some bits in the second word.
            if (w1_bits > 0) {
                // Replace `w1_bits` low-order bits in `w1` with the same number of high-order bits from `value`
                gf2::replace_bits(m_begin[i + 1], 0, w1_bits, value >> w0_bits);
            }
        }
    }

    /// Returns a pointer to the first span word in some underlying store of words (const version).
    constexpr const Word* data() const { return m_begin; }

    /// Returns a pointer to the first span word in some underlying store of words (const version).
    constexpr Word* data() { return m_begin; }

    /// Returns the offset (in bits) of the first bit in the span within the first span word.
    constexpr u8 offset() const { return m_offset; }

    /// @}

private:
    /// A helper method to return the recipe used to "bake" the (generally synthetic) `word(i)`.
    ///
    /// These spans may not be aligned with the underlying unsigneds so we need to synthesise "words" as needed.
    /// To synthesise word `i`, we use two contiguous words from the underlying vector of words, `w0`, and `w1`,
    /// where `w0` is always m_words[i] and `w1` is always m_words[i + 1].
    ///
    /// We copy `w0_bits` high-order bits from `w0` and they become the low-order bits in the span "word": `word(i)`.
    /// We copy `w1_bits` low-order bits from `w1` and they become the high-order bits in the span "word": `word(i)`.
    ///
    /// This "recipe" method returns the pair: `(w0_bits, w1_bits)` which tells you the numbers of needed bits.
    ///
    /// The only tricky part is that the last span word may not be fully occupied so we need to handle it differently
    /// from the others.
    ///
    /// @note In debug mode we check that the index `i` is valid.
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

// --------------------------------------------------------------------------------------------------------------------
// Specialises `std::formatter` for `BitSpan` -- defers to the version for `BitStore` ...
// -------------------------------------------------------------------------------------------------------------------
template<gf2::unsigned_word Word>
struct std::formatter<gf2::BitSpan<Word>> : std::formatter<gf2::BitStore<gf2::BitSpan<Word>>> {
    // Inherits all functionality from `BitStore` formatter
};