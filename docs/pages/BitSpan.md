# The `BitSpan` Class

## Introduction

A `gf2::BitSpan` is a non-owning _view_ of bit elements stored compactly in an array of unsigned integer words.
Typically bit-spans are created as views into existing `gf2::BitSpan` or `gf2::BitVec` objects, but they can also be created from an existing bit-span or even from raw pointers to word arrays.

The class satisfies the `gf2::BitStore` concept, which provides a rich API for manipulating the bits in the bit-span.
The free functions defined for that concept are also pulled into the class as member functions.
For example, if `s` is a `gf2::BitSpan`, you can call `s.count_ones()` to count the number of set bits in the span instead of calling the free function `gf2::count_ones(s)`, though both forms are valid.

A span is a cheap way to work with a slice of a `gf2::BitSpan`, `gf2::BitVec`, or any other `gf2::BitStore`.
It never allocates or copies; it just remembers a pointer into the backing words, a bit-offset, and a length.

> [!NOTE]
> Operations on and between bit-spans and other objects in the `gf2` library are implemented using bitwise operations on whole underlying words at a time.
> These operations are highly optimised in modern CPUs, allowing for fast computation even on large bitsets.
> It also means we never have to worry about overflows or carries as we would with normal integer arithmetic.

Use spans when you need fast, zero-copy views for algorithms (e.g., matrix row slices, polynomial segments, or temporary windows) without giving up the rich `gf2::BitStore` operations.

Because they are non-owning you can pass spans by value without worrying about copies.

This is similar in spirit to [`std::span`] for arrays of regular types, but specialised for bit-level access and manipulation.

> [!WARNING]
> Because a `gf2::BitSpan` keeps a pointer back to the underlying `gf2::BitStore`, that store must outlive the span.
> Outside of that lifetime guarantee, you can freely pass bit spans by value -- they are tiny and intentionally lightweight.

## Declaration

The declaration of the bit-span class looks like:

```c++
template <Unsigned Word>
class BitSpan {
public:
    using word_type = Word;
    // ...
};
```

In the case of a a bit-span, the const-ness of the `Word` type determines whether the span is read-only or read-write.
A span created using `gf2::BitStore::span(begin, end) const` will have `Word` as a `const` type, while one created using `gf2::BitStore::span(begin, end)` will have a non-const `Word` type.
The former is a read-only view, while the latter allows mutation.

> [!NOTE]
> Spans are lightweight and you generally pass them by value so `fun(BitSpan<Word> span)` is preferred over `fun(BitSpan<Word> const& span)`.
> This form also emphasises that what matters is the _interior_ const-ness of the span, not whether the span object itself is const.

## Methods Overview

The `gf2::BitSpan` class provides a rich set of methods for querying and manipulating, bit-spans.
Here is an overview of the main methods available in the class:

| Category                                      | Description                                                                       |
| --------------------------------------------- | --------------------------------------------------------------------------------- |
| [Concept Methods](#span-concept)              | Methods needed to satisfy the `gf2::BitStore` concept.                            |
| [Constructors](#span-constructors)            | Methods to create bit-spans.                                                      |
| [Bit Access](#span-bit-access)                | Methods to access individual bit elements in a bit-span.                          |
| [Queries](#span-queries)                      | Methods to query the overall state of a bit-span.                                 |
| [Mutators](#span-mutators)                    | Methods to mutate the overall state of a bit-span.                                |
| [Fills](#span-fills)                          | Methods to fill a bit-span from various sources.                                  |
| [Spans](#span-span)                           | Methods to create non-owning views over a part of a bit-span --- _sib-bit-spans_. |
| [Sub-vectors](#span-sub-vectors)              | Methods to pull out a clone of piece of a bit-span as new bit-vector.             |
| [Riffling](#span-riffling)                    | Methods to create bit-vectors that copy a bit-span with interleaved zeros.        |
| [Set/Unset Indices](#span-indices)            | Methods to find the indices of set & unset bits in a bit-span.                    |
| [Iterators](#span-iterators)                  | Methods to create various iterators over a bit-span.                              |
| [Stringification](#span-stringification)      | Methods to create string representations of a bit-span.                           |
| [Equality Operator](#equality-operator)       | Operator to compare bit-stores, including bit-spans for content equality.         |
| [Bit Shifts](#bit-shifts)                     | Operators to shift the bits in bit-spans left or right.                           |
| [Bit-wise Operators](#bit-wise-operators)     | Operators to combine bit-stores using logical operations.                         |
| [Arithmetic Operators](#arithmetic-operators) | Operators to add or subtract bit-stores.                                          |
| [Other Functions](#other-functions)           | Dot products, convolutions, concatenation etc. for bit-spans.                     |

## Concept Methods {#span-concept}

Bit-spans satisfy the `gf2::BitStore` concept and forwards many method calls to free functions defined for that concept.
The concept requires us to provide the following methods:

| Method Name                 | Description                                                             |
| --------------------------- | ----------------------------------------------------------------------- |
| `gf2::BitSpan::size`        | Returns the number of bit elements in the span.                         |
| `gf2::BitSpan::words`       | Returns the _minimum_ number of `Word`s needed to store those elements. |
| `gf2::BitSpan::word`        | Returns the "word" for the passed index.                                |
| `gf2::BitSpan::set_word`    | Sets the "word" at the passed index to the passed value.                |
| `gf2::BitSpan::store const` | Returns a const pointer to the beginning of the underlying store.       |
| `gf2::BitSpan::store`       | Returns a non-const pointer to the beginning of the underlying store.   |
| `gf2::BitSpan::offset`      | The bit-span begins at this bit-offset inside its first word.           |

These methods were trivial to implement for bit-arrays and vectors but require some thought for bit spans.

The key thing to understand is that all functions operating on a `gf2::BitStore` operates _as if_ the span is a contiguous array of bits starting at bit-index **0**.

Of course, the actual bits in the span may start part-way through a word in the underlying store, so we have to adjust for that in our implementation.

For a bit-span, the return value from the `gf2::BitSpan::word(i)` method will often be synthesised from two contiguous "real" words `w[j]` and `w[j+1]` for some `j`:

`word[i]` will use some high-order bits from `w[j]` and low-order bits from `w[j+1]` as shown in the following example:

![A bit-span with 20 elements, where the `X` is always zero](docs/images/bit-span.svg)

The `BitSpan` class behaves _as if_ bits from the real underlying store were copied and shuffled down so that element zero is bit 0 of word 0 in the bit-span. However, it never actually copies anything; instead, it synthesises "words" as needed.

The same principle applies to the `gf2::BitSpan::set_word(i, value)` method.
In the case of a bit-span, calls to `set_word(i, value)` will generally copy low-order bits from `value` into the high-order bits of some real underlying word `w[j]` and copy the rest of the high-order bits from `value` into the low-order bits of `w[j+1]`.

The other bits in `w[j]` and `w[j+1]` will not be touched.

> [!WARNING]
> While the `gf2::BitSpan::store` method provides write access to the underlying words, this is primarily for internal use.
> If you do use the pointer, you must ensure that any bits outside the span's range remain unaltered.
> The `set_word` method takes care of this for you, so prefer using that method when possible.

## Constructors {#span-constructors}

The `gf2::BitSpan` class provides just one constructor to create bit-span objects:

```c++
BitSpan(Word* data, size_t offset, size_t length);
```

This constructor creates a bit-span that views `length` bits starting at bit-offset `offset` inside the array of words pointed to by `data`.
If the `Word` type is `const`, the span will be read-only; otherwise, it will allow mutation.

However, you don't usually construct `BitSpan` objects directly.
Instead, you typically obtain them from existing `gf2::BitStore` objects using their `span` member functions:

-   `gf2::BitXXX::span(begin, end) const` creates a bit-span as a read-only view of the bits in the half open range `[begin, end)`.
-   `gf2::BitXXX::span(begin, end)` creates a bit-span as a read-write view of the bits in the half open range `[begin, end)`.

Here `BitXXX` can be any class that satisfies the `gf2::BitStore` concept, such as `gf2::BitVec`, `gf2::BitArray`, or even `gf2::BitSpan` as taking a span of a span is supported.

Those methods use the generic `gf2::span` method defined for the `gf2::BitStore` concept as described [here](BitStore.md#store-spans) documentation.

Spans can start in the middle of a machine word and can end part-way through another word; the class synthesises whole-word reads/writes for you.

> [!NOTE]
> Because a `gf2::BitSpan` keeps a pointer back to the underlying word store, that store must outlive the span.
> In the Rust version of the library, this is enforced using lifetime parameters.
> In C++, this is just a convention the user must follow.
> We could use smart pointers to enforce this at runtime, but that would add overhead and complexity to the concrete `gf2::BitStore` types.

### Example

```cpp
auto v = gf2::BitVec<u8>::from_string("1111'1111'1111").value();
v.span(2, 6).flip_all();
assert_eq(v.to_string(), "110000111111");
```

## Bit Access {#span-bit-access}

The following methods provide access to individual bit elements in the bit-span.

| Function                     | Description                                                                                                         |
| ---------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| `gf2::BitSpan::get`          | Returns the value of a single bit element as a read-only boolean.                                                   |
| `gf2::BitSpan::operator[]()` | Returns a `bool` in the const case and a `BitRef` with read-write access to a bit element in the non-const version. |
| `gf2::BitSpan::front`        | Returns the value of the first element in the bit-span.                                                             |
| `gf2::BitSpan::back`         | Returns the value of the last element in the bit-span.                                                              |
| `gf2::BitSpan::set`          | Sets a bit to the given boolean value which defaults to `true`.                                                     |
| `gf2::BitSpan::flip`         | Flips the value of the bit element at a given index.                                                                |
| `gf2::BitSpan::swap`         | Swaps the values of bit elements at locations `i` and `j`.                                                          |

> [!NOTE]
> You can set the `DEBUG` flag at compile time to enable bounds checks on the index arguments.

The non-const version of `gf2::BitSpan::operator[]()` returns a `gf2::BitRef`, which is "reference" to an individual bit in the vector.
It is automatically converted to a boolean on reads, but it also allows writes, which means you can write natural-looking single-bit assignments:

```c++
s[12] = true;
```

## Queries {#span-queries}

The following methods let you query the overall state of a bit-span.

| Function                       | Description                                                |
| ------------------------------ | ---------------------------------------------------------- |
| `gf2::BitSpan::is_empty`       | Returns true if the bit-span is empty                      |
| `gf2::BitSpan::any`            | Returns true if _any_ bit in the bit-span is set.          |
| `gf2::BitSpan::all`            | Returns true if _every_ bit in the bit-span is set.        |
| `gf2::BitSpan::none`           | Returns true if _no_ bit in the bit-span is set.           |
| `gf2::BitSpan::count_ones`     | Returns the number of set bits in the bit-span.            |
| `gf2::BitSpan::count_zeros`    | Returns the number of unset bits in the bit-span.          |
| `gf2::BitSpan::leading_zeros`  | Returns the number of leading unset bits in the bit-span.  |
| `gf2::BitSpan::trailing_zeros` | Returns the number of trailing unset bits in the bit-span. |

These methods efficiently operate on words at a time, so they are inherently parallel.

## Mutators {#span-mutators}

The following methods let you mutate the entire bit-span in a single call.

| Function                 | Description                                                                  |
| ------------------------ | ---------------------------------------------------------------------------- |
| `gf2::BitSpan::set_all`  | Sets all the bits in the span to the passed value, which defaults to `true`. |
| `gf2::BitSpan::flip_all` | Flips the values of all the bits in the span.                                |

The methods operate on words at a time, so are inherently parallel.

## Fills {#span-fills}

The following methods let you populate the entire bit-span in a single call.

| Function                    | Description                                                          |
| --------------------------- | -------------------------------------------------------------------- |
| `gf2::BitSpan::copy`        | Makes the bits in this span identical to those from various sources. |
| `gf2::BitSpan::fill_random` | Fills the span with random 0's and 1's.                              |

### Copies

The `gf2::BitSpan::copy` methods support copying bits from:

-   Another bit-store of the same size but possibly a different underlying word type.
-   A [`std::bitset`] of the same size as the bit-span.
-   An unsigned integer that has the same number of bits as the span. The integer type need not be the same as the underlying `Word` used by the bit-span.
-   A function or callable object that takes a single `usize` index argument and returns a boolean value for that index.

> [!NOTE]
> In each case, the size of the source and destinations must match exactly and that condition is always checked unless the `NDEBUG` flag is set at compile time. You can always use a `gf2::BitSpan` sub-span to copy a subset of bits if needed.

### Random Fills

By default, the random fill method uses a random number generator seeded with system entropy, so the results change from run to run.
You can set a specific seed to get reproducible fills.

The default probability that a bit is set is 50%, but you can pass a different probability in the range `[0.0, 1.0]` if desired.

## Spans {#span-span}

The following methods let you create a `gf2::BitSpan` from a bit-span --- a _sub-bit-span_.

| Function             | Description                                                                         |
| -------------------- | ----------------------------------------------------------------------------------- |
| `gf2::BitSpan::span` | Returns a `gf2::BitSpan` encompassing the bits in a half-open range `[begin, end)`. |

There are two overloads of the `gf2::BitSpan::span` method --- one for `const` bit-spans and one for non-`const` bit-spans:

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

## Sub-vectors {#span-sub-vectors}

The following methods create or fill _independent_ bit-vectors with copies of some contiguous subset of the bits in the vector.

| Function                 | Description                                                                            |
| ------------------------ | -------------------------------------------------------------------------------------- |
| `gf2::BitSpan::sub`      | Returns a new `gf2::BitVec` encompassing the bits in a half-open range `[begin, end)`. |
| `gf2::BitSpan::split_at` | Fills two bit-vectors with the bits in the ranges `[0, at)` and `[at, size())`.        |

The `gf2::BitSpan::split_at` method can optionally take two pre-existing bit-vectors to fill, thereby avoiding unnecessary allocations in some iterative algorithms that repeatedly use this method.

> [!NOTE]
> These methods do not alter the underlying vector.

## Riffling {#span-riffling}

We have methods that can interleave (_riffle_) the bits in a span with zeros.

| Function                | Description                                                                          |
| ----------------------- | ------------------------------------------------------------------------------------ |
| `gf2::BitSpan::riffled` | Fills a pre-existing bit-vector with the result of riffling this bit-span.           |
| `gf2::BitSpan::riffled` | Returns a new bit-vector that is this bit-span with its bits interleaved with zeros. |

If the bit-span looks like $v_0 v_1 v_2 \ldots v_n$, then the riffling operation produces the vector $v_0 0 v_1 0 v_2 0 \ldots v_n$ where a zero is interleaved _between_ every bit in the original bit-span (there is no trailing zero at the end).

If you think of a bit-span as representing the coefficients of a polynomial over GF(2), then riffling corresponds to squaring that polynomial. See the documentation for `gf2::BitPoly::squared` for more information.

## Set/Unset Bit Indices {#span-indices}

The following methods find the indices of set or unset bits in the vector.

| Function                       | Description                                                                            |
| ------------------------------ | -------------------------------------------------------------------------------------- |
| `gf2::BitSpan::first_set`      | Returns the index of the first set bit in the bit-span.                                |
| `gf2::BitSpan::last_set`       | Returns the index of the last set bit in the bit-span.                                 |
| `gf2::BitSpan::next_set`       | Returns the index of the next set bit in the bit-span _after_ the passed index.        |
| `gf2::BitSpan::previous_set`   | Returns the index of the previous set bit in the bit-span _before_ the passed index.   |
| `gf2::BitSpan::first_unset`    | Returns the index of the first unset bit in the bit-span.                              |
| `gf2::BitSpan::last_unset`     | Returns the index of the last unset bit in the bit-span.                               |
| `gf2::BitSpan::next_unset`     | Returns the index of the next unset bit in the bit-span _after_ the passed index.      |
| `gf2::BitSpan::previous_unset` | Returns the index of the previous unset bit in the bit-span _before_ the passed index. |

## Iterators {#span-iterators}

The following methods create iterators for traversing the bits or underlying words in the bit-span:

-   Read-only iteration through the individual bits.
-   Read-write iteration through the individual bits.
-   Read-only iteration through the indices of the set bits.
-   Read-only iteration through the indices of the unset bits.
-   Read-write iteration through the underlying vector words.

| Function                    | Description                                                                    |
| --------------------------- | ------------------------------------------------------------------------------ |
| `gf2::BitSpan::bits`        | Returns a `gf2::Bits` iterator over the bits in the bit-span.                  |
| `gf2::BitSpan::set_bits`    | Returns a `gf2::SetBits` iterator to view the indices of all the set bits.     |
| `gf2::BitSpan::unset_bits`  | Returns a `gf2::UnsetBits` iterator to view the indices of all the unset bits. |
| `gf2::BitSpan::store_words` | Returns a `gf2::Words` iterator to view the "words" underlying the bit-span.   |
| `gf2::BitSpan::to_words`    | Returns a copy of the "words" underlying the bit-span.                         |

There are two overloads of the `gf2::BitSpan::bits` method --- one for `const` bit-stores and one for non-`const` bit-stores:

```c++
auto bits();        // <1>
auto bits() const;  // <2>
```

1. Returns a mutable `gf2::Bits` that allows modification of the bits in the bit-span.
2. Returns an immutable `gf2::Bits` that only allows one to view the bits in the bit-span.

## Stringification {#span-stringification}

The following methods return a string representation for a bit-span.
The string can be in the obvious binary format or a more compact hex format.

| Function                         | Description                                                 |
| -------------------------------- | ----------------------------------------------------------- |
| `gf2::BitSpan::to_string`        | Returns a default string representation for a bit-span.     |
| `gf2::BitSpan::to_pretty_string` | Returns a "pretty" string representation for a bit-span.    |
| `gf2::BitSpan::to_binary_string` | Returns a binary string representation for a bit-span.      |
| `gf2::BitSpan::to_hex_string`    | Returns a compact hex string representation for a bit-span. |

## Other Operators and Functions

There are many operators and free functions defined for any `gf2::BitStore` compatible class, including:

| Category                                      | Description                                                               |
| --------------------------------------------- | ------------------------------------------------------------------------- |
| [Equality Operator](#equality-operator)       | Operator to compare bit-stores, including bit-spans for content equality. |
| [Bit Shifts](#bit-shifts)                     | Operators to shift the bits in bit-spans left or right.                   |
| [Bit-wise Operators](#bit-wise-operators)     | Operators to combine bit-stores using logical operations.                 |
| [Arithmetic Operators](#arithmetic-operators) | Operators to add or subtract bit-stores.                                  |
| [Other Functions](#other-functions)           | Dot products, convolutions, concatenation etc. for bit-spans.             |

## See Also

-   `gf2::BitSpan` for detailed documentation of all class methods.
-   [`BitStore`](BitStore.md) for the common API shared by all bit-stores.
-   [`BitArray`](BitArray.md) for fixed-size vectors of bits.
-   [`BitVec`](BitVec.md) for dynamically-sized vectors of bits.
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
