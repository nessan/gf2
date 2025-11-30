# Store Iterators

## Introduction

The `<gf2/Iterators.h>` header provides five bit-store iterators that serve different purposes:

| Iterator             | Description                                                                                        |
| -------------------- | -------------------------------------------------------------------------------------------------- |
| `gf2::BitsIter`      | The const version reads individual bits from a bit-store one at a time.                            |
| `gf2::BitsIter`      | The non-const version reads and potentially writes individual bits from a bit-store one at a time. |
| `gf2::SetBitsIter`   | Iterates through the _index locations_ of the set bits in the associated bit-store.                |
| `gf2::UnsetBitsIter` | Iterates through the _index locations_ of the unset bits in the associated bit-store.              |
| `gf2::WordsIter`     | Allows reading and writing of "words" that underlie the bit-store.                                 |

The `gf2::BitsIter` comes in both const and non-const versions.
The `value_type` of the const version is `bool`, allowing you to read individual bits from the bit-store.
The `value_type` of the non-const version is `gf2::BitRef` that allows modification of individual bits as you iterate through them.

> [!NOTE]
> Const-ness here refers to whether the bits in the bit-store can be modified via the iterator, and not whether the iterator itself is const.

## Construction

You create these iterators by calling member functions defined in `gf2::BitStore`.

| Method                             | Description                                                          |
| ---------------------------------- | -------------------------------------------------------------------- |
| `gf2::BitStore::bits const`        | Returns the **const** version of `gf2::BitsIter` for this store.     |
| `gf2::BitStore::bits`              | Returns the **non-const** version of `gf2::BitsIter` for this store. |
| `gf2::BitStore::set_bit_indices`   | Returns a `gf2::SetBitsIter` for this store.                         |
| `gf2::BitStore::unset_bit_indices` | Returns a `gf2::UnsetBitsIter` for this store.                       |
| `gf2::BitStore::store_words`       | Returns `gf2::WordsIter` for this store.                             |

### Example

```cpp
#include <gf2/namespace.h>
#include <gf2/namespace.h>
int main() {

    // Use the non-const `BitsIter` to set all the bits in a bit-vector
    auto v0 = BitVec<>::zeros(10);
    for (auto&& bit : v0.bits()) bit = true;
    std::println("v0: {}", v0);                             // "v0: 1111111111"

    // Exercise the `SetBitsIter` & the `UnsetBitIter` to get indices of set and unset bits
    auto v1 = BitVec<u8>::alternating(10);
    auto set_locations = std::ranges::to<std::vector>(v1.set_bit_indices());
    auto unset_locations = std::ranges::to<std::vector>(v1.unset_bit_indices());
    std::println("Set indices:   {}", set_locations);       // "Set indices:   [0, 2, 4, 6, 8]"
    std::println("Unset indices: {}", unset_locations);     // "Unset indices: [1, 3, 5, 7, 9]"

    // Look at the underlying store words of a bit-vector
    auto v2 = BitVec<u8>::ones(10);
    auto words = std::ranges::to<std::vector>(v2.store_words());
    std::println("Words: {::08b}", words);                  // "Words: [11111111, 00000011]"
}
```

## See Also

-   `gf2::BitsIter` for detailed documentation on the bits iterator.
-   `gf2::SetBitsIter` for detailed documentation on the set-bits iterator.
-   `gf2::UnsetBitsIter` for detailed documentation on the unset-bits iterator.
-   `gf2::WordsIter` for detailed documentation on the words iterator.
-   [`BitStore`](BitStore.md) for the common API shared by all bit-stores.
-   [`BitSet`](BitSet.md) for fixed-size vectors of bits.
-   [`BitVec`](BitVec.md) for dynamically-sized vectors of bits.
-   [`BitSpan`](BitSpan.md) for non-owning views into any bit-store.
