# Iterators

## Introduction

The `<gf2/Iterators.h>` header provides five iterators over bit-stores that serve different purposes:

| Iterator         | Description                                                                                         |
| ---------------- | --------------------------------------------------------------------------------------------------- |
| `gf2::Bits`      | The const version reads individual bits from a bit-store one at a time.                             |
| `gf2::Bits`      | The non-const version reads and potentially mutates individual bits from a bit-store one at a time. |
| `gf2::SetBits`   | Iterates through the _index locations_ of the set bits in the associated bit-store.                 |
| `gf2::UnsetBits` | Iterates through the _index locations_ of the unset bits in the associated bit-store.               |
| `gf2::Words`     | Allows reading and writing of "words" that underlie the bit-store.                                  |

The `gf2::Bits` comes in both const and non-const versions.
The `value_type` of the const version is `bool`, allowing you to read individual bits from the bit-store.
The `value_type` of the non-const version is `gf2::BitRef` that allows modification of individual bits as you iterate through them.

> [!NOTE]
> Const-ness here refers to whether the bits in the bit-store can be modified via the iterator, and not whether the iterator itself is const.

## Construction

You can create these iterators for any type that implements the `gf2::BitStore` interface, such as `gf2::BitVector`, `gf2::BitArray`, and `gf2::BitSpan` by using free functions or member functions.

The free functions are defined in the `gf2` namespace as follows:

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

More commonly you create the iterators by calling member functions defined in `gf2::BitVector`, `gf2::BitArray`, and `gf2::BitSpan`.
For example, `gf2::BitVector` defines the following member functions:

| Method                              | Description                                                       |
| ----------------------------------- | ----------------------------------------------------------------- |
| `gf2::BitVector::bits const`        | Returns the **const** version of `gf2::Bits` for this vector.     |
| `gf2::BitVector::bits`              | Returns the **non-const** version of `gf2::Bits` for this vector. |
| `gf2::BitVector::set_bit_indices`   | Returns a `gf2::SetBits` for this vector.                         |
| `gf2::BitVector::unset_bit_indices` | Returns a `gf2::UnsetBits` for this vector.                       |
| `gf2::BitVector::store_words`       | Returns `gf2::Words` for this vector.                             |

### Example

```cpp
#include <gf2/namespace.h>
int main() {

    // Use the non-const `Bits` to set all the bits in a bit-vector
    auto v0 = BitVector<>::zeros(10);
    for (auto&& bit : v0.bits()) bit = true;
    std::println("v0: {}", v0);                             // "v0: 1111111111"

    // Exercise the `SetBits` & the `UnsetBitIter` to get indices of set and unset bits
    auto v1 = BitVector<u8>::alternating(10);
    auto set_locations = std::ranges::to<std::vector>(v1.set_bit_indices());
    auto unset_locations = std::ranges::to<std::vector>(v1.unset_bit_indices());
    std::println("Set indices:   {}", set_locations);       // "Set indices:   [0, 2, 4, 6, 8]"
    std::println("Unset indices: {}", unset_locations);     // "Unset indices: [1, 3, 5, 7, 9]"

    // Look at the underlying store words of a bit-vector
    auto v2 = BitVector<u8>::ones(10);
    auto words = std::ranges::to<std::vector>(v2.store_words());
    std::println("Store words: {::08b}", words);             // "Store words: [11111111, 00000011]"
}
```

## See Also

- `gf2::Bits` for detailed documentation on the bits iterator.
- `gf2::SetBits` for detailed documentation on the set-bits iterator.
- `gf2::UnsetBits` for detailed documentation on the unset-bits iterator.
- `gf2::Words` for detailed documentation on the words iterator.
- [`BitStore`](BitStore.md) for the concept API shared by all bit-stores.
- [`BitArray`](BitArray.md) for fixed-size vectors of bits.
- [`BitVector`](BitVector.md) for dynamically-sized vectors of bits.
- [`BitSpan`](BitSpan.md) for non-owning views into any bit-store.
- [`BitRef`](BitRef.md) for the proxy class that represents a single bit in a bit-store.
