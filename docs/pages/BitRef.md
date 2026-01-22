# The `BitRef` Reference

## Introduction

A `gf2::BitRef` is a _proxy class_ that lets you address a single bit in any `gf::BitStore` object.
It behaves like a reference to a boolean value, but under the hood it reads and writes the appropriate bit in the underlying bit-store.

You get a `gf2::BitRef` from using the non-const version of `operator[]` on `BitVector`, `BitArray`, `BitSpan`, and friends.

## Example

```cpp
gf2::BitVector v{5};
v[0] = true;        // writes through BitRef
v[3].flip();        // mutate in-place
bool b = v[0];      // implicit conversion to bool
v[1] ^= b;          // compound op stays inside the store
assert_eq(v.to_string(), "10100");
```

You can use a `BitRef` in several ways:

- Treat it like a bool: it converts implicitly for reads, so you can use it in conditionals or arithmetic on GF(2).
- Assign to it: `v[i] = true`, `v[i] = other[i]`, and the compound assignments `&=`, `|=`, `^=` all work in-place.
- Flip bits: call `bit.flip()` or use `^= true` to toggle.

> [!WARNING]
> Because a `gf2::BitRef` keeps a pointer back to the underlying `gf2::BitStore`, that store must outlive the proxy.
> Outside of that lifetime guarantee, you can freely pass a `BitRef` by value -- they are tiny and intentionally lightweight.

## See Also

- [`BitStore`](BitStore.md) for the concept API shared by all bit-stores.
- [`BitArray`](BitArray.md) for fixed-size vectors of bits.
- [`BitVector`](BitVector.md) for dynamically-sized vectors of bits.
- [`BitSpan`](BitSpan.md) for non-owning views into any bit-store.
- [`BitMatrix`](BitMatrix.md) for matrices of bits.
