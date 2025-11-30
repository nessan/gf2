# `BitSet`

## Introduction

A `gf2::BitSet` is a _fixed_ size vector of bit elements stored compactly in an array of unsigned integer words. <br>
The size is set at compile time by a template parameter.

The class inherits from `gf2::BitStore`, which provides a rich API for manipulating the bits in the array.

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

The `gf2::BitSet` class inherits from `gf2::BitStore` using the Curiously Recurring Template Pattern ([CRTP]) idiom to avoid the overhead of virtual function calls.
The declaration looks like:

```cpp
template <usize N, unsigned_word Word = usize>
class BitSet : public BitStore<BitSet<Word>> {
 // ...
};
```

The first template parameter `N` specifies the number of bit elements in the bitset, which are all initialised to zero.

The `Word` template parameter specifies the underlying unsigned integer type used to store the vector's bits, and the default is usually the most efficient type for the target platform.
On most modern platforms, that [`usize`] will be a 64-bit unsigned integer.

If your application calls for a vast number of bit arrays with only a few bits each, you might consider using `std::uint8_t` as the `Word` type to save memory.

> [!NOTE]
> You might notice that many of the [doctests] in the library use 8-bit underlying words.
> The reason is we want to exercise the various functions across word boundaries for modest, easily readable bit-stores.

## Required `BitStore` Methods

The base `gf2::BitStore` class provides all bitset functionality.
That class requires us to provide the following methods:

| Method Name               | Description                                                       |
| ------------------------- | ----------------------------------------------------------------- |
| `gf2::BitSet::size`       | Returns `N`, the number of bit elements in the bitset.            |
| `gf2::BitSet::words`      | Returns the number of `Word`s needed to store those elements.     |
| `gf2::BitSet::word`       | Returns the word for the passed index.                            |
| `gf2::BitSet::set_word`   | Sets the word at the passed index to the passed value.            |
| `gf2::BitSet::data const` | Returns a const pointer to the beginning of the `Word` store.     |
| `gf2::BitSet::data`       | Returns a non-const pointer to the beginning of the `Word` store. |
| `gf2::BitSet::offset`     | For bitsets, this always returns 0.                               |

These methods are trivial to implement for bit arrays.

The one place where care is needed is in the `set_word` method, which must ensure that any bits beyond the bitset's size remain set to zero.

> [!WARNING]
> While the `gf2::BitSet::data` method provides write access to the underlying words, you must be careful when modifying them directly.
> You must ensure that any bits beyond the bitset's size remain zero.
> The `set_word` method takes care of this for you, so prefer using that method when possible.

## Inherited Methods

A `gf2::BitSet` has all the methods of a `gf2::BitStore` available to it.

## See Also

-   `gf2::BitSet` for detailed documentation of all class methods.
-   [`BitStore`](BitStore.md) for the common API shared by all bit-stores.
-   [`BitVec`](BitVec.md) for dynamically-sized vectors of bits.
-   [`BitSpan`](BitSpan.md) for non-owning views into any bit-store.
-   [`BitMat`](BitMat.md) for matrices of bits.
-   [`BitPoly`](BitPoly.md) for polynomials over GF(2).

<!-- Reference Links -->

[GF2]: https://en.wikipedia.org/wiki/GF(2)
[Galois-Field]: https://en.wikipedia.org/wiki/Galois_field
[`std::array`]: https://en.cppreference.com/w/cpp/container/array
[`std::bitset`]: https://en.cppreference.com/w/cpp/utility/bitset
[`usize`]: https://en.cppreference.com/w/cpp/types/size_t
[CRTP]: https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
[doctests]: https://nessan.github.io/doxytest/
