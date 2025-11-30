#pragma once
// Internal file for the gf2 library.
//
// In the CRTP idiom, the base `BitStore` class is templatized over its subclasses (the `Store` template parameter).
// Each concrete `Store` class is templatized over the particular choice of unsigned integer used to store its bits.
// The `BitStore` base class needs to have access to that unsigned integer type also which can raise a circular
// dependence issue.
//
// A little indirection eliminates the problem. We introduce a new type `store_word` which is specialised for the
// derived sub-classes. The type is trivial but it extracts the word type from the derived `Store` classes in a way
// that avoids circular dependencies.
//
// We also have some concepts that check whether two stores have the same underlying word type. There are many methods
// in the library that work on pairs of objects and we usually require that their word types match. It is possible
// to relax that requirement, but only at the cost of increased code complexity, for little practical gain.
//
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <gf2/unsigned_word.h>
#include <cstddef>
#include <concepts>
#include <type_traits>

namespace gf2 {

// A trivial struct that's specialised to provide the "true" underlying word type for classes of interest
template<typename Store>
struct store_word;

// clang-format off
// Forward declare classes of interest:
template<usize N, unsigned_word Word> class BitSet;
template<unsigned_word Word> class BitVec;
template<unsigned_word Word> class BitSpan;
template<unsigned_word Word> class BitMat;

// Specialise the `store_word` definition for each of those (dropping any `const` qualifier as needed):
template<usize N, unsigned_word Word> struct store_word<BitSet<N, Word>> { using word_type = Word; };
template<unsigned_word Word> struct store_word<BitVec<Word>>  { using word_type = Word; };
template<unsigned_word Word> struct store_word<BitSpan<Word>> { using word_type = std::remove_const_t<Word>; };
template<unsigned_word Word> struct store_word<BitMat<Word>>  { using word_type = Word; };
// clang-format on

// A concept you can use to check if a store has a specific underlying word type.
template<typename Store, typename Word>
concept store_word_is = std::same_as<typename store_word<Store>::word_type, Word>;

// A concept you can use to check if two bit-stores are using the same underlying word type.
template<typename Lhs, typename Rhs>
concept store_words_match = std::same_as<typename store_word<Lhs>::word_type, typename store_word<Rhs>::word_type>;

}; // namespace gf2
