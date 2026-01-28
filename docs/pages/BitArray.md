# The `BitArray` Class

## Introduction

A `gf2::BitArray` is a _fixed_ size vector of bit elements stored compactly in an array of unsigned integer words. <br>
The size is set at compile time by a template parameter referred to here as `N`.

The class satisfies the `gf2::BitStore` concept, which provides a rich API for manipulating the bits in the bit-array.
The free functions defined for that concept are also pulled into the class as member functions.
For example, if `v` is a `gf2::BitArray`, you can call `v.count_ones()` to count the number of set bits in the bit-array instead of calling the free function `gf2::count_ones(v)`, though both forms are valid.

In mathematical terms, a bitset is a vector over [GF2], the simplest [Galois-Field] with just two elements usually denoted 0 & 1, as the booleans true & false, or as the bits set & unset.
Arithmetic over GF(2) is mod 2, so addition/subtraction becomes the `XOR` operation while multiplication/division becomes `AND`.

> [!NOTE]
> Operations on and between bitsets and other objects in the `gf2` library are implemented using bitwise operations on whole underlying words at a time.
> These operations are highly optimised in modern CPUs, allowing for fast computation even on large bitsets.
> It also means we never have to worry about overflows or carries as we would with normal integer arithmetic.

The `gf2::BitArray` class is a hybrid between a [`std::array`] and a [`std::bitset`], along with extra mathematical features to facilitate numerical work, and in particular, linear algebra.

It is worth noting that a `BitArray` prints in _vector-order_.

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

The declaration of the bit-array class looks like:

```c++
template <usize N, Unsigned Word = usize>
class BitArray {
public:
    using word_type = Word;
    // ...
};
```

The first template parameter `N` specifies the number of bit elements in the bitset, which are all initialised to zero.

The `Word` template parameter specifies the underlying `gf2::Unsigned` integer type used to hold the bit-array's bits, and the default is usually the most efficient type for the target platform.
On most modern platforms, that [`usize`] will be a 64-bit unsigned integer.
The choice of underlying word type is exposed via the public `word_type` type alias as required by the `gf2::BitStore` concept.

If your application calls for a vast number of bit-arrays with only a few bits each, you might consider using `std::uint8_t` as the `Word` type to save memory.

> [!NOTE]
> You might notice that many of the [doctests] in the library use 8-bit underlying words.
> The reason is we want to exercise the various functions across word boundaries for modest, easily readable bit-stores.

## Methods Overview

The `gf2::BitArray` class provides a rich set of methods for querying and manipulating, bit-arrays.
Here is an overview of the main methods available in the class:

| Category                                      | Description                                                                    |
| --------------------------------------------- | ------------------------------------------------------------------------------ |
| [Concept Methods](#array-concept)             | Methods needed to satisfy the `gf2::BitStore` concept.                         |
| [Constructors](#array-constructors)           | Methods to create bit-arrays with various initial values..                     |
| [Factory Constructors](#array-factory)        | Class methods to create bit-arrays with specific fills.                        |
| [Bit Access](#array-bit-access)               | Methods to access individual bit elements in a bit-array.                      |
| [Queries](#array-queries)                     | Methods to query the overall state of a bit-array.                             |
| [Mutators](#array-mutators)                   | Methods to mutate the overall state of a bit-array.                            |
| [Fills](#array-fills)                         | Methods to fill a bit-array from various sources.                              |
| [Exports](#array-exports)                     | Methods to export the bits in the bit-array to various destinations.           |
| [Spans](#array-span)                          | Methods to create non-owning views over a part of a bit-array --- _bit-spans_. |
| [Sub-vectors](#array-sub-vectors)             | Methods to pull out a clone of piece of a bit-array as new bit-vector.         |
| [Riffling](#array-riffling)                   | Methods to create bit-vectors that copy a bit-array with interleaved zeros.    |
| [Set/Unset Indices](#array-indices)           | Methods to find the indices of set & unset bits in a bit-array.                |
| [Iterators](#array-iterators)                 | Methods to create various iterators over a bit-array.                          |
| [Stringification](#array-stringification)     | Methods to create string representations of a bit-array.                       |
| [Equality Operator](#equality-operator)       | Operator to compare bit-stores, including bit-arrays for content equality.     |
| [Bit Shifts](#bit-shifts)                     | Operators to shift the bits in bit-arrays left or right.                       |
| [Bit-wise Operators](#bit-wise-operators)     | Operators to combine bit-stores using logical operations.                      |
| [Arithmetic Operators](#arithmetic-operators) | Operators to add or subtract bit-stores.                                       |
| [Other Functions](#other-functions)           | Dot products, convolutions, concatenation etc. for bit-arrays.                 |

## Concept Methods {#array-concept}

Bit-arrays satisfy the `gf2::BitStore` concept and forwards many method calls to free functions defined for that concept.
The concept requires us to provide the following methods:

| Method Name                  | Description                                                       |
| ---------------------------- | ----------------------------------------------------------------- |
| `gf2::BitArray::size`        | Returns `N`, the number of bit elements in the bitset.            |
| `gf2::BitArray::words`       | Returns the number of `Word`s needed to store those elements.     |
| `gf2::BitArray::word`        | Returns the word for the passed index.                            |
| `gf2::BitArray::set_word`    | Sets the word at the passed index to the passed value.            |
| `gf2::BitArray::store const` | Returns a const pointer to the beginning of the `Word` store.     |
| `gf2::BitArray::store`       | Returns a non-const pointer to the beginning of the `Word` store. |
| `gf2::BitArray::offset`      | For bitsets, this always returns 0.                               |

These methods are trivial to implement for bit-arrays.

The one place where care is needed is in the `set_word` method, which must ensure that any bits beyond the bitset's size remain set to zero.

> [!WARNING]
> While the `gf2::BitArray::store` method provides write access to the underlying words, you must be careful when modifying them directly.
> You must ensure that any bits beyond the bitset's size remain zero.
> The `gf2::BitArray::set_word` method takes care of this for you, so prefer using that method when possible.

## Constructors {#array-constructors}

The default constructor for a `gf2::BitArray<N>` creates an a bit-array with `N` bits, all initialised to zero.
You can also create a bit-vector with an initial fill using the following constructor:

| Method Name           | Description                                                                       |
| --------------------- | --------------------------------------------------------------------------------- |
| `gf2::BitArray()`     | Constructs a bit-vector with `N` zero elements.                                   |
| `gf2::BitArray(Word)` | Constructs a bit-array of size `N` by repeatedly copying the bits from the `Word` |

## Factory Constructors {#array-factory}

There are also many static factory construction functions.

| Method Name                    | Description                                                                     |
| ------------------------------ | ------------------------------------------------------------------------------- |
| `gf2::BitArray::zeros`         | Returns a bit-array where all the elements are 0.                               |
| `gf2::BitArray::ones`          | Returns a bit-array where all the elements are 1.                               |
| `gf2::BitArray::constant`      | Returns a bit-array where all the elements are whatever is passed as a `value`. |
| `gf2::BitArray::unit`          | Returns a bit-array where all the elements are zero except for a single 1.      |
| `gf2::BitArray::alternating`   | Returns a bit-array where all the elements follow the pattern `101010...`       |
| `gf2::BitArray::from`          | Returns a `BitArray` filled with bits copied from various sources.              |
| `gf2::BitArray::random`        | Returns a `BitArray` with a random fill seeded from entropy.                    |
| `gf2::BitArray::seeded_random` | Returns a `BitArray` with a reproducible random fill.                           |
| `gf2::BitArray::biased_random` | Returns a random `BitArray` where you set the probability of bits being 1.      |

The `gf2::BitVector::from` factory method is overloaded to allow construction from:

- _Any_ unsigned integer value, where we copy the bits in the passed value.<br> The source type need not be the same as the underlying `Word` type used by the bit-array. The number of bits in the value must match the size of given by the `N` template parameter. This is enforced at compile time.
- An iteration of _any_ unsigned integer values, where we copy all the bits in that iteration.<br> The source type need not be the same as the underlying `Word` type used by the bit-vector. The total number of bits in the iteration must match the size of given by the `N` template parameter. This is enforced at runtime.
- A [`std::bitset`] where we copy the bits over.
- A callable object (a function of some sort) that generates bits on demand when passed an index.

## Bit Access {#array-bit-access}

The following methods provide access to individual bit elements in the bit-array.

| Function                      | Description                                                                                                         |
| ----------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| `gf2::BitArray::get`          | Returns the value of a single bit element as a read-only boolean.                                                   |
| `gf2::BitArray::operator[]()` | Returns a `bool` in the const case and a `BitRef` with read-write access to a bit element in the non-const version. |
| `gf2::BitArray::front`        | Returns the value of the first element in the bit-array.                                                            |
| `gf2::BitArray::back`         | Returns the value of the last element in the bit-array.                                                             |
| `gf2::BitArray::set`          | Sets a bit to the given boolean value which defaults to `true`.                                                     |
| `gf2::BitArray::flip`         | Flips the value of the bit element at a given index.                                                                |
| `gf2::BitArray::swap`         | Swaps the values of bit elements at locations `i` and `j`.                                                          |

> [!NOTE]
> You can set the `DEBUG` flag at compile time to enable bounds checks on the index arguments.

The non-const version of `gf2::BitArray::operator[]()` returns a `gf2::BitRef`, which is "reference" to an individual bit in the vector.
It is automatically converted to a boolean on reads, but it also allows writes, which means you can write natural-looking single-bit assignments:

```c++
v[12] = true;
```

## Queries {#array-queries}

The following methods let you query the overall state of a bit-array.

| Function                        | Description                                                 |
| ------------------------------- | ----------------------------------------------------------- |
| `gf2::BitArray::is_empty`       | Returns true if the bit-array is empty                      |
| `gf2::BitArray::any`            | Returns true if _any_ bit in the bit-array is set.          |
| `gf2::BitArray::all`            | Returns true if _every_ bit in the bit-array is set.        |
| `gf2::BitArray::none`           | Returns true if _no_ bit in the bit-array is set.           |
| `gf2::BitArray::count_ones`     | Returns the number of set bits in the bit-array.            |
| `gf2::BitArray::count_zeros`    | Returns the number of unset bits in the bit-array.          |
| `gf2::BitArray::leading_zeros`  | Returns the number of leading unset bits in the bit-array.  |
| `gf2::BitArray::trailing_zeros` | Returns the number of trailing unset bits in the bit-array. |

These methods efficiently operate on words at a time, so they are inherently parallel.

## Mutators {#array-mutators}

The following methods let you mutate the entire bit-array in a single call.

| Function                  | Description                                                                       |
| ------------------------- | --------------------------------------------------------------------------------- |
| `gf2::BitVector::set_all` | Sets all the bits in the bit-array to the passed value, which defaults to `true`. |
| `gf2::BitArray::flip_all` | Flips the values of all the bits in the bit-array.                                |

The methods operate on words at a time, so are inherently parallel.

## Copies & Fills {#array-fills}

The following methods let you populate the entire bit-array in a single call.

| Function                     | Description                                                               |
| ---------------------------- | ------------------------------------------------------------------------- |
| `gf2::BitArray::copy`        | Makes the bits in this bit-array identical to those from various sources. |
| `gf2::BitArray::fill_random` | Fills the bit-array with random 0's and 1's.                              |

### Copies

The `gf2::BitArray::copy` method is overloaded to copy bit values from various sources, where the size of bit-array **must** match the number of bits in the source:

- Another bit-store of the same size but possibly a different underlying word type.
- A single unsigned integer value, which need not be the same type as the underlying `Word` used here.
- An iteration of unsigned integer values, which need not be the same type as the underlying `Word` used here.
- A function or callable object that takes a single `usize` index argument and returns a boolean value for that index.
- A [`std::bitset`] of the same size as the bit-array.

> [!NOTE]
> In each case, the _number of bits_ in the source and destination must match exactly and that condition is always checked unless the `NDEBUG` flag is set at compile time. You can always use a `gf2::BitSpan` to copy a subset of bits if needed.

### Random Fills

By default, the random fill method uses a random number generator seeded with system entropy, so the results change from run to run.
You can set a specific seed to get reproducible fills.

The default probability that a bit is set is 50%, but you can pass a different probability in the range `[0.0, 1.0]` if desired.

## Exports {#array-exports}

The following overloaded method lets you export the bits in the bit-array to various destinations.

| Method                    | Description                                          |
| ------------------------- | ---------------------------------------------------- |
| `gf2::BitArray::to_words` | Exports the bits in the bit-array as unsigned words. |

The ``gf2::BitArray::to_words` function can be passed an output iterator to fill where we assume:

- The output iterator points to a location that can accept values of the underlying word type.
- There is enough space at the output location to hold all those words.

If `gf2::BitVector::to_words` is called with no argument it returns a new `std::vector` of the underlying word type.

**Note:** The final word in the output may have unused high-order bits that are guaranteed to be set to zero.

## Spans {#array-span}

The following methods let you create a `gf2::BitSpan`, which is a non-owning view of some contiguous subset of bits in the bit-array.

| Function              | Description                                                                         |
| --------------------- | ----------------------------------------------------------------------------------- |
| `gf2::BitArray::span` | Returns a `gf2::BitSpan` encompassing the bits in a half-open range `[begin, end)`. |

There are two overloads of the `gf2::BitArray::span` method --- one for `const` bit-arrays and one for non-`const` bit-arrays:

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

## Sub-vectors {#array-sub-vectors}

The following methods create or fill _independent_ bit-vectors with copies of some contiguous subset of the bits in the vector.

| Function                  | Description                                                                               |
| ------------------------- | ----------------------------------------------------------------------------------------- |
| `gf2::BitArray::sub`      | Returns a new `gf2::BitVector` encompassing the bits in a half-open range `[begin, end)`. |
| `gf2::BitArray::split_at` | Fills two bit-vectors with the bits in the ranges `[0, at)` and `[at, size())`.           |

The `gf2::BitArray::split_at` method can optionally take two pre-existing bit-vectors to fill, thereby avoiding unnecessary allocations in some iterative algorithms that repeatedly use this method.

> [!NOTE]
> These methods do not alter the underlying vector.

## Riffling {#array-riffling}

We have methods that can interleave (_riffle_) the bits in a bit-array with zeros.

| Function                 | Description                                                                           |
| ------------------------ | ------------------------------------------------------------------------------------- |
| `gf2::BitArray::riffled` | Fills a pre-existing bit-vector with the result of riffling this bit-array.           |
| `gf2::BitArray::riffled` | Returns a new bit-vector that is this bit-array with its bits interleaved with zeros. |

If the bit-array looks like $v_0 v_1 v_2 \ldots v_n$, then the riffling operation produces the vector $v_0 0 v_1 0 v_2 0 \ldots v_n$ where a zero is interleaved _between_ every bit in the original bit-array (there is no trailing zero at the end).

If you think of a bit-array as representing the coefficients of a polynomial over GF(2), then riffling corresponds to squaring that polynomial. See the documentation for `gf2::BitPolynomial::squared` for more information.

## Set/Unset Bit Indices {#array-indices}

The following methods find the indices of set or unset bits in the vector.

| Function                        | Description                                                                             |
| ------------------------------- | --------------------------------------------------------------------------------------- |
| `gf2::BitArray::first_set`      | Returns the index of the first set bit in the bit-array.                                |
| `gf2::BitArray::last_set`       | Returns the index of the last set bit in the bit-array.                                 |
| `gf2::BitArray::next_set`       | Returns the index of the next set bit in the bit-array _after_ the passed index.        |
| `gf2::BitArray::previous_set`   | Returns the index of the previous set bit in the bit-array _before_ the passed index.   |
| `gf2::BitArray::first_unset`    | Returns the index of the first unset bit in the bit-array.                              |
| `gf2::BitArray::last_unset`     | Returns the index of the last unset bit in the bit-array.                               |
| `gf2::BitArray::next_unset`     | Returns the index of the next unset bit in the bit-array _after_ the passed index.      |
| `gf2::BitArray::previous_unset` | Returns the index of the previous unset bit in the bit-array _before_ the passed index. |

## Iterators {#array-iterators}

The following methods create iterators for traversing the bits or underlying words in the bit-array:

- Read-only iteration through the individual bits.
- Read-write iteration through the individual bits.
- Read-only iteration through the indices of the set bits.
- Read-only iteration through the indices of the unset bits.
- Read-write iteration through the underlying vector words.

| Function                     | Description                                                                    |
| ---------------------------- | ------------------------------------------------------------------------------ |
| `gf2::BitArray::bits`        | Returns a `gf2::Bits` iterator over the bits in the bit-array.                 |
| `gf2::BitArray::set_bits`    | Returns a `gf2::SetBits` iterator to view the indices of all the set bits.     |
| `gf2::BitArray::unset_bits`  | Returns a `gf2::UnsetBits` iterator to view the indices of all the unset bits. |
| `gf2::BitArray::store_words` | Returns a `gf2::Words` iterator to view the "words" underlying the bit-array.  |
| `gf2::BitArray::to_words`    | Returns a copy of the "words" underlying the bit-array.                        |

There are two overloads of the `gf2::BitArray::bits` method --- one for `const` bit-stores and one for non-`const` bit-stores:

```c++
auto bits();        // <1>
auto bits() const;  // <2>
```

1. Returns a mutable `gf2::Bits` that allows modification of the bits in the bit-array.
2. Returns an immutable `gf2::Bits` that only allows one to view the bits in the bit-array.

## Stringification {#array-stringification}

The following methods return a string representation for a bit-array.
The string can be in the obvious binary format or a more compact hex format.

| Function                          | Description                                                  |
| --------------------------------- | ------------------------------------------------------------ |
| `gf2::BitArray::to_string`        | Returns a default string representation for a bit-array.     |
| `gf2::BitArray::to_pretty_string` | Returns a "pretty" string representation for a bit-array.    |
| `gf2::BitArray::to_binary_string` | Returns a binary string representation for a bit-array.      |
| `gf2::BitArray::to_hex_string`    | Returns a compact hex string representation for a bit-array. |

## Other Operators and Functions

There are many operators and free functions defined for any `gf2::BitStore` compatible class, including:

| Category                                      | Description                                                                |
| --------------------------------------------- | -------------------------------------------------------------------------- |
| [Equality Operator](#equality-operator)       | Operator to compare bit-stores, including bit-arrays for content equality. |
| [Bit Shifts](#bit-shifts)                     | Operators to shift the bits in bit-arrays left or right.                   |
| [Bit-wise Operators](#bit-wise-operators)     | Operators to combine bit-stores using logical operations.                  |
| [Arithmetic Operators](#arithmetic-operators) | Operators to add or subtract bit-stores.                                   |
| [Other Functions](#other-functions)           | Dot products, convolutions, concatenation etc. for bit-arrays.             |

## See Also

- `gf2::BitArray` reference for detailed documentation with examples for each method.
- [`BitStore`](BitStore.md) for the concept API shared by all bit-stores.
- [`BitVector`](BitVector.md) for dynamically-sized vectors of bits.
- [`BitSpan`](BitSpan.md) for non-owning views of some of the bits in a bit-array.
- [`BitMatrix`](BitMatrix.md) for matrices of bits.
- [`BitPolynomial`](BitPolynomial.md) for polynomials over GF(2).

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
