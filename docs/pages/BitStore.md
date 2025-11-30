# `BitStore`

## Introduction

`gf2::BitStore` is the _base class_ for the library's three vector-like types:

| Type           | Description                                                                       |
| -------------- | --------------------------------------------------------------------------------- |
| `gf2::BitSet`  | A fixed-size array of bits packed into a `std::array` of unsigned words.          |
| `gf2::BitVec`  | A dynamically sized vector of bits packed into a `std::vector` of unsigned words. |
| `gf2::BitSpan` | A non-owning _view_ of some slice of bits owned by one of the two previous types. |

The classes have _many_ methods in common, which are implemented by this base class, avoiding code duplication.

The only methods _not_ in this base class are those that deal with memory management, such as constructors, and functions that grow or shrink the store.

The `BitStore` class has no data members of its own.
It is purely a mechanism for sharing code between its three subclasses and for allowing generic programming over all bit-store types.

## Declaration

`gf2::BitStore` is declared as:

```cpp
template <typename Derived>
class BitStore {
 // ...
};
```

The `Derived` template parameter is the subclass that inherits from `BitStore`.
As discussed in the [Class Design](#class-design) section below, we are using the Curiously Recurring Template Pattern ([CRTP]) idiom to avoid the overhead of virtual function calls.

Each `Derived` class is declared as:

```cpp
template <typename Word>
class Derived : public BitStore<Derived> {
 // ...
};
```

where `Word` is the unsigned integer type used to store the bits in that particular `Derived` bit-store.

The particular word choice used in a bit-store is available as the typename `gf2::BitStore::word_type`.
The default is `usize`, which is typically the most efficient type for the target platform.

## Required Methods

This base class could be more accurately called `HasBitsStoredInWords`, where the `Words` should be in "quotes" because the actual underlying storage may not align perfectly with the words as seen by the base class.
We went with the shorter name `BitStore

> [!NOTE]
> The `BitStore` base class operates _as if_ bit element 0 is always stored at the **lowest-order** bit of "word" **0**.
> This is straightforward for bitsets and bit-vectors but requires some thought for bit-spans which may start and end part-way through underlying words.
> See the [BitSpan](BitSpan.md#required-bitstore-methods) documentation for more information.

To implement the shared functionality, the derived subclasses must implement some methods that access the underlying store used to hold their bits.

| Required Method             | Expected Return Value or Behaviour                                |
| --------------------------- | ----------------------------------------------------------------- |
| `gf2::BitStore::size`       | Returns the number of accessible bits in the object.              |
| `gf2::BitStore::data const` | Returns a const pointer to the first real underlying word.        |
| `gf2::BitStore::data`       | Returns a non-const pointer to the first real underlying word.    |
| `gf2::BitStore::offset`     | Returns the offset in bits to the first element in `data[0]`.     |
| `gf2::BitStore::words`      | Returns the _minimum_ number of words needed to store those bits. |
| `gf2::BitStore::word`       | Returns a copy of "word" number `i`.                              |
| `gf2::BitStore::set_word`   | Sets the accessible bits in "word" `i` to those from `v`.         |

### Aside

We could require the first four methods above and have the `BitStore` class implement the other three.

For example, the `words()` method could be implemented by `BitStore` as a trivial computation based on
`size()` and the number of bits per underlying word. However, all the subclasses already cache the required value, so we use that instead. Every hot loop in the library calls `words()`, and benchmarking shows that precomputing the value significantly improves performance.

Having optimised versions of the `word` and `set_word` methods has an even larger impact on performance.

### Sample Layout

Consider a bit-store with 20 elements, where the `Word` type used to store those bits is an unsigned 8-bit integer.

The `BitStore` base class expects `size()` to return 20 and `words()` to return 3, as it takes three 8-bit words to hold 20 bits with four bits to spare.

It expects that `word(0)` holds the first 8 bits in the bit-store, `word(1)` has the following 8 bits, and`word(2)` holds the final four elements in its four lowest order bits.
It also expects that the four highest "unoccupied" bits in`word(2)` are set to 0.

If the store is a bitset or a bit-vector, the implementation of the `BitStore` requirements is easy.
Those classes just have to be careful to ensure that any unoccupied high-order bits in the final word remain zeros.

It is a different matter for a bit-span, which isn't usually zero-aligned with the real underlying array of unsigned words, `w[0]`, `w[1]`, ...
The bit-store base class still expects that `words()` returns three even though the span may touch bits in _four_ underlying words!

For a bit-span, the return value for `word(i)` will often be synthesised from two contiguous "real" words
`w[j]` and `w[j+1]` for some `j`.
`word[i]` will use some high-order bits from `w[j]` and low-order bits from `w[j+1]` as shown in the following example:

![A bit-span with 20 elements, where the `X` is always zero](docs/images/bit-span.svg)

The `BitSpan` class behaves _as if_ bits from the real underlying store were copied and shuffled down so that element zero is bit 0 of word 0 in the bit-span. However, it never actually copies anything; instead, it synthesises "words" as needed.

The same principle applies to the `set_word` method.
The implementation of `set_word` for bit vectors and bit sets is trivial, with the one caveat that we have to be careful not to inadvertently touch any unoccupied bits in the final underlying word, or at least be sure to leave them as zeros.

In the case of a bit-span, calls to `set_word(i, value)` will generally copy low-order bits from `value` into the high-order bits of some real underlying word `w[j]` and copy the rest of the high-order bits from `value` into the low-order bits of `w[j+1]`. The other bits in `w[j]` and `w[j+1]` will not be touched.

## Provided Methods

Given the required methods listed above, the `BitStore` base class defines _many_ more member functions and operators inherited by its subclasses.
The provided methods fall into categories:

### Bit Access

The following methods provide access to individual bit elements in the bit-store.

| Method                                   | Description                                                     |
| ---------------------------------------- | --------------------------------------------------------------- |
| `gf2::BitStore::get`                     | Returns the value of a bit element as a read-only boolean.      |
| `gf2::BitStore::operator[](usize) const` | The operator version of the `get` method.                       |
| `gf2::BitStore::operator[](usize)`       | Returns a `BitProxy` with read-write access to a bit element.   |
| `gf2::BitStore::front`                   | Returns the value of the first element in the store.            |
| `gf2::BitStore::back`                    | Returns the value of the last element in the store.             |
| `gf2::BitStore::set`                     | Sets a bit to the given boolean value which defaults to `true`. |
| `gf2::BitStore::flip`                    | Flips the value of the bit element at a given index.            |
| `gf2::BitStore::swap`                    | Swaps the values of bit elements at locations `i` and `j`.      |

> [!NOTE]
> You can set the `DEBUG` flag at compile time to enable bounds checks on the index arguments.

The non-const version of `operator[]` returns a `gf2::BitRef`, which is "reference" to an individual bit in the store.
It is automatically converted to a boolean on reads, but it also allows writes, which means you can write natural-looking single-bit assignments:

```c++
v[12] = true;
```

This is equivalent to calling `v.set(12, true);`.

### Store Queries

The following methods let you query the overall state of the store.

| Method                          | Description                                             |
| ------------------------------- | ------------------------------------------------------- |
| `gf2::BitStore::is_empty`       | Returns true if the store is empty                      |
| `gf2::BitStore::any`            | Returns true if _any_ bit in the store is set.          |
| `gf2::BitStore::all`            | Returns true if _every_ bit in the store is set.        |
| `gf2::BitStore::none`           | Returns true if _no_ bit in the store is set.           |
| `gf2::BitStore::count_ones`     | Returns the number of set bits in the store.            |
| `gf2::BitStore::count_zeros`    | Returns the number of unset bits in the store.          |
| `gf2::BitStore::leading_zeros`  | Returns the number of leading unset bits in the store.  |
| `gf2::BitStore::trailing_zeros` | Returns the number of trailing unset bits in the store. |

These methods efficiently operate on words at a time, so they are inherently parallel.

### Store Mutators

The following methods let you mutate the entire store in a single call.

| Method                    | Description                                                                   |
| ------------------------- | ----------------------------------------------------------------------------- |
| `gf2::BitStore::set_all`  | Sets all the bits in the store to the passed value, which defaults to `true`. |
| `gf2::BitStore::flip_all` | Flips the values of all the bits in the store.                                |

These methods efficiently operate on words at a time, so they are inherently parallel.

### Store Fills

The following methods let you populate the entire store from multiple sources in a single call.

| Method                       | Description                                                      |
| ---------------------------- | ---------------------------------------------------------------- |
| `gf2::BitStore::copy`        | Copies the bits from various sources to this store.              |
| `gf2::BitStore::fill`        | Fills the store by repeatedly calling a function for each index. |
| `gf2::BitStore::random_fill` | Fills the store with random 0's and 1's.                         |

The `copy` method supports copying from:

-   Another `BitStore` of the same size but possibly a different underlying word type.
-   A [`std::bitset`] of the same size as the store.
-   An unsigned integer that has the same number of bits as the store.

> [!NOTE]
> In each case, the source and destination sizes must match exactly.
> That condition is always checked unless the `NDEBUG` flag is set at compile time.
> You can always use a `gf2::BitSpan` to copy a subset of bits if needed.

By default, the random fill method uses a random number generator seeded with system entropy, so the results change from run to run.
You can set a specific seed to get reproducible fills.

The default probability that a bit is set is 50%, but you can pass a different probability in the range `[0.0, 1.0]` if desired.

### Spans

The following methods let you create a `gf2::BitSpan`, which is a non-owning view of some contiguous subset of bits in the store.

| Method                      | Description                                                                                      |
| --------------------------- | ------------------------------------------------------------------------------------------------ |
| `gf2::BitStore::span const` | Returns a _read-only_ `gf2::BitSpan` encompassing the bits in a half-open range `[begin, end)`.  |
| `gf2::BitStore::span`       | Returns a _read-write_ `gf2::BitSpan` encompassing the bits in a half-open range `[begin, end)`. |

These methods take two indices, `begin` and `end`, that define a half-open range of bits in the store.

Mutability/immutability of the returned `BitSpan` is _deep_.
The span's mutability reflects that of the underlying store, so if the store is mutable, so is the span, and vice versa.

This is similar to the C++20 [`std::span`] class for regular data collection types.

> [!NOTE]
> A `gf2::BitSpan` is a `gf2::BitStore`, so you can take a span of a span.

### Sub-vectors

The following methods create or fill _independent_ bit-vectors with copies of some contiguous subset of the bits in the store.

| Method                    | Description                                                                            |
| ------------------------- | -------------------------------------------------------------------------------------- |
| `gf2::BitStore::sub`      | Returns a new `gf2::BitVec` encompassing the bits in a half-open range `[begin, end)`. |
| `gf2::BitStore::split_at` | Fills two bit-vectors with the bits in the ranges `[0, at)` and `[at, size())`.        |

The `split_at` method can optionally take two pre-existing bit-vectors to fill, thereby avoiding unnecessary allocations in some iterative algorithms that repeatedly use this method.

> [!NOTE]
> These methods do not alter the underlying store.

### Riffling

We have methods that can interleave (riffle) the bits in a store with zeros.

| Method                       | Description                                                                       |
| ---------------------------- | --------------------------------------------------------------------------------- |
| `gf2::BitStore::riffle_into` | Fills a pre-existing bit-vector with the result of riffling this store.           |
| `gf2::BitStore::riffled`     | Returns a new bit-vector that is this store with its bits interleaved with zeros. |

If the store looks like $v_0 v_1 v_2 \ldots v_n$, then the riffling operation produces the vector $v_0 0 v_1 0 v_2 0 \ldots v_n$ where a zero is interleaved _between_ every bit in the original store (there is no trailing zero at the end).

If you think of a bit-store as representing the coefficients of a polynomial over GF(2), then riffling corresponds to squaring that polynomial. See the documentation for `gf2::BitPoly::squared` for more information.

### Iteration

The following methods iterate over the bit store in various ways.

-   Read-only iteration through the individual bits.
-   Read-write iteration through the individual bits.
-   Read-only iteration through the indices of the set bits.
-   Read-only iteration through the indices of the unset bits.
-   Read-write iteration through the underlying store words.

| Method                             | Description                                                                           |
| ---------------------------------- | ------------------------------------------------------------------------------------- |
| `gf2::BitStore::first_set`         | Returns the index of the first set bit in the store.                                  |
| `gf2::BitStore::last_set`          | Returns the index of the last set bit in the store.                                   |
| `gf2::BitStore::next_set`          | Returns the index of the next set bit in the store _after_ the passed index.          |
| `gf2::BitStore::previous_set`      | Returns the index of the previous set bit in the store _before_ the passed index.     |
| `gf2::BitStore::first_unset`       | Returns the index of the first unset bit in the store.                                |
| `gf2::BitStore::last_unset`        | Returns the index of the last unset bit in the store.                                 |
| `gf2::BitStore::next_unset`        | Returns the index of the next unset bit in the store _after_ the passed index.        |
| `gf2::BitStore::previous_unset`    | Returns the index of the previous unset bit in the store _before_ the passed index.   |
| `gf2::BitStore::bits() const`      | Returns a `gf2::BitsIter` iterator to view the bits in the store.                     |
| `gf2::BitStore::bits()`            | Returns a `gf2::BitsIter` iterator to view and possibly mutate the bits in the store. |
| `gf2::BitStore::set_bit_indices`   | Returns a `gf2::SetBitsIter` iterator to view the indices of all the set bits.        |
| `gf2::BitStore::unset_bit_indices` | Returns a `gf2::UnsetBitsIter` iterator to view the indices of all the unset bits.    |
| `gf2::BitStore::words`             | Returns a `gf2::WordsIter` iterator to view the "words" underlying the store.             |
| `gf2::BitStore::to_words`          | Returns a copy of the words underlying the bit-store.                                 |

The `gf2::BitsIter` iterator has a const and a non-const version, where the const-ness refers to the store being iterated over.

### Stringification

The following methods return a string representation for a bit-store.
The string can be in the obvious binary format or a more compact hex format.

| Method                                     | Description                                                    |
| ------------------------------------------ | -------------------------------------------------------------- |
| `gf2::BitStore::to_string`                 | Returns a default string representation for a bit-store.       |
| `gf2::BitStore::to_pretty_string`          | Returns a "pretty" string representation for a bit-store.      |
| `gf2::BitStore::to_binary_string`          | Returns a binary string representation for a bit-store.        |
| `gf2::BitStore::to_hex_string`             | Returns a compact hex string representation for a bit-store.   |
| `gf2::BitStore::operator<<(std::ostream&)` | The usual output stream output stream operator for bit-stores. |
| `struct std::formatter<gf2::BitStore>`     | Specialisation of [`std::formatter`] for bit-stores.           |

#### Binary String Encoding

The straightforward character encoding for a bit-vector is a _binary_ string containing just 0's and 1's, for example, `"10101"`.
Each character in a binary string represents a single element in the bit-store.
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

#### Hex String Encoding

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

### Equality Operator

As you'd expect, equality between two bit-stores tests whether they have identical content.
It **also** requires that the two stores use the same underlying word type.

With a few well-documented exceptions, the library avoids interactions between bit-stores with different word types.
While it is possible to implement cross-type interactions, such as comparing a `BitStore<uint32_t>` with a `BitStore<uint64_t>`, this is not a typical use case.
Supporting such interactions would add complexity and bloat the compiled code with many more template instantiations.

| Method            | Description                                                                                |
| ----------------- | ------------------------------------------------------------------------------------------ |
| `gf2::operator==` | Checks that two stores are using the same underlying word type and have identical content. |

### Bit Shifts

| Method                            | Description                                                        |
| --------------------------------- | ------------------------------------------------------------------ |
| `gf2::BitStore::operator<<=`      | Left shifts in-place.                                              |
| `gf2::BitStore::operator>>=`      | Right shifts in-place.                                             |
| `gf2::BitStore::operator<< const` | Copies the store to a new bit-vector and left shifts that vector.  |
| `gf2::BitStore::operator>> const` | Copies the store to a new bit-vector and right shifts that vector. |

These operators act in vector space, so if the vector is $v_0, v_1, \ldots, v_n$, then a right shift produces the vector $0, v_0, v_1, \ldots, v_n-1$ where we have shifted out the last element and shifted in a zero at the start.
Similarly, a left shift produces the vector $v_1, v_2, \ldots, v_n, 0$ where we have shifted out the first element and shifted in a zero at the end.

Contrast this to shifts in bit space, where if a bit container is $b_n, b_{n-1}, \ldots, b_0$, then a right shift produces $0, b_n, b_{n-1}, \ldots, b_1$ and a left shift produces $b_{n-1}, b_{n-2}, \ldots, b_0, 0$.

Essentially, right shifts in vector space correspond to left shifts in bit space, and vice versa.

> [!NOTE]
> Like every method in the library, the shift operators are implemented efficiently to operate on words at a time.

### Bit-wise Operators

We have methods that combine two bit-stores using the logical operations `XOR`, `AND`, and `OR`.

> [!IMPORTANT]
> These methods require that the two bit-stores use the same underlying word type.
> They also require that the left-hand-side and right-hand-side bit-store operands are the same size.
> That precondition is always checked unless the `NDEBUG` flag is set at compile time.
> Interactions between bit-stores with different word types are only possible at the cost of increased code complexity, and are not a common use case.

The methods can act in place, mutating the left-hand side operator: `lhs &= rhs`.
There is also a non-mutating version `result = lhs & rhs`, which returns a new `result` _bit-vector_ in each case.

| Method                       | Description                                                                         |
| ---------------------------- | ----------------------------------------------------------------------------------- |
| `gf2::BitStore::operator^=`  | In-place `XOR` operation of equal-sized bit-stores: `lhs = lhs ^ rhs`.              |
| `gf2::BitStore::operator&=`  | In-place `AND` operation of equal-sized bit-stores: `lhs = lhs & rhs`.              |
| `gf2::BitStore::operator\|=` | In-place `OR` operation of equal-sized bit-stores: `lhs \|= lhs ^ rhs`.             |
| `gf2::BitStore::operator~`   | Returns a new _bit-vector_ that has the bits all flipped.                           |
| `gf2::BitStore::operator^`   | Returns the `XOR` of this store with another equal-sized store as a new bit-vector. |
| `gf2::BitStore::operator&`   | Returns the `AND` of this store with another equal-sized store as a new bit-vector. |
| `gf2::BitStore::operator\`   | Returns the `OR` of this store with another equal-sized store as a new bit-vector.  |

### Arithmetic Operators

In GF(2), the arithmetic operators `+` and `-` are both the `XOR` operator.

| Method            | Description                                                                  |
| ----------------- | ---------------------------------------------------------------------------- |
| `gf2::operator+=` | Adds the passed (equal-sized) `rhs` bit-store to this one.                   |
| `gf2::operator-=` | Subtracts the passed (equal-sized) `rhs` bit-store from this one.            |
| `gf2::operator+`  | Adds two equal-sized bit-stores and returns the result as a bit-vector.      |
| `gf2::operator-`  | Subtracts two equal-sized bit-stores and returns the result as a bit-vector. |

The constraints mentioned in the last bit-twiddling section also apply here.

### Other Functions

| Method           | Description                                                         |
| ---------------- | ------------------------------------------------------------------- |
| `gf2::dot`       | Returns the dot product of two equal-sized bit-stores as a boolean. |
| `gf2::operator*` | Returns the dot product of two equal-sized bit-stores as a boolean. |
| `gf2::convolve`  | Returns the convolution of two bit-stores as a new bit-vector.      |
| `gf2::join`      | Concatenates two bit-stores into a new bit-vector.                  |

We have overloaded the `*` operator to compute the dot product.

## Class Design

A textbook implementation of `BitStore` as an _abstract base class_ might look like:

```c++
template<unsigned_word Word>
class gf2::BitStore {
public:
 // Declare the methods that subclasses must implement ...
    virtual usize  size() const = 0;
    virtual const Word*  data() const = 0;
    virtual Word*        data() = 0;
    virtual u8 offset() const = 0;
    virtual usize  words() const = 0
    virtual Word         word(usize i) const = 0;
    virtual void         set_word(size_t i, Word value) = 0;

 // Use those to define many other useful concrete methods listed above ...
};
```

The bit-vector subclass might then look like:

```c++
template<unsigned_word Word>
class BitVector: gf2::BitStore<Word> {
private:
    usize       m_size;
    std::vector<Word> m_data;
public:
 // Implement the methods needed to keep our base class happy -- most are completely trivial:
    virtual usize size() const              { return m_size;        }
    virtual const Word* data() const              { return m_data.data(); }
    virtual       Word* data()                    { return m_data.data(); }
    virtual       u8    offset() const            { return 0;             }
    virtual usize words() const             { return m_data.size(); }
    virtual Word        word(usize i) const { return m_data[i];     }

 // The `set_word` method needs to keep the final underlying word "clean".
    virtual void set_word(size_t i, Word value) {
        m_data[i] = value;
        if(i == m_data.size() - 1) ...
    }

 // Add constructors, and any methods that cannot be in `gf2::BitStore` ...
};
```

The `BitSpan` class has a more complicated version of the `word` and `set_word` virtual methods, but the relationship to the methods inherited from the `BitStore` base class is transparent.

This design, where `BitStore` is an abstract base class, does work.
It has the advantage of being straightforward to understand.
Unfortunately, it is also slow!

There is always some cost associated with virtual dispatch.
Instead of jumping directly into a method implementation, a runtime lookup of the `vtbl` is required to find the address to jump to.
Often, this penalty is very slight and poses no concern.

However, in our case, the virtual methods are at the heart of every hot loop in the library, which is a problem for a numerical library expected to be efficient with large bit vectors and bit matrices.

We instrumented an initial implementation of the library to identify where time was spent in non-trivial problems.
The instrumentation tool worked on object code produced by the compiler invoked with maxed-out optimisation settings, so it was representative of real-world applications.
It became immediately apparent that virtual dispatch consumes significant time.

### The [CRTP] Idiom

The purpose of introducing the `BitStore` base class is to eliminate code duplication.
The base class _needs_ those "virtual" methods to do its job.
How can we supply the required methods _without_ runtime virtual dispatch?

The answer is to use _compile-time_ virtual dispatch instead.

This is a C++ pattern known as "The Curiously Recurring Template Pattern" ([CRTP] for short).
And it does look curious at first!

Instead of having a template parameter `Word`, in this paradigm, the `BitStore` class is templatised over its subclasses!
A code sketch looks like:

```c++
template<typename RealStore>
class gf2::BitStore {
private:
 // Helper methods to convert this `gf2::BitStore` to the "real" store, which owns and can access data.
    const RealStore* real_store() const { return static_cast<const RealStore*>(this); }
          RealStore* real_store()       { return static_cast<RealStore*>(this);       }
public:
 // Punt the required methods to the real `Store` by casting ourselves to a subclass first.
    constexpr usize  size() const                    { return real_store()->size(); }
    constexpr auto         data() const                    { return real_store()->data(); }
    constexpr auto         data()                          { return real_store()->data(); }
    constexpr u8 offset() const                            { return real_store()->offset(); }
    constexpr usize  words() const                   { return real_store()->words(); }
    constexpr auto         word(usize i) const       { return real_store()->word(i); }
    constexpr void         set_word(usize i, auto v) { real_store()->set_word(i, v); }

 // Now we can use those to define many other useful concrete methods listed above ...
};
```

There are no virtual methods here; instead, the `BitStore` class is templatised over its subclasses.
When it needs to access a piece of data, it just converts itself into the appropriate subclass and directly accesses the data.

The bit-vector subclass now might look like:

```c++
template<unsigned_word Word>
class BitVector: gf2::BitStore<BitVector<Word>> {
private:
    usize       m_size;
    std::vector<Word> m_data;

public:
 // Implement the methods needed to keep our base class happy -- trivial and non-virtual:
    usize size() const              { return m_size;        }
    auto        data() const              { return m_data.data(); }
    auto        data()                    { return m_data.data(); }
    u8          offset() const            { return 0;             }
    usize words() const             { return m_data.size(); }
    auto        word(usize i) const { return m_data[i];     }

 // The `set_word` method needs to keep the final underlying word "clean".
    void set_word(size_t i, auto value) {
        m_data[i] = value;
        if(i == m_data.size() - 1) ...
    }

 // Add constructors, and any methods that cannot be in `gf2::BitStore` ...
};
```

> [!NOTE]
> Moving from runtime virtual dispatch to compile-time dispatch has a measurable performance benefit.
> A speed-up by a factor of three was observed for the largest eigenvalue problems.

### One Gotcha

In practice, the `BitStore` class needs to know the size of the specific unsigned integral type used by the subclass to store its bits.
We "cheated" in the listing above by using `auto` in the method signatures, though that works for the methods shown.
However, other methods really need to know the exact `Word` type.

The obvious solution is to add a `using word_type = Word;` declaration to each subclass and then have the base class access that type via `RealStore::word_type`.
So the `BitVector` class would look like:

```c++
template<unsigned_word Word>
class BitVector: gf2::BitStore<BitVector<Word>> {
public:
    using word_type = Word;
 ...
};
```

The `BitStore` class would then use `typename RealStore::word_type` wherever it needed to know the underlying word type, and look like:

```c++
template<typename RealStore>
class gf2::BitStore {
public:
    using word_type = typename RealStore::word_type;
 ...
};
```

Unfortunately, this doesn't work!
The C++ standard requires that all dependent types must be fully defined before use.
That means that when the compiler is compiling the `BitStore` class template, it needs to know what `RealStore::word_type` is.
However, at that point, the `RealStore` class is only partially defined, so the compiler cannot know what `word_type` is.
A classic circular definition problem!

We could add an extra template parameter to `BitStore` to specify the underlying word type:

```c++
template<typename RealStore, unsigned_word Word>
class gf2::BitStore {
 ...
};
```

Then the `BitVector` class would look like:

```c++
template<unsigned_word Word>
class BitVector: gf2::BitStore<BitVector<Word>, Word> {
 ...
};
```

This scheme works, but it is clunky.
Everywhere we have a `BitStore` class, we have to specify the underlying word type again, which is redundant since the subclass already knows that type.

The solution we adopted is to introduce a separate trivial _trait_ class that each subclass must specialise to define its underlying word type.

```c++
template<typename RealStore>
struct store_word;

template<unsigned_word Word>
struct store_word<BitVector<Word>> {
    using type = Word;
};
```

The `store_word` class only exists to provide the underlying word type for each subclass.

The `BitStore` class can then access the underlying word type via the traits class:

```c++
template<typename RealStore>
class gf2::BitStore {
public:
    using word_type = typename store_word<RealStore>::type;
 ...
};
```

Introducing this trivial level of indirection solves the circular definition problem cleanly.
The `BitStore` class retains a single template parameter and can still access the underlying word type of its subclasses as needed.

## See Also

-   `gf2::BitStore` for detailed documentation of all class methods.
-   [`BitSet`](BitSet.md) for fixed-size vectors of bits.
-   [`BitVec`](BitVec.md) for dynamically-sized vectors of bits.
-   [`BitSpan`](BitSpan.md) for non-owning views into any bit-store.

<!-- Reference Links -->

[`std::span`]: https://en.cppreference.com/w/cpp/container/span.html
[`std::formatter`]: https://en.cppreference.com/w/cpp/utility/format/formatter.html
[`std::bitset`]: https://en.cppreference.com/w/cpp/utility/bitset
[CRTP]: https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
