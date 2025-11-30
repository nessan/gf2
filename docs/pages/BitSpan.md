# `BitSpan`

## Introduction

A `gf2::BitSpan` is a non-owning view of a contiguous range of bits from some underlying store of unsigned words.

The class inherits from `gf2::BitStore`, which provides a rich API for manipulating the bits in the span.

A span is a cheap way to work with a slice of a `gf2::BitSet`, `gf2::BitVec`, or any other `gf2::BitStore`.
It never allocates or copies; it just remembers a pointer into the backing words, a bit-offset, and a length.

The template parameter keeps the underlying word type (and const-ness) so that `BitSpan<const Word>` is read-only while `BitSpan<Word>` lets you mutate through the view.

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

The `gf2::BitSpan` class inherits from `gf2::BitStore` using the Curiously Recurring Template Pattern ([CRTP]) idiom to avoid the overhead of virtual function calls.
The declaration looks like this:

```cpp
template <unsigned_word Word>
class BitSpan : public BitStore<BitSpan<Word>> {
 // ...
};
```

`gf2::BitSpan` inherits the full `gf2::BitStore` API, so you can read/write bits, iterate set or unset indices, riffle, convolve, or form sub-spans just as you would for a `gf2::BitVec` or a `gf2::BitSet`.

In the case of a a bit-span, the const-ness of the `Word` type determines whether the span is read-only or read-write.
A span created using `gf2::BitStore::span(begin, end) const` will have `Word` as a `const` type, while one created using `gf2::BitStore::span(begin, end)` will have a non-const `Word` type.
The former is a read-only view, while the latter allows mutation.

> [!NOTE]
> Spans are lightweight and you generally pass them by value so `fun(BitSpan<Word> span)` is preferred over `fun(const BitSpan<Word>& span)`.
> This form also emphasises that what matters is the _interior_ const-ness of the span, not whether the span object itself is const.

## Required `BitStore` Methods

The base `gf2::BitStore` class provides all bit-span functionality.
That class requires us to provide the following methods:

| Method Name                | Description                                                             |
| -------------------------- | ----------------------------------------------------------------------- |
| `gf2::BitSpan::size`       | Returns the number of bit elements in the span.                         |
| `gf2::BitSpan::words`      | Returns the _minimum_ number of `Word`s needed to store those elements. |
| `gf2::BitSpan::word`       | Returns the "word" for the passed index.                                |
| `gf2::BitSpan::set_word`   | Sets the "word" at the passed index to the passed value.                |
| `gf2::BitSpan::data const` | Returns a const pointer to the beginning of the underlying store.       |
| `gf2::BitSpan::data`       | Returns a non-const pointer to the beginning of the underlying store.   |
| `gf2::BitSpan::offset`     | The bit-span begins at this bit-offset inside its first word.           |

These methods were trivial to implement for bit arrays and vectors but require some thought for bit spans.

The key thing to understand is that the base `gf2::BitStore` class operates _as if_ the span is a contiguous array of bits starting at bit-index **0**.

Of course, the actual bits in the span may start part-way through a word in the underlying store, so we have to adjust for that in our implementation.

For a bit-span, the return value from the `gf2::BitSpan::word(i)` method will often be synthesised from two contiguous "real" words `w[j]` and `w[j+1]` for some `j`:

`word[i]` will use some high-order bits from `w[j]` and low-order bits from `w[j+1]` as shown in the following example:

![A bit-span with 20 elements, where the `X` is always zero](docs/images/bit-span.svg)

The `BitSpan` class behaves _as if_ bits from the real underlying store were copied and shuffled down so that element zero is bit 0 of word 0 in the bit-span. However, it never actually copies anything; instead, it synthesises "words" as needed.

The same principle applies to the `gf2::BitSpan::set_word(i, value)` method.
In the case of a bit-span, calls to `set_word(i, value)` will generally copy low-order bits from `value` into the high-order bits of some real underlying word `w[j]` and copy the rest of the high-order bits from `value` into the low-order bits of `w[j+1]`.

The other bits in `w[j]` and `w[j+1]` will not be touched.

> [!WARNING]
> While the `gf2::BitSpan::data` method provides write access to the underlying words, this is primarily for internal use.
> If you do use the pointer, you must ensure that any bits outside the span's range remain unaltered.
> The `set_word` method takes care of this for you, so prefer using that method when possible.

## Construction

You don't usually construct `BitSpan` objects directly.
Instead, you typically obtain them from existing `gf2::BitStore` objects using their `span` member functions:

-   `gf2::BitStore::span(begin, end) const` creates a bit-span as a read-only view of the bits in the half open range `[begin, end)`.
-   `gf2::BitStore::span(begin, end)` creates a bit-span as a read-write view of the bits in the half open range `[begin, end)`.

Spans can start in the middle of a machine word and can end part-way through another word; the class synthesises whole-word reads/writes for you.

Empty spans are not allowed.

### Example

```cpp
auto v = gf2::BitVec<u8>::from_string("1111'1111'1111").value();
v.span(2, 6).flip_all();
assert_eq(v.to_string(), "110000111111");
```

## Inherited Methods

A `gf2::BitSpan` has all the methods of a `gf2::BitStore` available to it.

## Lifetimes

Because a `gf2::BitSpan` keeps a pointer back to the underlying `gf2::BitStore`, that store must outlive the span.

In the Rust version of the library, this is enforced using lifetime parameters.
In C++, this is just a convention the user must follow.

We could use smart pointers to enforce this at runtime, but that would add overhead and complexity to the concrete `gf2::BitStore` types.

## See Also

-   `gf2::BitSpan` for detailed documentation of all class methods.
-   [`BitStore`](BitStore.md) for the common API shared by all bit-stores.
-   [`BitSet`](BitSet.md) for fixed-size vectors of bits.
-   [`BitVec`](BitVec.md) for dynamically-sized vectors of bits.
-   [`BitMat`](BitMat.md) for matrices of bits.
-   [`BitPoly`](BitPoly.md) for polynomials over GF(2).

<!-- Reference Links -->

[`std::span`]: https://en.cppreference.com/w/cpp/container/span
[CRTP]: https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
