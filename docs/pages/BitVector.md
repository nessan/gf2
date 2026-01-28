# The `BitVector` Class

## Introduction

A `gf2::BitVector` is a dynamically sized vector of bit elements stored compactly in an array of unsigned integer words.

The class satisfies the `gf2::BitStore` concept, which provides a rich API for manipulating the bits in the vector.
The free functions defined for that concept are also pulled into the class as member functions.
For example, if `v` is a `gf2::BitVector`, you can call `v.count_ones()` to count the number of set bits in the vector instead of calling the free function `gf2::count_ones(v)`, though both forms are valid.

In addition to the many methods that mirror a corresponding utility function defined in `BitStore.h`, the `gf2::BitVector` class provides methods to construct bit-vectors in various ways, methods to resize a vector, and methods to append or remove elements from the end of a vector.

In mathematical terms, a bit-vector is a vector over [GF2], the simplest [Galois-Field] with just two elements, usually denoted 0 & 1, as the booleans true & false, or as the bits set & unset.
Arithmetic over GF(2) is mod 2, so addition/subtraction becomes the `XOR` operation while multiplication/division becomes `AND`.

> [!NOTE]
> Operations on and between bit-vectors and other objects in the `gf2` library are implemented using bitwise operations on whole underlying words at a time.
> These operations are highly optimised in modern CPUs, allowing for fast computation even on large bit-vectors.
> It also means we never have to worry about overflows or carries as we would with normal integer arithmetic.

The `gf2::BitVector` class is a hybrid between a [`std::vector`] and a [`std::bitset`], along with extra mathematical features to facilitate numerical work, and in particular, linear algebra.

One can dynamically resize a `BitVector` as needed.
Contrast this to a `std::bitset`, which has a _fixed_ size determined at compile time.
_Boost_ provides the [`boost::dynamic_bitset`] class, which allows runtime resizing, as its name suggests.
However, neither class supports algebraic operations.

It is worth noting that a `BitVector` is printed in _vector order_.
For example, a bit-vector of size four will print as $v_0 v_1 v_2 v_3$ with the elements in increasing index order, so the least significant vector element, $v_0$, comes **first** on the _left_.
Contrast that to a `std::bitset`, which always prints in _bit-order_.
The equivalent `std::bitset` with four elements prints as $b_3 b_2 b_1 b_0$ with the least significant bit $b_0$ printed **last** on the _right_.

Of course, for many applications, printing in _bit-order_ makes perfect sense.
A bit-vector of four elements initialised to `0x1` will print `1000`.
A `std::bitset` prints the same value as `0001`, which will be more natural in some settings.

However, our main aim is numerical work, where vector order is more natural.
In particular, bit-order is unnatural for _matrices_ over GF(2).
It is too confusing to print a matrix in any order other than the one where the (0,0) element is at the top left, and proceed from there.

## Declaration

The declaration of the bit-vector class looks like:

```c++
template <Unsigned Word = usize>
class BitVector {
public:
    using word_type = Word;
    // ...
};
```

The `Word` template parameter specifies the underlying `gf2::Unsigned` integer type used to hold the vector's bits, and the default is usually the most efficient type for the target platform.
On most modern platforms, that [`usize`] will be a 64-bit unsigned integer.
The choice of underlying word type is exposed via the public `word_type` type alias as required by the `gf2::BitStore` concept.

If your application calls for a vast number of bit-vectors with only a few bits each, you might consider using `std::uint8_t` as the `Word` type to save memory.

> [!NOTE]
> If you peruse the header files, you may notice that many of the [doctests] in the library use 8-bit underlying words.
> The reason is we want to exercise the various functions across word boundaries for modest, easily readable bit-stores.

## Methods Overview

The `gf2::BitVector` class provides a rich set of methods for constructing, querying, and manipulating, bit-vectors.
Here is an overview of the main methods available in the class:

| Category                                      | Description                                                                     |
| --------------------------------------------- | ------------------------------------------------------------------------------- |
| [Concept Methods](#vec-concept)               | Methods needed to satisfy the `gf2::BitStore` concept.                          |
| [Constructors](#vec-constructors)             | Methods to create bit-vectors of various sizes and initial values.              |
| [Factory Constructors](#vec-factory)          | Class methods to create bit-vectors with specific properties or fills.          |
| [Construction from Strings](#vec-strings)     | Class methods to create bit-vectors from string representations.                |
| [Resizing](#vec-resizing)                     | Methods to query and manipulate the size and capacity of a bit-vector.          |
| [Appending Elements](#vec-append)             | Methods to append bits from various sources to the end of a bit-vector.         |
| [Removing Elements](#vec-remove)              | Methods to remove bits from the end of a bit-vector.                            |
| [Bit Access](#vec-bit-access)                 | Methods to access individual bit elements in a bit-vector.                      |
| [Queries](#vec-queries)                       | Methods to query the overall state of a bit-vector.                             |
| [Mutators](#vec-mutators)                     | Methods to mutate the overall state of a bit-vector.                            |
| [Fills](#vec-fills)                           | Methods to fill a bit-vector from various sources.                              |
| [Exports](#vec-exports)                       | Methods to export the bits from a bit-vector to various destinations.           |
| [Spans](#vec-span)                            | Methods to create non-owning views over a part of a bit-vector --- _bit-spans_. |
| [Sub-vectors](#vec-sub-vectors)               | Methods to pull out a clone of piece of a bit-vector as new bit-vectors         |
| [Riffling](#vec-riffling)                     | Methods to create vectors that copy a bit-vector with interleaved zeros.        |
| [Set/Unset Indices](#vec-indices)             | Methods to find the indices of set & unset bits in a bit-vector.                |
| [Iterators](#vec-iterators)                   | Methods to create various iterators over a bit-vector.                          |
| [Stringification](#vec-stringification)       | Methods to create string representations of a bit-vector.                       |
| [Equality Operator](#equality-operator)       | Operator to compare bit-stores, including bit-vectors for content equality.     |
| [Bit Shifts](#bit-shifts)                     | Operators to shift the bits in bit-vectors left or right.                       |
| [Bit-wise Operators](#bit-wise-operators)     | Operators to combine bit-stores using logical operations.                       |
| [Arithmetic Operators](#arithmetic-operators) | Operators to add or subtract bit-stores.                                        |
| [Other Functions](#other-functions)           | Dot products, convolutions, concatenation etc. for bit-vectors.                 |

## Concept Methods {#vec-concept}

Bit-vectors satisfy the `gf2::BitStore` concept and forwards many method calls to free functions defined for that concept.
The concept requires us to provide the following methods:

| Method Name                   | Description                                                                 |
| ----------------------------- | --------------------------------------------------------------------------- |
| `gf2::BitVector::size`        | Returns the number of bit elements in the bit-vector.                       |
| `gf2::BitVector::store const` | Returns a const pointer to the first `Word` holding bits in the vector.     |
| `gf2::BitVector::store`       | Returns a non-const pointer to the first `Word` holding bits in the vector. |
| `gf2::BitVector::offset`      | For bit-vectors, this always returns 0.                                     |
| `gf2::BitVector::words`       | Returns the number of `Word`s needed to vector those elements.              |
| `gf2::BitVector::word`        | Returns the word for the passed index.                                      |
| `gf2::BitVector::set_word`    | Sets the word at the passed index to the passed value.                      |

These methods are trivial to implement for bit-vectors.

The one place where care is needed is in the `gf2::BitVector::set_word` method, which must ensure that any bits beyond the size of the bit-vector remain set to zero.

> [!WARNING]
> While the `gf2::BitVector::store` method provides write access to the underlying words, you must be careful when modifying them directly.
> You must ensure that any bits beyond the bit-vector's size remain zero.
> The `gf2::BitVector::set_word` method takes care of this for you, so prefer using that method when possible.

## Constructors {#vec-constructors}

The default constructor for a `gf2::BitVector` creates an _empty_ bit-vector with zero size.
You can also create a bit-vector of a given size using the following constructors:

| Method Name                   | Description                                                                  |
| ----------------------------- | ---------------------------------------------------------------------------- |
| `gf2::BitVector(usize)`       | Constructs a bit-vector of the given length where all the elements are zero. |
| `gf2::BitVector(usize, Word)` | Constructs a bit-vector by repeatedly copying the bits from the `Word`       |

## Factory Constructors {#vec-factory}

There are also many static factory construction functions.

| Method Name                     | Description                                                                               |
| ------------------------------- | ----------------------------------------------------------------------------------------- |
| `gf2::BitVector::with_capacity` | Returns a zero-sized bit-vector that can add some elements without any extra allocations. |
| `gf2::BitVector::zeros`         | Returns a bit-vector where all the elements are 0.                                        |
| `gf2::BitVector::ones`          | Returns a bit-vector where all the elements are 1.                                        |
| `gf2::BitVector::constant`      | Returns a bit-vector where all the elements are whatever is passed as a `value`.          |
| `gf2::BitVector::unit`          | Returns a bit-vector where all the elements are zero except for a single 1.               |
| `gf2::BitVector::alternating`   | Returns a bit-vector where all the elements follow the pattern `101010...`                |
| `gf2::BitVector::from`          | Returns a `BitVector` filled with bits from various sources.                              |
| `gf2::BitVector::random`        | Returns a `BitVector` with a random fill seeded from entropy.                             |
| `gf2::BitVector::seeded_random` | Returns a `BitVector` with a reproducible random fill.                                    |
| `gf2::BitVector::biased_random` | Returns a random `BitVector` where you set the probability of bits being 1.               |

The `gf2::BitVector::from` factory method is overloaded to allow construction from:

- _Any_ other `gf2::BitStore` object. The source object need not share the same underlying storage type. <br> This allows conversions from bit-stores based on a different word type.
- _Any_ unsigned integer type, where we copy the bits corresponding to the value.<br> The source type need not be the same as the underlying `Word` type used by the bit-vector. The size of the resulting vector is determined by the number of bits in the source type.
- A [`std::bitset`] where we copy the bits over.
- A callable object (a function of some sort) that generates bits on demand when passed an index.

## Construction from Strings {#vec-strings}

We can construct a `gf2::BitVector` from strings --- these methods can fail, so they return a `std::optional<gf2::BitVector>` and [`std::nullopt`] if the string cannot be parsed.

| Method Name                          | Description                                                  |
| ------------------------------------ | ------------------------------------------------------------ |
| `gf2::BitVector::from_string`        | Tries to construct a bit-vector from an arbitrary string.    |
| `gf2::BitVector::from_binary_string` | Tries to construct a bit-vector from a _binary_ string.      |
| `gf2::BitVector::from_hex_string`    | Tries to construct a bit-vector from a _hexadecimal_ string. |

Space, comma, single quote, and underscore characters are removed from the string.

If the string has an optional `"0b"` prefix, it is assumed to be binary.
If it has an optional `"0x"` prefix, it is assumed to be hex.
If there is no prefix and the string consists entirely of 0s and 1s, we assume it is binary; otherwise, we think it is hex.

> [!WARNING]
> This means the string `"0x11` is interpreted as the bit-vector of size 8 `"11110001"`, whereas the same string without a prefix, `"11"` is interpreted as the bit-vector of size 2 `"11"`.
> To avoid any ambiguity, it is best to use a prefix.

See the [string-encodings] section in the `BitStore` documentation for more details on the accepted string formats.

## Resizing {#vec-resizing}

We have methods to query and manipulate the size and capacity of a bit-vector:

| Method Name                          | Description                                                                                      |
| ------------------------------------ | ------------------------------------------------------------------------------------------------ |
| `gf2::BitVector::size`               | Returns the number of bit elements in the bit-vector.                                            |
| `gf2::BitVector::capacity`           | Returns the total number of bits the vector can hold without allocating more memory.             |
| `gf2::BitVector::remaining_capacity` | Returns the number of _additional_ elements we can store in the bit-vector without reallocating. |
| `gf2::BitVector::shrink_to_fit`      | Tries to shrink the vector's capacity as much as possible.                                       |
| `gf2::BitVector::clear`              | Sets the `size()` to zero. Leaves the capacity unaltered.                                        |
| `gf2::BitVector::resize`             | Resizes the bit-vector, either adding zeros, or truncating existing elements.                    |
| `gf2::BitVector::clean`              | Sets any unused bits in the _last_ occupied word to 0.                                           |

The `gf2::BitVector::clean` method is primarily used internally in the library.

## Appending Elements {#vec-append}

We have methods to append elements from various sources to the end of a bit-vector:

| Method Name                        | Description                                                            |
| ---------------------------------- | ---------------------------------------------------------------------- |
| `gf2::BitVector::push`             | Pushes a single bit (0 or 1) onto the end of the bit-vector.           |
| `gf2::BitVector::append`           | Appends bits from various sources to the end of the bit-vector.        |
| `gf2::BitVector::append_digit`     | Appends a "character's" worth of bits to the end of the bit-vector.    |
| `gf2::BitVector::append_hex_digit` | Appends four bits from a "hex-character" to the end of the bit-vector. |

The `gf2::BitVector::append` method is overloaded to allow appending bits from:

- A [`std::bitset`] where we append the bits to the end of the bit-vector.
- _Any_ unsigned integer type, where we append the bits corresponding to the value.<br> The type need not be the same as the underlying `Word` type used by the bit-vector.
- _Any_ other `gf2::BitStore` object, which need not share the same underlying `Word` storage type.

The `gf2::BitVector::append_digit` method appends bits from a character representing a digit in one of the bases 2, 4, 8, or 16.
It does nothing if it fails to parse the character.

## Removing Elements {#vec-remove}

We have methods to remove elements from the end of a bit-vector:

| Method Name                          | Description                                                                                     |
| ------------------------------------ | ----------------------------------------------------------------------------------------------- |
| `gf2::BitVector::pop`                | Removes the last bit from the bit-vector and returns it.                                        |
| `gf2::BitVector::split_off_unsigned` | Removes a single arbitrary-sized unsigned integer off the end of the bit-vector and returns it. |
| `gf2::BitVector::split_off`          | Splits a bit-vector into two at a given index                                                   |

The first two methods return the removed elements as a [`std::optional`], and as a [`std::nullopt`] if the vector is empty.

The two `gf2::BitVector::split_off` methods complement the `gf2::BitVector::split_at` methods.
These versions change the size of the bit-vector _in place_.

## Bit Access {#vec-bit-access}

The following methods provide access to individual bit elements in the bit-vector.

| Function                       | Description                                                                                                         |
| ------------------------------ | ------------------------------------------------------------------------------------------------------------------- |
| `gf2::BitVector::get`          | Returns the value of a single bit element as a read-only boolean.                                                   |
| `gf2::BitVector::operator[]()` | Returns a `bool` in the const case and a `BitRef` with read-write access to a bit element in the non-const version. |
| `gf2::BitVector::front`        | Returns the value of the first element in the vector.                                                               |
| `gf2::BitVector::back`         | Returns the value of the last element in the vector.                                                                |
| `gf2::BitVector::set`          | Sets a bit to the given boolean value which defaults to `true`.                                                     |
| `gf2::BitVector::flip`         | Flips the value of the bit element at a given index.                                                                |
| `gf2::BitVector::swap`         | Swaps the values of bit elements at locations `i` and `j`.                                                          |

> [!NOTE]
> You can set the `DEBUG` flag at compile time to enable bounds checks on the index arguments.

The non-const version of `gf2::BitVector::operator[]()` returns a `gf2::BitRef`, which is "reference" to an individual bit in the vector.
It is automatically converted to a boolean on reads, but it also allows writes, which means you can write natural-looking single-bit assignments:

```c++
v[12] = true;
```

This is equivalent to calling `v.set(12, true);`.

## Queries {#vec-queries}

The following methods let you query the overall state of a bit-vector.

| Function                         | Description                                              |
| -------------------------------- | -------------------------------------------------------- |
| `gf2::BitVector::is_empty`       | Returns true if the vector is empty                      |
| `gf2::BitVector::any`            | Returns true if _any_ bit in the vector is set.          |
| `gf2::BitVector::all`            | Returns true if _every_ bit in the vector is set.        |
| `gf2::BitVector::none`           | Returns true if _no_ bit in the vector is set.           |
| `gf2::BitVector::count_ones`     | Returns the number of set bits in the vector.            |
| `gf2::BitVector::count_zeros`    | Returns the number of unset bits in the vector.          |
| `gf2::BitVector::leading_zeros`  | Returns the number of leading unset bits in the vector.  |
| `gf2::BitVector::trailing_zeros` | Returns the number of trailing unset bits in the vector. |

These methods efficiently operate on words at a time, so they are inherently parallel.

## Mutators {#vec-mutators}

The following methods let you mutate the entire vector in a single call.

| Function                   | Description                                                                    |
| -------------------------- | ------------------------------------------------------------------------------ |
| `gf2::BitVector::set_all`  | Sets all the bits in the vector to the passed value, which defaults to `true`. |
| `gf2::BitVector::flip_all` | Flips the values of all the bits in the vector.                                |

The methods operate on words at a time, so are inherently parallel.

## Copies & Fills {#vec-fills}

The following methods let you populate the entire vector in a single call.

| Function                      | Description                                                            |
| ----------------------------- | ---------------------------------------------------------------------- |
| `gf2::BitVector::copy`        | Makes the bits in this vector identical to those from various sources. |
| `gf2::BitVector::fill_random` | Fills the vector with random 0's and 1's.                              |

### Copies

The `gf2::BitVector::copy` method is overloaded to copy bit values from various sources, where the size of bit-vector **must** match the number of bits in the source:

- Another bit-store of the same size but possibly a different underlying word type.
- A single unsigned integer value, which need not be the same type as the underlying `Word` used here.
- An iteration of unsigned integer values, which need not be the same type as the underlying `Word` used here.
- A function or callable object that takes a single `usize` index argument and returns a boolean value for that index.
- A [`std::bitset`] of the same size as the bit-vector.

> [!NOTE]
> In each case, the _number of bits_ in the source and destination must match exactly and that condition is always checked unless the `NDEBUG` flag is set at compile time. You can always use a `gf2::BitSpan` to copy a subset of bits if needed.

### Random Fills

By default, the random fill method uses a random number generator seeded with system entropy, so the results change from run to run.
You can set a specific seed to get reproducible fills.

The default probability that a bit is set is 50%, but you can pass a different probability in the range `[0.0, 1.0]` if desired.

## Exports {#vec-exports}

The following overloaded method lets you export the bits in the bit-vector to various destinations.

| Method                     | Description                                           |
| -------------------------- | ----------------------------------------------------- |
| `gf2::BitVector::to_words` | Exports the bits in the bit-vector as unsigned words. |

The ``gf2::BitVector::to_words` function can be passed an output iterator to fill where we assume:

- The output iterator points to a location that can accept values of the underlying word type.
- There is enough space at the output location to hold all those words.

If `gf2::BitVector::to_words` is called with no argument it returns a new `std::vector` of the underlying word type.

**Note:** The final word in the output may have unused high-order bits that are guaranteed to be set to zero.

## Spans {#vec-span}

The following methods let you create a `gf2::BitSpan`, which is a non-owning view of some contiguous subset of bits in the vector.

| Function               | Description                                                                         |
| ---------------------- | ----------------------------------------------------------------------------------- |
| `gf2::BitVector::span` | Returns a `gf2::BitSpan` encompassing the bits in a half-open range `[begin, end)`. |

There are two overloads of the `gf2::BitVector::span` method --- one for `const` bit-stores and one for non-`const` bit-stores:

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

## Sub-vectors {#vec-sub-vectors}

The following methods create or fill _independent_ bit-vectors with copies of some contiguous subset of the bits in the vector.

| Function                   | Description                                                                               |
| -------------------------- | ----------------------------------------------------------------------------------------- |
| `gf2::BitVector::sub`      | Returns a new `gf2::BitVector` encompassing the bits in a half-open range `[begin, end)`. |
| `gf2::BitVector::split_at` | Fills two bit-vectors with the bits in the ranges `[0, at)` and `[at, size())`.           |

The `gf2::BitVector::split_at` method can optionally take two pre-existing bit-vectors to fill, thereby avoiding unnecessary allocations in some iterative algorithms that repeatedly use this method.

> [!NOTE]
> These methods do not alter the underlying vector.

## Riffling {#vec-riffling}

We have methods that can interleave (_riffle_) the bits in a vector with zeros.

| Function                  | Description                                                                        |
| ------------------------- | ---------------------------------------------------------------------------------- |
| `gf2::BitVector::riffled` | Fills a pre-existing bit-vector with the result of riffling this vector.           |
| `gf2::BitVector::riffled` | Returns a new bit-vector that is this vector with its bits interleaved with zeros. |

If the vector looks like $v_0 v_1 v_2 \ldots v_n$, then the riffling operation produces the vector $v_0 0 v_1 0 v_2 0 \ldots v_n$ where a zero is interleaved _between_ every bit in the original vector (there is no trailing zero at the end).

If you think of a bit-vector as representing the coefficients of a polynomial over GF(2), then riffling corresponds to squaring that polynomial. See the documentation for `gf2::BitPolynomial::squared` for more information.

## Set/Unset Bit Indices {#vec-indices}

The following methods find the indices of set or unset bits in the vector.

| Function                         | Description                                                                          |
| -------------------------------- | ------------------------------------------------------------------------------------ |
| `gf2::BitVector::first_set`      | Returns the index of the first set bit in the vector.                                |
| `gf2::BitVector::last_set`       | Returns the index of the last set bit in the vector.                                 |
| `gf2::BitVector::next_set`       | Returns the index of the next set bit in the vector _after_ the passed index.        |
| `gf2::BitVector::previous_set`   | Returns the index of the previous set bit in the vector _before_ the passed index.   |
| `gf2::BitVector::first_unset`    | Returns the index of the first unset bit in the vector.                              |
| `gf2::BitVector::last_unset`     | Returns the index of the last unset bit in the vector.                               |
| `gf2::BitVector::next_unset`     | Returns the index of the next unset bit in the vector _after_ the passed index.      |
| `gf2::BitVector::previous_unset` | Returns the index of the previous unset bit in the vector _before_ the passed index. |

## Iterators {#vec-iterators}

The following methods create iterators for traversing the bits or underlying words in the vector:

- Read-only iteration through the individual bits.
- Read-write iteration through the individual bits.
- Read-only iteration through the indices of the set bits.
- Read-only iteration through the indices of the unset bits.
- Read-write iteration through the underlying vector words.

| Function                      | Description                                                                    |
| ----------------------------- | ------------------------------------------------------------------------------ |
| `gf2::BitVector::bits`        | Returns a `gf2::Bits` iterator over the bits in the vector.                    |
| `gf2::BitVector::set_bits`    | Returns a `gf2::SetBits` iterator to view the indices of all the set bits.     |
| `gf2::BitVector::unset_bits`  | Returns a `gf2::UnsetBits` iterator to view the indices of all the unset bits. |
| `gf2::BitVector::store_words` | Returns a `gf2::Words` iterator to view the "words" underlying the vector.     |

There are two overloads of the `gf2::BitVector::bits` method --- one for `const` bit-stores and one for non-`const` bit-stores:

```c++
auto bits();        // <1>
auto bits() const;  // <2>
```

1. Returns a mutable `gf2::Bits` that allows modification of the bits in the vector.
2. Returns an immutable `gf2::Bits` that only allows one to view the bits in the vector.

## Stringification {#vec-stringification}

The following methods return a string representation for a bit-vector.
The string can be in the obvious binary format or a more compact hex format.

| Function                           | Description                                                   |
| ---------------------------------- | ------------------------------------------------------------- |
| `gf2::BitVector::to_string`        | Returns a default string representation for a bit-vector.     |
| `gf2::BitVector::to_pretty_string` | Returns a "pretty" string representation for a bit-vector.    |
| `gf2::BitVector::to_binary_string` | Returns a binary string representation for a bit-vector.      |
| `gf2::BitVector::to_hex_string`    | Returns a compact hex string representation for a bit-vector. |

## Other Operators and Functions

There are many operators and free functions defined for any `gf2::BitStore` compatible class, including:

| Category                                      | Description                                                                 |
| --------------------------------------------- | --------------------------------------------------------------------------- |
| [Equality Operator](#equality-operator)       | Operator to compare bit-stores, including bit-vectors for content equality. |
| [Bit Shifts](#bit-shifts)                     | Operators to shift the bits in bit-vectors left or right.                   |
| [Bit-wise Operators](#bit-wise-operators)     | Operators to combine bit-stores using logical operations.                   |
| [Arithmetic Operators](#arithmetic-operators) | Operators to add or subtract bit-stores.                                    |
| [Other Functions](#other-functions)           | Dot products, convolutions, concatenation etc. for bit-vectors.             |

## See Also

- `gf2::BitVector` reference for detailed documentation with examples for each method.
- [`BitStore`](BitStore.md) for the concept API shared by all bit-stores.
- [`BitArray`](BitArray.md) for fixed-size vectors of bits.
- [`BitSpan`](BitSpan.md) for non-owning views of some of the bits in a bit-vector.
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
