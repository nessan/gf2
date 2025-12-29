# The `BitSet` Class

## Introduction

A `gf2::BitSet` is a _fixed_ size vector of bit elements stored compactly in an array of unsigned integer words. <br>
The size is set at compile time by a template parameter referred to here as `N`.

The class satisfies the `gf2::BitStore` concept, which provides a rich API for manipulating the bits in the bit-set.
The free functions defined for that concept are also pulled into the class as member functions.
For example, if `v` is a `gf2::BitSet`, you can call `v.count_ones()` to count the number of set bits in the bit-set instead of calling the free function `gf2::count_ones(v)`, though both forms are valid.

In mathematical terms, a bitset is a vector over [GF2], the simplest [Galois-Field] with just two elements usually denoted 0 & 1, as the booleans true & false, or as the bits set & unset.
Arithmetic over GF(2) is mod 2, so addition/subtraction becomes the `XOR` operation while multiplication/division becomes `AND`.

> [!NOTE]
> Operations on and between bitsets and other objects in the `gf2` library are implemented using bitwise operations on whole underlying words at a time.
> These operations are highly optimised in modern CPUs, allowing for fast computation even on large bitsets.
> It also means we never have to worry about overflows or carries as we would with normal integer arithmetic.

The `gf2::BitSet` class is a hybrid between a [`std::array`] and a [`std::bitset`], along with extra mathematical features to facilitate numerical work, and in particular, linear algebra.

It is worth noting that a `BitSet` prints in _vector-order_.

For example, a bitset of size four will print as $v_0 v_1 v_2 v_3$ with the elements in increasing index order, so the least significant vector element, $v_0$, comes **first** on the _left_.
Contrast that to a [`std::bitset`], which always prints in _bit-order_.
The equivalent `std::bitset` with four elements prints as $b_3 b_2 b_1 b_0$ with the least significant bit $b_0$ printed **last** on the _right_.

Of course, for many applications, printing in _bit-order_ makes perfect sense.
A bitset of four elements initialised to `0x1` will print `1000`.
A `std::bitset` prints the same value as `0001`, which will be more natural in some settings.

However, our main aim is numerical work, where vector order is more natural.
In particular, bit-order is unnatural for _matrices_ over GF(2).
It is too confusing to print a matrix in any order other than the one where the (0,0) element is at the top left, and proceed from there.

## Declaration

The declaration of the bit-set class looks like:

```c++
template <usize N, Unsigned Word = usize>
class BitSet {
public:
    using word_type = Word;
    // ...
};
```

The first template parameter `N` specifies the number of bit elements in the bitset, which are all initialised to zero.

The `Word` template parameter specifies the underlying `gf2::Unsigned` integer type used to hold the bit-set's bits, and the default is usually the most efficient type for the target platform.
On most modern platforms, that [`usize`] will be a 64-bit unsigned integer.
The choice of underlying word type is exposed via the public `word_type` type alias as required by the `gf2::BitStore` concept.

If your application calls for a vast number of bit-sets with only a few bits each, you might consider using `std::uint8_t` as the `Word` type to save memory.

> [!NOTE]
> You might notice that many of the [doctests] in the library use 8-bit underlying words.
> The reason is we want to exercise the various functions across word boundaries for modest, easily readable bit-stores.

## Methods Overview

The `gf2::BitSet` class provides a rich set of methods for querying and manipulating, bit-sets.
Here is an overview of the main methods available in the class:

| Category                                      | Description                                                                  |
| --------------------------------------------- | ---------------------------------------------------------------------------- |
| [Required Concept Methods](#set-concept)      | Methods needed to satisfy the `gf2::BitStore` concept.                       |
| [Bit Access](#set-bit-access)                 | Methods to access individual bit elements in a bit-set.                      |
| [Bit-set Queries](#set-queries)               | Methods to query the overall state of a bit-set.                             |
| [Bit-set Mutators](#set-mutators)             | Methods to mutate the overall state of a bit-set.                            |
| [Bit-set Fillers](#set-fillers)               | Methods to fill a bit-set from various sources.                              |
| [Span Creators](#set-span)                    | Methods to create non-owning views over a part of a bit-set --- _bit-spans_. |
| [Sub-vector Extractors](#set-sub-vectors)     | Methods to pull out a clone of piece of a bit-set as new bit-vector.         |
| [Riffling](#set-riffling)                     | Methods to create bit-vectors that copy a bit-set with interleaved zeros.    |
| [Set/Unset Indices](#set-indices)             | Methods to find the indices of set & unset bits in a bit-set.                |
| [Iterators](#set-iterators)                   | Methods to create various iterators over a bit-set.                          |
| [Stringification](#set-stringification)       | Methods to create string representations of a bit-set.                       |
| [Equality Operator](#equality-operator)       | Operator to compare bit-stores, including bit-sets for content equality.     |
| [Bit Shifts](#bit-shifts)                     | Operators to shift the bits in bit-sets left or right.                       |
| [Bit-wise Operators](#bit-wise-operators)     | Operators to combine bit-stores using logical operations.                    |
| [Arithmetic Operators](#arithmetic-operators) | Operators to add or subtract bit-stores.                                     |
| [Other Functions](#other-functions)           | Dot products, convolutions, concatenation etc. for bit-sets.                 |

## Concept Methods {#set-concept}

Bit-sets satisfy the `gf2::BitStore` concept and forwards many method calls to free functions defined for that concept.
The concept requires us to provide the following methods:

| Method Name               | Description                                                       |
| ------------------------- | ----------------------------------------------------------------- |
| `gf2::BitSet::size`       | Returns `N`, the number of bit elements in the bitset.            |
| `gf2::BitSet::words`      | Returns the number of `Word`s needed to store those elements.     |
| `gf2::BitSet::word`       | Returns the word for the passed index.                            |
| `gf2::BitSet::set_word`   | Sets the word at the passed index to the passed value.            |
| `gf2::BitSet::data const` | Returns a const pointer to the beginning of the `Word` store.     |
| `gf2::BitSet::data`       | Returns a non-const pointer to the beginning of the `Word` store. |
| `gf2::BitSet::offset`     | For bitsets, this always returns 0.                               |

These methods are trivial to implement for bit-sets.

The one place where care is needed is in the `set_word` method, which must ensure that any bits beyond the bitset's size remain set to zero.

> [!WARNING]
> While the `gf2::BitSet::data` method provides write access to the underlying words, you must be careful when modifying them directly.
> You must ensure that any bits beyond the bitset's size remain zero.
> The `gf2::BitSet::set_word` method takes care of this for you, so prefer using that method when possible.

## Bit Access {#set-bit-access}

The following functions provide access to individual bit elements in the bit-set.

| Function                    | Description                                                                                                         |
| --------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| `gf2::BitSet::get`          | Returns the value of a single bit element as a read-only boolean.                                                   |
| `gf2::BitSet::operator[]()` | Returns a `bool` in the const case and a `BitRef` with read-write access to a bit element in the non-const version. |
| `gf2::BitSet::front`        | Returns the value of the first element in the bit-set.                                                              |
| `gf2::BitSet::back`         | Returns the value of the last element in the bit-set.                                                               |
| `gf2::BitSet::set`          | Sets a bit to the given boolean value which defaults to `true`.                                                     |
| `gf2::BitSet::flip`         | Flips the value of the bit element at a given index.                                                                |
| `gf2::BitSet::swap`         | Swaps the values of bit elements at locations `i` and `j`.                                                          |

> [!NOTE]
> You can set the `DEBUG` flag at compile time to enable bounds checks on the index arguments.

The non-const version of `gf2::BitSet::operator[]()` returns a `gf2::BitRef`, which is "reference" to an individual bit in the vector.
It is automatically converted to a boolean on reads, but it also allows writes, which means you can write natural-looking single-bit assignments:

```c++
v[12] = true;
```

## Bit-set Queries {#set-queries}

The following functions let you query the overall state of a bit-set.

| Function                      | Description                                               |
| ----------------------------- | --------------------------------------------------------- |
| `gf2::BitSet::is_empty`       | Returns true if the bit-set is empty                      |
| `gf2::BitSet::any`            | Returns true if _any_ bit in the bit-set is set.          |
| `gf2::BitSet::all`            | Returns true if _every_ bit in the bit-set is set.        |
| `gf2::BitSet::none`           | Returns true if _no_ bit in the bit-set is set.           |
| `gf2::BitSet::count_ones`     | Returns the number of set bits in the bit-set.            |
| `gf2::BitSet::count_zeros`    | Returns the number of unset bits in the bit-set.          |
| `gf2::BitSet::leading_zeros`  | Returns the number of leading unset bits in the bit-set.  |
| `gf2::BitSet::trailing_zeros` | Returns the number of trailing unset bits in the bit-set. |

These methods efficiently operate on words at a time, so they are inherently parallel.

## Bit-set Mutators {#set-mutators}

The following functions let you mutate the entire bit-set in a single call.

| Function                      | Description                                      |
| ----------------------------- | ------------------------------------------------ |
| `gf2::BitVecSetVec::flip_all` | Flips the values of all the bits in the bit-set. |

They efficiently operate on words at a time, so they are inherently parallel.

## Bit-set Fillers {#set-fillers}

The following functions let you populate the entire bit-set from multiple sources in a single call.

| Function                        | Description                                                        |
| ------------------------------- | ------------------------------------------------------------------ |
| `gf2::BitSet::copy_from`        | Copies the bits from various sources to this bit-set.              |
| `gf2::BitSet::fill_from`        | Fills the bit-set by repeatedly calling a function for each index. |
| `gf2::BitSet::fill_from_random` | Fills the bit-set with random 0's and 1's.                         |

The `gf2::BitSet::copy_from` functions support copying from:

-   Another bit-store of the same size but possibly a different underlying word type.
-   A [`std::bitset`] of the same size as the bit-set.
-   _Any_ unsigned integer that has the same number of bits as the bit-set.

> [!NOTE]
> In each case, the source and destination sizes must match exactly.
> That condition is always checked unless the `NDEBUG` flag is set at compile time.
> You can always use a `gf2::BitSpan` to copy a subset of bits if needed.

By default, the random fill method uses a random number generator seeded with system entropy, so the results change from run to run.
You can set a specific seed to get reproducible fills.

The default probability that a bit is set is 50%, but you can pass a different probability in the range `[0.0, 1.0]` if desired.

## Span Creators {#set-span}

The following functions let you create a `gf2::BitSpan`, which is a non-owning view of some contiguous subset of bits in the bit-set.

| Function            | Description                                                                         |
| ------------------- | ----------------------------------------------------------------------------------- |
| `gf2::BitSet::span` | Returns a `gf2::BitSpan` encompassing the bits in a half-open range `[begin, end)`. |

There are two overloads of the `gf2::BitSet::span` method --- one for `const` bit-sets and one for non-`const` bit-sets:

```c++
auto span(usize begin, usize end);        // <1>
auto span(usize begin, usize end) const;  // <2>
```

1. Returns a mutable `gf2::BitSpan` that allows modification of the bits in the specified range.
2. Returns an immutable `gf2::BitSpan` that does not allow modification of the bits in the specified range.

In both cases, the `begin` and `end` arguments define a half-open range of bits in the vector.

Mutability/immutability of the returned `BitSpan` is _deep_.
The span's mutability reflects that of the underlying vector, so if the vector is mutable, so is the span, and vice versa.

This is similar to the C++20 [`std::span`] class for regular data collection types.

> [!NOTE]
> A `gf2::BitSpan` also satisfies the `gf2::BitStore` concept, so you can take a span of a span.

## Sub-vector Extractors {#set-sub-vectors}

The following methods create or fill _independent_ bit-vectors with copies of some contiguous subset of the bits in the vector.

| Function                | Description                                                                            |
| ----------------------- | -------------------------------------------------------------------------------------- |
| `gf2::BitSet::sub`      | Returns a new `gf2::BitVec` encompassing the bits in a half-open range `[begin, end)`. |
| `gf2::BitSet::split_at` | Fills two bit-vectors with the bits in the ranges `[0, at)` and `[at, size())`.        |

The `gf2::BitSet::split_at` method can optionally take two pre-existing bit-vectors to fill, thereby avoiding unnecessary allocations in some iterative algorithms that repeatedly use this method.

> [!NOTE]
> These methods do not alter the underlying vector.

## Riffling {#set-riffling}

We have methods that can interleave (_riffle_) the bits in a bit-set with zeros.

| Function               | Description                                                                         |
| ---------------------- | ----------------------------------------------------------------------------------- |
| `gf2::BitSet::riffled` | Fills a pre-existing bit-vector with the result of riffling this bit-set.           |
| `gf2::BitSet::riffled` | Returns a new bit-vector that is this bit-set with its bits interleaved with zeros. |

If the bit-set looks like $v_0 v_1 v_2 \ldots v_n$, then the riffling operation produces the vector $v_0 0 v_1 0 v_2 0 \ldots v_n$ where a zero is interleaved _between_ every bit in the original bit-set (there is no trailing zero at the end).

If you think of a bit-set as representing the coefficients of a polynomial over GF(2), then riffling corresponds to squaring that polynomial. See the documentation for `gf2::BitPoly::squared` for more information.

## Set/Unset Bit Indices {#set-indices}

The following functions find the indices of set or unset bits in the vector.

| Function                      | Description                                                                           |
| ----------------------------- | ------------------------------------------------------------------------------------- |
| `gf2::BitSet::first_set`      | Returns the index of the first set bit in the bit-set.                                |
| `gf2::BitSet::last_set`       | Returns the index of the last set bit in the bit-set.                                 |
| `gf2::BitSet::next_set`       | Returns the index of the next set bit in the bit-set _after_ the passed index.        |
| `gf2::BitSet::previous_set`   | Returns the index of the previous set bit in the bit-set _before_ the passed index.   |
| `gf2::BitSet::first_unset`    | Returns the index of the first unset bit in the bit-set.                              |
| `gf2::BitSet::last_unset`     | Returns the index of the last unset bit in the bit-set.                               |
| `gf2::BitSet::next_unset`     | Returns the index of the next unset bit in the bit-set _after_ the passed index.      |
| `gf2::BitSet::previous_unset` | Returns the index of the previous unset bit in the bit-set _before_ the passed index. |

## Iterators {#set-iterators}

The following functions create iterators for traversing the bits or underlying words in the bit-set:

-   Read-only iteration through the individual bits.
-   Read-write iteration through the individual bits.
-   Read-only iteration through the indices of the set bits.
-   Read-only iteration through the indices of the unset bits.
-   Read-write iteration through the underlying vector words.

| Function                   | Description                                                                        |
| -------------------------- | ---------------------------------------------------------------------------------- |
| `gf2::BitSet::bits`        | Returns a `gf2::BitsIter` iterator over the bits in the bit-set.                   |
| `gf2::BitSet::set_bits`    | Returns a `gf2::SetBitsIter` iterator to view the indices of all the set bits.     |
| `gf2::BitSet::unset_bits`  | Returns a `gf2::UnsetBitsIter` iterator to view the indices of all the unset bits. |
| `gf2::BitSet::store_words` | Returns a `gf2::WordsIter` iterator to view the "words" underlying the bit-set.    |
| `gf2::BitSet::to_words`    | Returns a copy of the "words" underlying the bit-set.                              |

There are two overloads of the `gf2::BitSet::bits` method --- one for `const` bit-stores and one for non-`const` bit-stores:

```c++
auto bits();        // <1>
auto bits() const;  // <2>
```

1. Returns a mutable `gf2::BitsIter` that allows modification of the bits in the bit-set.
2. Returns an immutable `gf2::BitsIter` that only allows one to view the bits in the bit-set.

## Stringification {#set-stringification}

The following methods return a string representation for a bit-set.
The string can be in the obvious binary format or a more compact hex format.

| Function                        | Description                                                |
| ------------------------------- | ---------------------------------------------------------- |
| `gf2::BitSet::to_string`        | Returns a default string representation for a bit-set.     |
| `gf2::BitSet::to_pretty_string` | Returns a "pretty" string representation for a bit-set.    |
| `gf2::BitSet::to_binary_string` | Returns a binary string representation for a bit-set.      |
| `gf2::BitSet::to_hex_string`    | Returns a compact hex string representation for a bit-set. |

## Other Operators and Functions

There are many operators and free functions defined for any `gf2::BitStore` compatible class, including:

| Category                                      | Description                                                              |
| --------------------------------------------- | ------------------------------------------------------------------------ |
| [Equality Operator](#equality-operator)       | Operator to compare bit-stores, including bit-sets for content equality. |
| [Bit Shifts](#bit-shifts)                     | Operators to shift the bits in bit-sets left or right.                   |
| [Bit-wise Operators](#bit-wise-operators)     | Operators to combine bit-stores using logical operations.                |
| [Arithmetic Operators](#arithmetic-operators) | Operators to add or subtract bit-stores.                                 |
| [Other Functions](#other-functions)           | Dot products, convolutions, concatenation etc. for bit-sets.             |

## See Also

-   `gf2::BitSet` reference for detailed documentation with examples for each method.
-   [`BitStore`](BitStore.md) for the concept API shared by all bit-stores.
-   [`BitVec`](BitVec.md) for dynamically-sized vectors of bits.
-   [`BitSpan`](BitSpan.md) for non-owning views of some of the bits in a bit-set.
-   [`BitMat`](BitMat.md) for matrices of bits.
-   [`BitPoly`](BitPoly.md) for polynomials over GF(2).

<!-- Reference Links -->

[GF2]: https://en.wikipedia.org/wiki/GF(2)
[Galois-Field]: https://en.wikipedia.org/wiki/Galois_field
[`std::vector`]: https://en.cppreference.com/w/cpp/container/vector
[`std::optional`]: https://en.cppreference.com/w/cpp/utility/optional
[`std::nullopt`]: https://en.cppreference.com/w/cpp/utility/optional/nullopt
[`std::bitset`]: https://en.cppreference.com/w/cpp/utility/bitset
[`std::span`]: https://en.cppreference.com/w/cpp/container/span.html
[`usize`]: https://en.cppreference.com/w/cpp/types/size_t
[`boost::dynamic_bitset`]: https://www.boost.org/doc/libs/release/libs/dynamic_bitset/dynamic_bitset.html
[doctests]: https://nessan.github.io/doxytest/
[string-encodings]: BitStore.md#string-encodings
