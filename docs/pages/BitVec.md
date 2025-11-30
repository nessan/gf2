# `BitVec`

## Introduction

A `gf2::BitVec` is a dynamically sized vector of bit elements stored compactly in an array of unsigned integer words.

The class inherits from `gf2::BitStore`, which provides a rich API for manipulating the bits in the vector.
In addition, `gf2::BitVec` provides methods to resize the vector and append or remove elements from the end.

In mathematical terms, a bit-vector is a vector over [GF2], the simplest [Galois-Field] with just two elements, usually denoted 0 & 1, as the booleans true & false, or as the bits set & unset.
Arithmetic over GF(2) is mod 2, so addition/subtraction becomes the `XOR` operation while multiplication/division becomes `AND`.

> [!NOTE]
> Operations on and between bit-vectors and other objects in the `gf2` library are implemented using bitwise operations on whole underlying words at a time.
> These operations are highly optimised in modern CPUs, allowing for fast computation even on large bit-vectors.
> It also means we never have to worry about overflows or carries as we would with normal integer arithmetic.

The `gf2::BitVec` class is a hybrid between a [`std::vector`] and a [`std::bitset`], along with extra mathematical features to facilitate numerical work, and in particular, linear algebra.

One can dynamically resize a `BitVec` as needed.
Contrast this to a `std::bitset`, which has a _fixed_ size determined at compile time.
_Boost_ provides the [`boost::dynamic_bitset`] class, which allows runtime resizing, as its name suggests.
However, neither class supports algebraic operations.

It is worth noting that a `BitVec` is printed in _vector order_.
For example, a bit-vector of size four will print as $v_0 v_1 v_2 v_3$ with the elements in increasing index order, so the least significant vector element, $v_0$, comes **first** on the _left_.
Contrast that to a `std::bitset`, which always prints in _bit-order_.
The equivalent `std::bitset` with four elements prints as $b_3 b_2 b_1 b_0$ with the least significant bit $b_0$ printed **last** on the _right_.

Of course, for many applications, printing in _bit-order_ makes perfect sense.
A bit vector of four elements initialised to `0x1` will print `1000`.
A `std::bitset` prints the same value as `0001`, which will be more natural in some settings.

However, our main aim is numerical work, where vector order is more natural.
In particular, bit-order is unnatural for _matrices_ over GF(2).
It is too confusing to print a matrix in any order other than the one where the (0,0) element is at the top left, and proceed from there.

## Declaration

The `gf2::BitVec` class inherits from `gf2::BitStore` using the Curiously Recurring Template Pattern ([CRTP]) idiom to avoid the overhead of virtual function calls.
The declaration looks like this:

```cpp
template <unsigned_word Word = usize>
class BitVec : public BitStore<BitVec<Word>> {
 // ...
};
```

The `Word` template parameter specifies the underlying unsigned integer type used to store the vector's bits, and the default is usually the most efficient type for the target platform.
On most modern platforms, that [`usize`] will be a 64-bit unsigned integer.

If your application calls for a vast number of bit-vectors with only a few bits each, you might consider using `std::uint8_t` as the `Word` type to save memory.

> [!NOTE]
> You might notice that many of the [doctests] in the library use 8-bit underlying words.
> The reason is we want to exercise the various functions across word boundaries for modest, easily readable bit-stores.

## Required `BitStore` Methods

The base `gf2::BitStore` class provides most bit-vector functionality.
That class requires us to provide the following methods:

| Method Name               | Description                                                       |
| ------------------------- | ----------------------------------------------------------------- |
| `gf2::BitVec::size`       | Returns the number of bit elements in the bit-vector.             |
| `gf2::BitVec::words`      | Returns the number of `Word`s needed to store those elements.     |
| `gf2::BitVec::word`       | Returns the word for the passed index.                            |
| `gf2::BitVec::set_word`   | Sets the word at the passed index to the passed value.            |
| `gf2::BitVec::data const` | Returns a const pointer to the beginning of the `Word` store.     |
| `gf2::BitVec::data`       | Returns a non-const pointer to the beginning of the `Word` store. |
| `gf2::BitVec::offset`     | For bit-vectors, this always returns 0.                           |

These methods are trivial to implement for bit vectors.

The one place where care is needed is in the `set_word` method, which must ensure that any bits beyond the size of the bit-vector remain set to zero.

> [!WARNING]
> While the `gf2::BitVector::data` method provides write access to the underlying words, you must be careful when modifying them directly.
> You must ensure that any bits beyond the bit-vector's size remain zero.
> The `set_word` method takes care of this for you, so prefer using that method when possible.

## Constructors

The default constructor for a `gf2::BitVec` creates an _empty_ bit-vector with zero size.
You can also create a bit-vector of a given size using the following constructors:

| Method Name                        | Description                                                                  |
| ---------------------------------- | ---------------------------------------------------------------------------- |
| `gf2::BitVec::BitVec(usize)`       | Constructs a bit-vector of the given length where all the elements are zero. |
| `gf2::BitVec::BitVec(usize, Word)` | Constructs a bit-vector by repeatedly copying the bits from the `Word`       |

### Factory Constructors

Most construction methods are static factory functions.

| Method Name                  | Description                                                                          |
| ---------------------------- | ------------------------------------------------------------------------------------ |
| `gf2::BitVec::with_capacity` | Returns a zero-sized bit-vector that can add elements without any extra allocations. |
| `gf2::BitVec::zeros`         | Returns a bit-vector where all the elements are 0.                                   |
| `gf2::BitVec::ones`          | Returns a bit-vector where all the elements are 1.                                   |
| `gf2::BitVec::constant`      | Returns a bit-vector where all the elements are whatever is passed as a `value`.     |
| `gf2::BitVec::unit`          | Returns a bit-vector where all the elements are zero except for a single 1.          |
| `gf2::BitVec::alternating`   | Returns a bit-vector where all the elements follow the pattern `101010...`           |
| `gf2::BitVec::from`          | Returns a `BitVec` filled with bits from various sources.                            |
| `gf2::BitVec::random`        | Returns a `BitVec` with a random fill seeded from entropy.                           |
| `gf2::BitVec::seeded_random` | Returns a `BitVec` with a reproducible random fill.                                  |
| `gf2::BitVec::biased_random` | Returns a random `BitVec` where you set the probability of bits being 1.             |

The `from` factory method is overloaded to allow construction from:

-   Any other `gf2::BitStore` object, which need not share the same underlying storage type.
-   A [`std::bitset`] where we copy the bits over.
-   A callable object (a function of some sort) that generates bits on demand when passed an index.

### Construction from Strings

We can construct a `gf2::BitVec` from strings -- these methods can fail, so they return a `std::optional<gf2::BitVec>` and [`std::nullopt`] if the string cannot be parsed.

| Method Name                       | Description                                                  |
| --------------------------------- | ------------------------------------------------------------ |
| `gf2::BitVec::from_string`        | Tries to construct a bit-vector from an arbitrary string.    |
| `gf2::BitVec::from_binary_string` | Tries to construct a bit-vector from a _binary_ string.      |
| `gf2::BitVec::from_hex_string`    | Tries to construct a bit-vector from a _hexadecimal_ string. |

Space, comma, single quote, and underscore characters are removed from the string.

If the string has an optional `"0b"` prefix, it is assumed to be binary.
If it has an optional `"0x"` prefix, it is assumed to be hex.
If there is no prefix and the string consists entirely of 0s and 1s, we assume it is binary; otherwise, we think it is hex.

> [!WARNING]
> This means the string `"0x11` is interpreted as the bit-vector of size 8 `"11110001"`, whereas the same string without a prefix, `"11"` is interpreted as the bit-vector of size 2 `"11"`.
> To avoid any ambiguity, it is best to use a prefix.

#### Binary String Encoding

The straightforward character encoding for a bit-vector is a _binary_ string containing just 0's and 1's, for example, `"10101"`.
Each character in a binary string represents a single element in the bit-store.

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

## Size & Capacity

We have methods to query and manipulate the size and capacity of a bit-vector:

| Method Name                  | Description                                                                          |
| ---------------------------- | ------------------------------------------------------------------------------------ |
| `gf2::BitVec::size`          | Returns the number of bit elements in the bit-vector.                                |
| `gf2::BitVec::capacity`      | Returns the total number of bits the vector can hold without allocating more memory. |
| `gf2::BitVec::shrink_to_fit` | Tries to shrink the vector's capacity as much as possible.                           |
| `gf2::BitVec::clear`         | Sets the `size()` to zero. Leaves the capacity unaltered.                            |
| `gf2::BitVec::resize`        | Returns a const pointer to the beginning of the `Word` store.                        |
| `gf2::BitVec::clean`         | Sets any unused bits in the _last_ occupied word to 0.                               |

The `clean` method is primarily used internally in the library.

## Appending Elements

We have methods to append elements from various sources to the end of a bit-vector:

| Method Name                     | Description                                                            |
| ------------------------------- | ---------------------------------------------------------------------- |
| `gf2::BitVec::push`             | Pushes a single bit (0 or 1) onto the end of the bit-vector.           |
| `gf2::BitVec::append`           | Appends bits from various sources to the end of the bit-vector.        |
| `gf2::BitVec::append_digit`     | Appends a "character's" worth of bits to the end of the bit-vector.    |
| `gf2::BitVec::append_hex_digit` | Appends four bits from a "hex-character" to the end of the bit-vector. |

The `append` method is overloaded to allow appending bits from:

-   A [`std::bitset`] where we append the bits to the end of the bit-vector.
-   Any unsigned integer type, where we append the bits corresponding to the value.<br> The type need not be the same as the underlying `Word` type used by the bit-vector.
-   Any other `gf2::BitStore` object, which need not share the same underlying `Word` storage type.

The `append_digit` method appends bits from a character representing a digit in one of the bases 2, 4, 8, or 16. It does nothing if it fails to parse the character.

## Removing Elements

We have methods to remove elements from the end of a bit-vector:

| Method Name                       | Description                                                                                     |
| --------------------------------- | ----------------------------------------------------------------------------------------------- |
| `gf2::BitVec::pop`                | Removes the last bit from the bit-vector and returns it.                                        |
| `gf2::BitVec::split_off_unsigned` | Removes a single arbitrary-sized unsigned integer off the end of the bit-vector and returns it. |
| `gf2::BitVec::split_off`          | Splits a bit-vector into two at a given index                                                   |

The first two methods return the removed elements as a [`std::optional`], and as a [`std::nullopt`] if the vector is empty.

The two `split_off` methods complement the `gf2::BitStore::split_at` methods.
The `BitVector` versions change the size of the bit-vector in place, which a `BitStore` cannot do.

## Inherited Methods

A `gf2::BitVec` has all the methods of a `gf2::BitStore` available to it.

## See Also

-   `gf2::BitVec` for detailed documentation of all class methods.
-   [`BitStore`](BitStore.md) for the common API shared by all bit-stores.
-   [`BitSet`](BitSet.md) for fixed-size vectors of bits.
-   [`BitSpan`](BitSpan.md) for non-owning views into any bit-store.
-   [`BitMat`](BitMat.md) for matrices of bits.
-   [`BitPoly`](BitPoly.md) for polynomials over GF(2).

<!-- Reference Links -->

[GF2]: https://en.wikipedia.org/wiki/GF(2)
[Galois-Field]: https://en.wikipedia.org/wiki/Galois_field
[`std::vector`]: https://en.cppreference.com/w/cpp/container/vector
[`std::optional`]: https://en.cppreference.com/w/cpp/utility/optional
[`std::nullopt`]: https://en.cppreference.com/w/cpp/utility/optional/nullopt
[`std::bitset`]: https://en.cppreference.com/w/cpp/utility/bitset
[`usize`]: https://en.cppreference.com/w/cpp/types/size_t
[`boost::dynamic_bitset`]: https://www.boost.org/doc/libs/release/libs/dynamic_bitset/dynamic_bitset.html
[CRTP]: https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
[doctests]: https://nessan.github.io/doxytest/
