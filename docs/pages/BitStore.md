# The `BitStore` Concept

## Introduction

The library's vector-like types satisfy the `BitStore` [concept]:

| Type             | Description                                                                       |
| ---------------- | --------------------------------------------------------------------------------- |
| `gf2::BitArray`  | A fixed-size array of bits packed into a `std::array` of unsigned words.          |
| `gf2::BitVector` | A dynamically sized vector of bits packed into a `std::vector` of unsigned words. |
| `gf2::BitSpan`   | A non-owning _view_ of some span of bits held by one of the two previous types.   |

These types own or view individual bit elements packed into some underlying "store" of `Unsigned` words.
The particular choice of `Word` is generic and user selectable from one of the primitive unsigned integer types.
We refer to any type that implements the `BitStore` trait as a _bit-store_.

Bit-stores have _dozens_ of methods in common.
That `BitStore` concept defines the requirements for implementing the shared functionality _once_ as associated methods of the trait.
Each concrete bit-store type forwards many methods to the equivalent free function that operates on generic `gf2::BitStore` arguments.

The functions include bit accessors, mutators, fills, queries, iterators, stringification methods, bit-wise operators, arithmetic operators, and more. Operations on and between bit-stores work on a whole-word basis, so are inherently parallel.

> [!NOTE]
> For convenience, the free functions that work on bit-stores are also pulled directly into the three vector-like classes as _member functions_.
> For example, if `v` is a bit-vector, you can call `v.count_ones()` to count the number of set bits in `v` instead of calling the free function `gf2::count_ones(v)` though both forms are valid.

## Concept Requirements

Bit-stores all own, or view, a store of bits packed into contiguous "words", where a word is some `gf2::Unsigned`.
Operations on and between bit-stores work a whole word at a time, so are inherently parallel.
The particular unsigned integer type used to hold the bits must be exposed as a class-level typename.

| Required Type | Description                                                                         |
| ------------- | ----------------------------------------------------------------------------------- |
| `word_type`   | The bit-store holds or views bits in contiguous words of this `gf2::Unsigned` type. |

To satisfy the `gf2::BitStore` concept, a class should also define the following seven methods:

| Required Method | Return Type        | Expected Return Value/Method Functionality                |
| --------------- | ------------------ | --------------------------------------------------------- |
| `size`          | `usize`            | The number of accessible bits in the object.              |
| `store const`   | `word_type const*` | A const pointer to the first real underlying word.        |
| `store`         | `word_type*`       | A non-const pointer to the first real underlying word.    |
| `offset`        | `u8`               | The offset in bits to the first element in `store()[0]`.  |
| `words`         | `usize`            | The _minimum_ number of words needed to store those bits. |
| `word(i)`       | `word_type`        | A copy of "word" number `i`.                              |
| `set_word(i,v)` | `void`             | Set the accessible bits in "word" `i` to those from `v`.  |

> [!NOTE]
> We could implement the last three methods above using the first four.
> For example, the `words` method is a trivial computation based on `size` and the number of bits per underlying word.
> However, all the concrete bit-store types already cache the required value, so we use that instead.
> Every hot loop in the library calls `words()`, and benchmarking shows that precomputing the value _significantly_ improves performance.
> Having optimised versions of the `word(i)` and `set_word(i,v)` methods has an even larger impact on performance.

### Other Notes

- The underlying store must contain enough words of storage to accommodate `size` bits.
- The `words` method always returns the same number as `words_needed<Word>(size())` but cached as this value is in constant use.
- The store's final word can have extra unused bits, but the `word` method should always set those unused bits to zero.
- The `set_word` method sets a "word" to a passed value, being careful to only have an effect on _accessible_ bits in the store.

### Example

The methods are trivial to implement for `gf2::BitArray` and `gf2::BitVector`.

Here is a sketch of how they might work for the `gf2::BitVector` class which stores `m_size` bits in a `std::vector<Word>` called `m_store`:

```c++
template <Unsigned Word = usize>                                    // <1>
class BitVector {
private:
    usize m_size;
    std::vector<Word> m_store;
public:
    using word_type = Word;                                         // <2>

    constexpr usize size() const        { return m_size; }          // <3>
    constexpr Word const* store() const { return m_store.data(); }
    constexpr Word* store()             { return m_store.data(); }
    constexpr u8 offset() const         { return 0; }
    constexpr usize words() const       { return m_store.size(); }
    constexpr Word word(usize i) const  { return m_store[i]; }
    constexpr void set_word(usize i, Word v) { m_store[i] = v; }    // <4>
};
```

1. The template parameter `Word` is the unsigned integer type used to store the bits in the bit-vector. It defaults to `usize`, which is typically the most efficient type for the target platform.
2. The `BitStore` concept requires a `word_type` typename.
3. The required `BitStore` methods are all trivially implemented, though the real implementation allows for range checks if the compile-time `DEBUG` flag is present.
4. In this simple sketch, the `set_word` method directly sets the underlying word. The real implementation is careful to avoid touching unoccupied bits in the final word.

A sketch of the `BitArray` class is similar, except that the underlying store is a `std::array` of words rather than a `std::vector`.

### Bit-spans

The `gf2::BitSpan` class is a bit different because it is a non-owning view into some contiguous subset of bits held by another bit-store, and that subset may not align with the underlying words.

However, all `BitStore` related functions operate _as if_ bit element 0 in the store is the **lowest-order** bit of "word" **0**.
This constraint means that the implementation of the `word(i)` and `set_word(i,v)` methods for `gf2::BitSpan` is more complex than for the other two bit-stores because they often have to synthesise words from two underlying words in the real store.

#### Sample Layout

Consider a bit-store `store` with 20 elements, where the `Word` type used to store those bits is an unsigned 8-bit integer.

The `BitStore` functions all naturally expect that `store.size()` returns 20.
Less obviously, they all expect `store.words()` to return 3, as it takes three 8-bit words to hold 20 bits with four bits to spare.

The functions expect that `store.word(0)` holds the first 8 bits in the bit-store, `store.word(1)` has the following 8 bits, and `store.word(2)` holds the final four elements in its four lowest order bits.
It also expects that the four highest "unoccupied" bits in `store.word(2)` are set to 0.

If the store is a bit-array or a bit-vector, the implementation of these `BitStore` expectations is easy.
Those classes just have to be careful to ensure that any unoccupied high-order bits in the final word remain zeros.

It is a different matter for a bit-span, which isn't usually zero-aligned with the real underlying array of unsigned words, `w[0]`, `w[1]`, ...
The various bit-store functions still expect that `store.words()` returns three even though the span may touch bits in _four_ underlying words!

For a bit-span, the return value for `store.word(i)` will often be synthesised from two contiguous "real" words
`w[j]` and `w[j+1]` for some `j`.
`store.word[i]` will use some high-order bits from `w[j]` and low-order bits from `w[j+1]`.

The following diagram shows how bits in a bit-slice lie within the underlying words, which are `u8`s in this example:

![A bit-span with 20 elements, where the `X` is always zero](docs/images/bit-span.svg)

The `BitSpan` class must always behave _as if_ bits from the real underlying store were copied and shuffled down so that element zero is bit 0 of word 0 in the bit-span. However, it never actually copies anything; instead, it synthesises "words" as needed.

The same principle applies to the `store.set_word(i, value)` method.
The implementation of `set_word` for bit-vectors and bit-arrays is trivial, with the one caveat that we have to be careful not to inadvertently touch any unoccupied bits in the final underlying word, or at least be sure to leave them as zeros.

In the case of a bit-span, calls to `set_word(i, value)` will generally copy low-order bits from `value` into the high-order bits of some real underlying word `w[j]` and copy the rest of the high-order bits from `value` into the low-order bits of `w[j+1]`. The other bits in `w[j]` and `w[j+1]` will not be touched.

## Functions Overview

We provide dozens of useful free functions in the `gf2` namespace that operate on any bit store.

The first argument to each function is a reference to a type that satisfies the `gf2::BitStore` concept, for example, the following function:

```c++
template <BitStore Store>
void set_all(Store& store, bool value = true) { ... }
```

sets all the bits in the passed `store` to the given `value`.

The provided functions fall into categories:

| Category                                      | Description                                                                      |
| --------------------------------------------- | -------------------------------------------------------------------------------- |
| [Bit Access](#store-bit-access)               | Functions to access individual bit elements in a bit-store.                      |
| [Queries](#store-queries)                     | Functions to query the overall state of a bit-store.                             |
| [Mutators](#store-mutators)                   | Functions to mutate the overall state of a bit-store.                            |
| [Fills](#store-fills)                         | Functions to fill a bit-store from various sources.                              |
| [Spans](#store-spans)                         | Functions to create non-owning views over a part of a bit-store --- _bit-spans_. |
| [Sub-vectors](#store-sub-vectors)             | Functions to clone a piece of a bit-store as a new bit-vector.                   |
| [Riffling](#store-riffling)                   | Functions to create vectors that copy a bit-store with interleaved zeros.        |
| [Set/Unset Indices](#store-indices)           | Functions to find the indices of set & unset bits in a bit-store.                |
| [Iterators](#store-iterators)                 | Functions to create various iterators over a bit-store.                          |
| [Stringification](#store-stringification)     | Functions to create string representations of a bit-store.                       |
| [Equality Operator](#equality-operator)       | Operator to compare two bit-stores for equality.                                 |
| [Bit Shifts](#bit-shifts)                     | Operators to shift the bits in a bit-store left or right.                        |
| [Bit-wise Operators](#bit-wise-operators)     | Operators to combine two bit-stores using logical operations.                    |
| [Arithmetic Operators](#arithmetic-operators) | Operators to add or subtract two bit-stores.                                     |
| [Other Functions](#other-functions)           | Dot products, convolutions, concatenation, etc. for bit-stores.                  |

## Bit Access {#store-bit-access}

The following functions provide access to individual bit elements in the bit-store.

| Function     | Description                                                        |
| ------------ | ------------------------------------------------------------------ |
| `gf2::get`   | Returns the value of a single bit element as a read-only boolean.  |
| `gf2::ref`   | Returns a `BitRef` with read-write access to a single bit element. |
| `gf2::front` | Returns the value of the first element in the store.               |
| `gf2::back`  | Returns the value of the last element in the store.                |
| `gf2::set`   | Sets a bit to the given boolean value, which defaults to `true`.   |
| `gf2::flip`  | Flips the value of the bit element at a given index.               |
| `gf2::swap`  | Swaps the values of bit elements at locations `i` and `j`.         |

> [!NOTE]
> You can set the `DEBUG` flag at compile time to enable bounds checks on the index arguments.

The `ref(store, i)` function returns a `gf2::BitRef`, which is a "reference" to an individual bit in the store.
It is automatically converted to a boolean on reads, but it also allows writes.

This is equivalent to calling `set(v, 12, true);`.

## Queries {#store-queries}

The following functions let you query the overall state of a bit-store.

| Function              | Description                                             |
| --------------------- | ------------------------------------------------------- |
| `gf2::is_empty`       | Returns true if the store is empty                      |
| `gf2::any`            | Returns true if _any_ bit in the store is set.          |
| `gf2::all`            | Returns true if _every_ bit in the store is set.        |
| `gf2::none`           | Returns true if _no_ bit in the store is set.           |
| `gf2::count_ones`     | Returns the number of set bits in the store.            |
| `gf2::count_zeros`    | Returns the number of unset bits in the store.          |
| `gf2::leading_zeros`  | Returns the number of leading unset bits in the store.  |
| `gf2::trailing_zeros` | Returns the number of trailing unset bits in the store. |

These methods efficiently operate on words at a time, so they are inherently parallel.

## Mutators {#store-mutators}

The following functions let you mutate the entire store in a single call.

| Function        | Description                                                                   |
| --------------- | ----------------------------------------------------------------------------- |
| `gf2::set_all`  | Sets all the bits in the store to the passed value, which defaults to `true`. |
| `gf2::flip_all` | Flips the values of all the bits in the store.                                |

They efficiently operate on words at a time, so they are inherently parallel.

## Copies & Fills {#store-fills}

The following functions let you populate the entire store from multiple sources in a single call.

| Function           | Description                                           |
| ------------------ | ----------------------------------------------------- |
| `gf2::copy`        | Copies bit values from various sources to this store. |
| `gf2::fill_random` | Fills the store with random 0's and 1's.              |

### Copies

The `copy` functions support copying bit values from:

- Another bit-store of the same size but possibly a different underlying word type.
- A [`std::bitset`] of the same size as the store.
- An unsigned integer that has the same number of bits as the store. The integer type need not be the same as the underlying `Word` used by the bit-vector.
- A function or callable object that takes a single `usize` index argument and returns a boolean value for that index.

> [!NOTE]
> In each case, the _size_ of the source and destinations must match exactly and that condition is always checked unless the `NDEBUG` flag is set at compile time. You can always use a `gf2::BitSpan` to copy a subset of bits if needed. However, the underlying _word types_ need **not** match, so you can copy between bit-stores that use different underlying word types. You can use the `gf2::copy` method to convert between different `Word` type stores (e.g., from `BitVector<u32>` to `BitVector<u8>`) as long as the size of the source and destinations match.

### Random Fills

By default, the random fill method uses a random number generator seeded with system entropy, so the results change from run to run.
You can set a specific seed to get reproducible fills.

The default probability that a bit is set is 50%, but you can pass a different probability in the range `[0.0, 1.0]` if desired.

## Spans {#store-spans}

The following functions let you create a `gf2::BitSpan`, which is a non-owning view of some contiguous subset of bits in the store.

| Function    | Description                                                                         |
| ----------- | ----------------------------------------------------------------------------------- |
| `gf2::span` | Returns a `gf2::BitSpan` encompassing the bits in a half-open range `[begin, end)`. |

There are two overloads of the `gf2::span` function --- one for `const` bit-stores and one for non-`const` bit-stores:

```c++
template <BitStore Store>
auto span(Store& store, usize begin, usize end); // <1>

template <BitStore Store>
auto span(Store const& store, usize begin, usize end); // <2>
```

1. Returns a mutable `gf2::BitSpan` that allows modification of the bits in the specified range.
2. Returns an immutable `gf2::BitSpan` that does not allow modification of the bits in the specified range.

In both cases, the `begin` and `end` arguments define a half-open range of bits in the store.

Mutability/immutability of the returned `BitSpan` is _deep_.
The span's mutability reflects that of the underlying store, so if the store is mutable, so is the span, and vice versa.

This is similar to the C++20 [`std::span`] class for regular data collection types.

> [!NOTE]
> A `gf2::BitSpan` also satisfies the `gf2::BitStore` concept, so you can take a span of a span.

## Sub-vectors {#store-sub-vectors}

The following functions create or fill _independent_ bit-vectors with copies of some contiguous subset of the bits in the store.

| Function     | Description                                                                               |
| ------------ | ----------------------------------------------------------------------------------------- |
| `gf2::sub`   | Returns a new `gf2::BitVector` encompassing the bits in a half-open range `[begin, end)`. |
| `gf2::split` | Fills two bit-vectors with the bits in the ranges `[0, at)` and `[at, size())`.           |

The `split_` method can optionally take two pre-existing bit-vectors to fill, thereby avoiding unnecessary allocations in some iterative algorithms that repeatedly use this method.

> [!NOTE]
> These methods do not alter the underlying store.

## Riffling {#store-riffling}

We have functions that can interleave (_riffle_) the bits in a store with zeros.

| Function      | Description                                                                       |
| ------------- | --------------------------------------------------------------------------------- |
| `gf2::riffle` | Fills a pre-existing bit-vector with the result of riffling this store.           |
| `gf2::riffle` | Returns a new bit-vector that is this store with its bits interleaved with zeros. |

If the store looks like $v_0 v_1 v_2 \ldots v_n$, then the riffling operation produces the vector $v_0 0 v_1 0 v_2 0 \ldots v_n$ where a zero is interleaved _between_ every bit in the original store (there is no trailing zero at the end).

If you think of a bit-store as representing the coefficients of a polynomial over GF(2), then riffling corresponds to squaring that polynomial. See the documentation for `gf2::BitPolynomial::squared` for more information.

## Set/Unset Bit Indices {#store-indices}

The following functions find the indices of set or unset bits in the store.

| Function              | Description                                                                         |
| --------------------- | ----------------------------------------------------------------------------------- |
| `gf2::first_set`      | Returns the index of the first set bit in the store.                                |
| `gf2::last_set`       | Returns the index of the last set bit in the store.                                 |
| `gf2::next_set`       | Returns the index of the next set bit in the store _after_ the passed index.        |
| `gf2::previous_set`   | Returns the index of the previous set bit in the store _before_ the passed index.   |
| `gf2::first_unset`    | Returns the index of the first unset bit in the store.                              |
| `gf2::last_unset`     | Returns the index of the last unset bit in the store.                               |
| `gf2::next_unset`     | Returns the index of the next unset bit in the store _after_ the passed index.      |
| `gf2::previous_unset` | Returns the index of the previous unset bit in the store _before_ the passed index. |

## Iterators {#store-iterators}

The following functions create iterators for traversing the bits or underlying words in the store:

- Read-only iteration through the individual bits.
- Read-write iteration through the individual bits.
- Read-only iteration through the indices of the set bits.
- Read-only iteration through the indices of the unset bits.
- Read-write iteration through the underlying store words.

| Function           | Description                                                                    |
| ------------------ | ------------------------------------------------------------------------------ |
| `gf2::bits`        | Returns a `gf2::Bits` iterator over the bits in the store.                     |
| `gf2::set_bits`    | Returns a `gf2::SetBits` iterator to view the indices of all the set bits.     |
| `gf2::unset_bits`  | Returns a `gf2::UnsetBits` iterator to view the indices of all the unset bits. |
| `gf2::store_words` | Returns a `gf2::Words` iterator to view the "words" underlying the store.      |
| `gf2::to_words`    | Returns a copy of the "words" underlying the bit-store.                        |

There are two overloads of the `gf2::bits` function --- one for `const` bit-stores and one for non-`const` bit-stores:

```c++
template <BitStore Store>
auto bits(Store& store); // <1>

template <BitStore Store>
auto bits(Store const& store); // <2>
```

1. Returns a mutable `gf2::Bits` that allows modification of the bits in the store.
2. Returns an immutable `gf2::Bits` that only allows one to view the bits in the store.

## Stringification {#store-stringification}

The following functions returns a string representation of a bit store.
The string can be in the obvious binary format or a more compact hex format.

| Function                                 | Description                                                    |
| ---------------------------------------- | -------------------------------------------------------------- |
| `gf2::to_string`                         | Returns a default string representation for a bit-store.       |
| `gf2::to_pretty_string`                  | Returns a "pretty" string representation for a bit-store.      |
| `gf2::to_binary_string`                  | Returns a binary string representation for a bit-store.        |
| `gf2::to_hex_string`                     | Returns a compact hex string representation for a bit-store.   |
| `operator<<(Store const&,std::ostream&)` | The usual output stream output stream operator for bit-stores. |
| `struct std::formatter<gf2::BitStore>`   | Specialisation of [`std::formatter`] for bit-stores.           |

## String Encodings

A bit-store has two different string representations: as a binary string or as a compact hex string.
The two encodings are:

### Binary String Encoding

The straightforward character encoding for a bit-store is a _binary_ string containing just 0's and 1's, for example, `"10101"`.
Each character in a binary string represents a single element in the store.
The `to_binary_string` method produces this string.
The method allows for an optional prefix, suffix, and separator between each bit.

The `to_string` calls `to_binary_string` to produce the most compact output, e.g. `"10101"`.
The `to_pretty_string` method produces a more human-friendly version, e.g., `"[1 0 1 0 1]"`.

The format used by the output stream operator is the same as that used by `to_string`.
That is also the default format used by the `std::formatter` specialisation.
However, you can use a `:p` format specifier to get the "pretty" version instead.
For example, if `std::format("{}, v)` is `"1010101010"`, then `std::format("{:p}", v)` is `"[1 0 1 0 1 0 1 0 1 0]"`.

> [!NOTE]
> The output is in _vector order_ `v[0] v[1] v[2] ...` with the first element in the vector on the left.
> This is in contrast to a [`std::bitset`] which puts its lowest indexed element on the right.

### Hex String Encoding

The other supported encoding for bit-stores is a compact hex-type string containing just the 16 hex characters `0123456789ABCDEF`.
For example, the string `"3ED02"`.
We allow for hex strings with an optional prefix `"0x"` or `"0X"`, for example `"0x3ED02"`.

Each hex character translates to _four_ elements in a `BitStore`.
The hex string `0x0` is equivalent to the binary string `0000`, and so on, up to the string `0xF`, which is equivalent to the binary string `1111`.

The hex pair `0x0F` will be interpreted in the store as the eight-bit value `00001111`.
Of course, this is the advantage of hex.
It is a more compact format that occupies a quarter of the space needed to write out the equivalent binary string.

However, what happens if you want to encode a vector whose size is _not_ a multiple of 4?
We handle that by allowing the final character in the string to have a base that is _not_ 16.
To accomplish that, we allow for an optional _suffix_, which must be one of `.2`, `.4`, or `.8`.
If present, the suffix gives the base for just the _preceding_ character in the otherwise hex-based string.
If there is no suffix, the final character is assumed to be hex-encoded, as with all the others.

Therefore, the string `0x1` (without a suffix, so the last character is the default hexadecimal base 16) is equivalent to `0001`.
On the other hand, the string `0x1.8` (the last character is base 8) is equivalent to `001`.
Similarly, the string `0x1.4` (the last character is base 4) is equivalent to `01,` and finally, the string `0x1.2` (the previous character is base 2) is comparable to `1`

In the string `0x3ED01.8`, the first four characters, `3`, `E`, `D`, and `0`, are interpreted as hex values, and each will translate to four slots in the store.
However, the final 1.8 is parsed as an octal 1, which takes up three slots (001).
Therefore, this store has a size of 19 (i.e., 4 × 4 + 3).

The `std::formatter` specialisation recognises the `:x` format specifier as a request to produce a hex string representation of a bit-store.
For example, if `std::format("{}, v)` is `"1010101010"`, then `std::format("{:x}", v)` is `"AA2.4"`.

## Equality Operator

As you'd expect, equality between two bit-stores tests whether they have identical content.
It **also** requires that the two stores use the same underlying word type.

With a few well-documented exceptions, the library avoids interactions between bit-stores with different word types.
While it is possible to implement cross-type interactions, such as comparing a `BitStore<uint32_t>` with a `BitStore<uint64_t>`, this is not a typical use case.
Supporting such interactions would add complexity and bloat the compiled code with many more template instantiations.

| Function          | Description                                                                                |
| ----------------- | ------------------------------------------------------------------------------------------ |
| `gf2::operator==` | Checks that two stores are using the same underlying word type and have identical content. |

## Bit Shifts

| Function                | Description                                                        |
| ----------------------- | ------------------------------------------------------------------ |
| `gf2::operator<<=`      | Left shifts in-place.                                              |
| `gf2::operator>>=`      | Right shifts in-place.                                             |
| `gf2::operator<< const` | Copies the store to a new bit-vector and left shifts that vector.  |
| `gf2::operator>> const` | Copies the store to a new bit-vector and right shifts that vector. |

These operators act in vector space, so if the vector is $v_0, v_1, \ldots, v_n$, then a right shift produces the vector $0, v_0, v_1, \ldots, v_n-1$ where we have shifted out the last element and shifted in a zero at the start.
Similarly, a left shift produces the vector $v_1, v_2, \ldots, v_n, 0$ where we have shifted out the first element and shifted in a zero at the end.

Contrast this to shifts in bit space, where if a bit container is $b_n, b_{n-1}, \ldots, b_0$, then a right shift produces $0, b_n, b_{n-1}, \ldots, b_1$ and a left shift produces $b_{n-1}, b_{n-2}, \ldots, b_0, 0$.

Essentially, right shifts in vector space correspond to left shifts in bit space, and vice versa.

> [!NOTE]
> Like every method in the library, the shift operators are implemented efficiently to operate on words at a time.

## Bit-wise Operators

We have methods that combine two bit-stores using the logical operations `XOR`, `AND`, and `OR`.

> [!IMPORTANT]
> These methods require that the two bit-stores use the same underlying word type.
> They also require that the left-hand-side and right-hand-side bit-store operands are the same size.
> That precondition is always checked unless the `NDEBUG` flag is set at compile time.
> Interactions between bit-stores with different word types are only possible at the cost of increased code complexity, and are not a common use case.

The methods can act in place, mutating the left-hand side operator: `lhs &= rhs`.
There is also a non-mutating version `result = lhs & rhs`, which returns a new `result` _bit-vector_ in each case.

| Function           | Description                                                                         |
| ------------------ | ----------------------------------------------------------------------------------- |
| `gf2::operator^=`  | In-place `XOR` operation of equal-sized bit-stores: `lhs = lhs ^ rhs`.              |
| `gf2::operator&=`  | In-place `AND` operation of equal-sized bit-stores: `lhs = lhs & rhs`.              |
| `gf2::operator\|=` | In-place `OR` operation of equal-sized bit-stores: `lhs = lhs \| rhs`.              |
| `gf2::operator~`   | Returns a new _bit-vector_ that has the bits all flipped.                           |
| `gf2::operator^`   | Returns the `XOR` of this store with another equal-sized store as a new bit-vector. |
| `gf2::operator&`   | Returns the `AND` of this store with another equal-sized store as a new bit-vector. |
| `gf2::operator\|`  | Returns the `OR` of this store with another equal-sized store as a new bit-vector.  |

## Arithmetic Operators

In GF(2), the arithmetic operators `+` and `-` are both the `XOR` operator.

| Function          | Description                                                                  |
| ----------------- | ---------------------------------------------------------------------------- |
| `gf2::operator+=` | Adds the passed (equal-sized) `rhs` bit-store to this one.                   |
| `gf2::operator-=` | Subtracts the passed (equal-sized) `rhs` bit-store from this one.            |
| `gf2::operator+`  | Adds two equal-sized bit-stores and returns the result as a bit-vector.      |
| `gf2::operator-`  | Subtracts two equal-sized bit-stores and returns the result as a bit-vector. |

The constraints mentioned in the last bit-twiddling section also apply here.

## Other Functions

| Function         | Description                                                         |
| ---------------- | ------------------------------------------------------------------- |
| `gf2::dot`       | Returns the dot product of two equal-sized bit-stores as a boolean. |
| `gf2::operator*` | Returns the dot product of two equal-sized bit-stores as a boolean. |
| `gf2::convolve`  | Returns the convolution of two bit-stores as a new bit-vector.      |
| `gf2::join`      | Concatenates two bit-stores into a new bit-vector.                  |

We have overloaded the `*` operator for pairs of bit-stores to compute the dot product of those stores.

## See Also

- The `gf2::BitStore` reference for detailed documentation with examples for each function.
- [`BitArray`](BitArray.md) for fixed-size vectors of bits.
- [`BitVector`](BitVector.md) for dynamically-sized vectors of bits.
- [`BitSpan`](BitSpan.md) for non-owning views into any bit-store.
- [`BitRef`](BitRef.md) for read-write references to individual bits in a bit-store.
- [`Bits`](Iterators.md) for iterators over the bits in a bit-store.
- [`SetBits`](Iterators.md) for iterators over the indices of set bits in a bit-store.
- [`UnsetBits`](Iterators.md) for iterators over the indices of unset bits in a bit-store.
- [`Words`](Iterators.md) for iterators over the underlying "words" in a bit-store.

<!-- Reference Links -->

[concept]: https://en.cppreference.com/w/cpp/language/constraints
[`std::span`]: https://en.cppreference.com/w/cpp/container/span.html
[`std::formatter`]: https://en.cppreference.com/w/cpp/utility/format/formatter.html
[`std::bitset`]: https://en.cppreference.com/w/cpp/utility/bitset
