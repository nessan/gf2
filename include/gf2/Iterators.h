#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// Five bit-store iterators that serve different purposes. <br>
/// See the [Iterators](docs/pages/Iterators.md) page for more details.

#include <gf2/BitStore.h>
#include <gf2/BitRef.h>

namespace gf2 {

// --------------------------------------------------------------------------------------------------------------------
// `BitsIter` ...
// --------------------------------------------------------------------------------------------------------------------

/// Two iterators over *all* the bits in a bit-store --- one const and the other non-const. <br>
/// The `BitStore::bits()` & `BitStore::bits() const` methods return the appropriate iterator type.
///
/// - `BitsIter<Store, Word, false>`: A non-const iterator over the bits in a bit-store (value type is `BitRef`).
/// - `BitsIter<Store, Word, true>`:  A const iterator over the bits in a bit-store (value type is `bool`).
///
/// Both iterators allow us to view the bit values but only the non-const iterator allows us to set the bit values.
///
/// The two iterator types have lots of common functionality. To avoid code duplication we add a final boolean template
/// parameter `is_const` to distinguish the two types. Depending on the value of `is_const`, the `std::conditional_t`
/// is used to choose the correct type for the store pointer and value type.
///
/// # Example
/// ```
/// BitVec v{10};
/// assert_eq(v.to_string(), "0000000000");
/// for (auto&& bit : v.bits()) bit = true;
/// assert_eq(v.to_string(), "1111111111");
/// ```
template<BitStore Store, bool is_const>
class BitsIter {
public:
    // We keep a pointer to the parent bit-store which is either const or non-const.
    using store_pointer = std::conditional_t<is_const, const Store*, Store*>;

    // The value type is either a `bool` or a `BitRef` depending on the constness of the iterator.
    using value_type = std::conditional_t<is_const, bool, BitRef<Store>>;

    // The difference type is always a `std::ptrdiff_t` (required by the iterator concept).
    using difference_type = std::ptrdiff_t;

private:
    store_pointer m_store = nullptr;
    usize         m_index = 0;

public:
    // Our constructor captures a (const or non-const) pointer to the store and the index for the bit of interest.
    BitsIter(store_pointer store, usize idx) : m_store(store), m_index{idx} {}

    // Iterator are required to act like value types so must be default constructible, moveable, and copyable.
    BitsIter() = default;
    BitsIter(BitsIter&&) = default;
    BitsIter(BitsIter const&) = default;
    BitsIter& operator=(BitsIter&&) = default;
    BitsIter& operator=(BitsIter const&) = default;

    // Iterators must be comparable (this is needed so that any range-for loop will work).
    constexpr bool operator==(BitsIter const& other) const {
        return m_store == other.m_store && m_index == other.m_index;
    }

    // Returns the item that the iterator is currently pointing to.
    // The `decltype(auto)` will either be a `bool` or a `BitRef` depending on the constness of the pointer.
    constexpr decltype(auto) operator*() const { return m_store->operator[](m_index); }

    // Any call to `begin()` returns a new iterator pointing to the first bit in the store.
    constexpr auto begin() const { return BitsIter(m_store, 0); }

    // Any call to `end()` returns a new iterator pointing to the bit after the last bit in the store.
    constexpr auto end() const { return BitsIter(m_store, m_store->size()); }

    // Increment the iterator, returning the new value (the `i++` operator).
    constexpr BitsIter& operator++() {
        ++m_index;
        return *this;
    }

    // Increment the iterator, returning the previous value (the `++i` operator).
    constexpr BitsIter operator++(int) {
        auto prev = *this;
        ++*this;
        return prev;
    }

    // Decrement the iterator, returning the new value (the `--i` operator).
    constexpr BitsIter& operator--() {
        --m_index;
        return *this;
    }

    // Decrement the iterator, returning the previous value (the `--i` operator).
    constexpr BitsIter operator--(int) {
        auto prev = *this;
        --*this;
        return prev;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// `SetBitsIter` ...
// --------------------------------------------------------------------------------------------------------------------

/// An iterator over the _index locations_ of the set bits in a bit-store. <br>
/// You get this iterator by calling the `BitStore::set_bits()` method.
///
/// This is a constant iterator that returns the indices of the set bits in the store as successive `usize`s.
///
/// # Example
/// ```
/// auto v = BitVec<u8>::alternating(10);
/// assert_eq(v.to_string(), "1010101010");
/// auto indices = std::ranges::to<std::vector>(v.set_bits());
/// assert_eq(indices, (std::vector<usize>{0, 2, 4, 6, 8}));
/// ```
template<BitStore Store>
class SetBitsIter {
private:
    const Store*         m_store = nullptr;      // The store we're iterating over
    std::optional<usize> m_index = std::nullopt; // Search after this index.

public:
    // The value type is a `usize` (the index of the set bit).
    using value_type = usize;

    // The difference type is always a `std::ptrdiff_t` (required by the iterator concept).
    using difference_type = std::ptrdiff_t;

    // Our constructor captures a pointer to the store and sets the index to uninitialized.
    SetBitsIter(Store const* store, std::optional<usize> idx = std::nullopt) : m_store(store), m_index{idx} {
        // Empty body ...
    }

    // Iterator are required to act like value types so must be default constructible, moveable, and copyable.
    SetBitsIter() = default;
    SetBitsIter(SetBitsIter&&) = default;
    SetBitsIter(SetBitsIter const&) = default;
    SetBitsIter& operator=(SetBitsIter&&) = default;
    SetBitsIter& operator=(SetBitsIter const&) = default;

    // Iterators must be comparable (this is how the range-for loops work).
    constexpr bool operator==(SetBitsIter const& other) const {
        return m_store == other.m_store && m_index == other.m_index;
    }

    // What is the iterator currently pointing to? (this is what the range-for loop will use).
    constexpr value_type operator*() const { return *m_index; }

    // Any call to `begin` returns a new iterator pointing to the first set bit in the store.
    constexpr auto begin() const { return SetBitsIter(m_store, m_store->first_set()); }

    // Any call to `end` returns a new iterator that is not initialized.
    constexpr auto end() const { return SetBitsIter(m_store, std::nullopt); }

    // Increment the iterator, returning the new value (the `i++` operator).
    constexpr SetBitsIter& operator++() {
        if (m_index.has_value()) m_index = m_store->next_set(*m_index);
        return *this;
    }

    // Increment the iterator, returning the previous value (the `++i` operator).
    constexpr SetBitsIter operator++(int) {
        auto prev = *this;
        ++*this;
        return prev;
    }

    // Decrement the iterator, returning the new value (the `--i` operator).
    constexpr SetBitsIter& operator--() {
        if (m_index.has_value()) m_index = previous_set(*m_store, *m_index);
        return *this;
    }

    // Decrement the iterator, returning the previous value (the `--i` operator).
    constexpr SetBitsIter operator--(int) {
        auto prev = *this;
        --*this;
        return prev;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// `UnsetBitsIter` ...
// --------------------------------------------------------------------------------------------------------------------

/// An iterator over the _index locations_ of the unset bits in a bit-store.<br>
/// You get this iterator by calling the `BitStore::unset_bits()` method.
///
/// This is a constant iterator that returns the indices of the unset bits in the store as successive `usize`s.
///
/// # Example
/// ```
/// auto v = BitVec<u8>::alternating(10);
/// assert_eq(v.to_string(), "1010101010");
/// auto indices = std::ranges::to<std::vector>(v.unset_bits());
/// assert_eq(indices, (std::vector<usize>{1, 3, 5, 7, 9}));
/// ```
template<BitStore Store>
class UnsetBitsIter {
private:
    const Store*         m_store = nullptr;      // The store we're iterating over
    std::optional<usize> m_index = std::nullopt; // Search after this index.

public:
    // The value type is a `usize` (the index of the unset bit).
    using value_type = usize;

    // The difference type is always a `std::ptrdiff_t` (required by the iterator concept).
    using difference_type = std::ptrdiff_t;

    // Our constructor captures a pointer to the store and sets the index to uninitialized.
    UnsetBitsIter(Store const* store, std::optional<usize> idx = std::nullopt) : m_store(store), m_index{idx} {
        // Empty body ...
    }

    // Iterator are required to act like value types so must be default constructible, moveable, and copyable.
    UnsetBitsIter() = default;
    UnsetBitsIter(UnsetBitsIter&&) = default;
    UnsetBitsIter(UnsetBitsIter const&) = default;
    UnsetBitsIter& operator=(UnsetBitsIter&&) = default;
    UnsetBitsIter& operator=(UnsetBitsIter const&) = default;

    // Iterators must be comparable (this is how the range-for loops work).
    constexpr bool operator==(UnsetBitsIter const& other) const {
        return m_store == other.m_store && m_index == other.m_index;
    }

    // What is the iterator currently pointing to? (this is what the range-for loop will use).
    constexpr value_type operator*() const { return *m_index; }

    // Any call to `begin` returns a new iterator pointing to the first unset bit in the store.
    constexpr auto begin() const { return UnsetBitsIter(m_store, m_store->first_unset()); }

    // Any call to `end` returns a new iterator that is not initialized.
    constexpr auto end() const { return UnsetBitsIter(m_store, std::nullopt); }

    // Increment the iterator, returning the new value (the `i++` operator).
    constexpr UnsetBitsIter& operator++() {
        if (m_index.has_value()) m_index = m_store->next_unset(*m_index);
        return *this;
    }

    // Increment the iterator, returning the previous value (the `++i` operator).
    constexpr UnsetBitsIter operator++(int) {
        auto prev = *this;
        ++*this;
        return prev;
    }

    // Decrement the iterator, returning the new value (the `--i` operator).
    constexpr UnsetBitsIter& operator--() {
        if (m_index.has_value()) m_index = previous_unset(*m_store, *m_index);
        return *this;
    }

    // Decrement the iterator, returning the previous value (the `--i` operator).
    constexpr UnsetBitsIter operator--(int) {
        auto prev = *this;
        --*this;
        return prev;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// `WordsIter` ...
// --------------------------------------------------------------------------------------------------------------------

/// An iterator over the "words" underlying a bit-store.<br>
/// You get this iterator by calling the `BitStore::store_words()` method.
///
/// This is a constant iterator that returns the "words" underlying a bit-store as successive `Word`s.
///
/// # Example
/// ```
/// auto v = BitVec<u8>::ones(10);
/// assert_eq(v.to_string(), "1111111111");
/// auto words = std::ranges::to<std::vector>(v.store_words());
/// assert_eq(words, (std::vector<std::uint8_t>{0b1111'1111, 0b0000'0011}));
/// ```
template<BitStore Store>
class WordsIter {
private:
    const Store* m_store = nullptr;
    usize        m_index = 0;

public:
    // The value type is the word type of the store with any const qualification dropped.
    using value_type = typename Store::word_type;

    // The difference type is always a `std::ptrdiff_t` (required by the iterator concept).
    using difference_type = std::ptrdiff_t;

    // Our constructor captures a pointer to the store and sets the index to 0
    WordsIter(Store const* store, usize idx = 0) : m_store(store), m_index{idx} {
        // Empty body ...
    }

    // Iterator are required to act like value types so must be default constructible, moveable, and copyable.
    WordsIter() = default;
    WordsIter(WordsIter&&) = default;
    WordsIter(WordsIter const&) = default;
    WordsIter& operator=(WordsIter&&) = default;
    WordsIter& operator=(WordsIter const&) = default;

    // Iterators must be comparable (this is how the range-for loops work).
    constexpr bool operator==(WordsIter const& other) const {
        return m_store == other.m_store && m_index == other.m_index;
    }

    // What is the iterator currently pointing to? (this is what the range-for loop will use).
    constexpr value_type operator*() const { return m_store->word(m_index); }

    // Any call to `begin` returns a new iterator pointing to the first unset bit in the store.
    constexpr auto begin() const { return WordsIter(m_store, 0); }

    // Any call to `end` returns a new iterator that is not initialized.
    constexpr auto end() const { return WordsIter(m_store, m_store->words()); }

    // Increment the iterator, returning the new value (the `i++` operator).
    constexpr WordsIter& operator++() {
        ++m_index;
        return *this;
    }

    // Increment the iterator, returning the previous value (the `++i` operator).
    constexpr WordsIter operator++(int) {
        auto prev = *this;
        ++*this;
        return prev;
    }

    // Decrement the iterator, returning the new value (the `--i` operator).
    constexpr WordsIter& operator--() {
        --m_index;
        return *this;
    }

    // Decrement the iterator, returning the previous value (the `--i` operator).
    constexpr WordsIter operator--(int) {
        auto prev = *this;
        --*this;
        return prev;
    }
};

} // namespace gf2
