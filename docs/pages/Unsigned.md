# Unsigned Words

## Introduction

The `<gf2/Unsigned.h>` header provides utility functions for any [unsigned type].

The `gf2` library operates by packing individual bit elements into one of the primitive unsigned words that are natively addressed by computer.
We have a number of utility functions that operate on those words.

## Concept

The `gf2::Unsigned` concept is satisfied by any of the computer's primitive unsigned types.
It is identical to [`std::unsigned_integral`] from the C++ standard library but with a better name for our purposes.

## Type Aliases

We define the following type aliases for all the common unsigned word types:

| Type         | Description                           |
| ------------ | ------------------------------------- |
| `gf2::u8`    | An unsigned 8-bit integer --- a byte. |
| `gf2::u16`   | An unsigned 16-bit integer.           |
| `gf2::u32`   | An unsigned 32-bit integer.           |
| `gf2::u64`   | An unsigned 64-bit integer.           |
| `gf2::usize` | An unsigned native-sized integer.     |

The short names are match those used by [Rust](https://doc.rust-lang.org/std/#primitives) for its primitive unsigned types.

## Variables

We supply some convenience variables for any `gf2::Unsigned` type:

| Variable           | Description                                                 |
| ------------------ | ----------------------------------------------------------- |
| `gf2::BITS`        | The number of binary digits in the referenced type.         |
| `gf2::ZERO`        | The word whose bits are all zero.                           |
| `gf2::ONE`         | The word whose lowest order bit is 1 and the rest are zero. |
| `gf2::MAX`         | The word whose bits are all one.                            |
| `gf2::ALTERNATING` | The word whose bits are alternating zeros and ones.         |

These are all template variables, for example `BITS<u8>` returns 8.

## Constructors

We have template functions to create a `gf2::Unsigned` that has a given range of bits set or unset:

| Constructor            | Description                                                                              |
| ---------------------- | ---------------------------------------------------------------------------------------- |
| `gf2::with_set_bits`   | Returns the word that is all zeros _except_ for the bits in the given _half-open_ range. |
| `gf2::with_unset_bits` | Returns the word that is all ones _except_ for the bits in the given _half-open_ range.  |

### Example

```cpp
#include <gf2/namespace.h>
int main() {
    auto w1 = with_set_bits<u8>(2, 5);                  // set bits in range [2, 5) to 1
    auto w2 = with_unset_bits<u8>(1, 4);                // set bits in range [1, 4) to 0
    std::println("w1 = {:08b}, w2 = {:08b}", w1, w2);   // Output: w1 = 00011100, w2 = 11110001
}
```

## Mutators

We have template functions that take a `gf2::Unsigned` first argument and mutate its bits:

| Mutator                  | Description                                                                                                 |
| ------------------------ | ----------------------------------------------------------------------------------------------------------- |
| `gf2::set_bits`          | Sets the bits in a given half-open range to one.                                                            |
| `gf2::reset_bits`        | Sets the bits in a given half-open range to zero.                                                           |
| `gf2::set_except_bits`   | Sets the bits _except_ those in a given half-open range to one.                                             |
| `gf2::reset_except_bits` | Sets the bits _except_ those in a given half-open range to zero.                                            |
| `gf2::replace_bits`      | Copies some bits from another unsigned word to this one.                                                    |
| `gf2::reverse_bits`      | Returns a copy of the argument with all its bits reversed.                                                  |
| `gf2::riffle`            | Riffles the argument into a pair of others containing the bits in the original word interleaved with zeros. |

### Example

```cpp
#include <gf2/namespace.h>
int main() {
    // Set some bits and reversing them
    u8 w0 = 0b0000'0000;
    set_bits(w0, 1, 3);
    auto w1 = reverse_bits(w0);
    std::println("w0: {:08b}, w1: {:08b}", w0, w1); // w0: 00000110, w1: 01100000

    // Replacing bits
    w0 = 0b1111'1111;
    w1 = 0b0000'0000;
    replace_bits(w0, 2, 5, w1);
    std::println("w0: {:08b}", w0); // w0: 11100011

    // Riffling a word into two others
    w0 = 0b1111'1111;
    auto [lo, hi] = riffle(w0);
    std::println("w0: {:08b}, lo: {:08b}, hi: {:08b}", w0, lo, hi); // w0: 11111111, lo: 01010101, hi: 01010101
}
```

## Bit Counts

We have template functions that take a `gf2::Unsigned` argument and queries the number of set or unset bits:

| Count Function        | Description                                       |
| --------------------- | ------------------------------------------------- |
| `gf2::count_ones`     | Returns the number of bits set to one.            |
| `gf2::count_zeros`    | Returns the number of bits set to zero.           |
| `gf2::trailing_zeros` | Returns the number of low-order zero bits.        |
| `gf2::leading_zeros`  | Returns the number of high-order zero bits.       |
| `gf2::trailing_ones`  | Returns the number of low-order bits set to one.  |
| `gf2::leading_ones`   | Returns the number of high-order bits set to one. |

## Search

We have methods that take an unsigned word and return the bit location of the first set bit, the first unset bit, etc.

| Search Function          | Description                                                  |
| ------------------------ | ------------------------------------------------------------ |
| `gf2::lowest_set_bit`    | Returns the index of the lowest-order set bit in the word.   |
| `gf2::highest_set_bit`   | Returns the index of the highest-order set bet in the word.  |
| `gf2::lowest_unset_bit`  | Returns the index of the lowest-order zero bit in the word.  |
| `gf2::highest_unset_bit` | Returns the index of the highest-order zero bit in the word. |

## Stringification

We have methods that take an unsigned word and return a formatted string.

| String Function         | Description                                                      |
| ----------------------- | ---------------------------------------------------------------- |
| `gf2::to_binary_string` | Returns the binary string representation showing _all_ the bits. |
| `gf2::to_hex_string`    | Returns the hex string representation showing _all_ the bits.    |

## Bit Location

We have methods that help locate specific bits within a sequence of unsigned words.

| Locater Function        | Description                                                                                            |
| ----------------------- | ------------------------------------------------------------------------------------------------------ |
| `gf2::words_needed`     | How many words of a specific type is needed to store the passed number of bits.                        |
| `gf2::word_index`       | Which word in a sequence of words holds the bit at the given index.                                    |
| `gf2::bit_offset`       | Where in the containing word is this specific bit?                                                     |
| `gf2::index_and_offset` | Returns a [`std::pair`] that encompasses the last two results in one call.                             |
| `gf2::index_and_mask`   | Returns a [`std::pair`] --- the index of the containing word and a mask that to isolate the given bit. |

The last two methods can be most conveniently used with structured bindings.

### Example

```cpp
#include <gf2/namespace.h>
int main() {
    using word_type = gf2::u8;
    usize bit_index = 19;
    auto [wi, offset] = gf2::index_and_offset<word_type>(bit_index);
    std::println("Bit {} is in word {} at offset {}", bit_index, wi, offset);
    auto [wj, mask] = gf2::index_and_mask<word_type>(bit_index);
    std::println("Bit {} is in word {} with isolator mask {:08b}", bit_index, wj, mask);
    return 0;
}
```

which outputs:

```txt
Bit 19 is in word 2 at offset 3
Bit 19 is in word 2 with isolator mask 00001000
```

<!-- Reference Links -->

[unsigned type]: https://en.cppreference.com/w/cpp/concepts/unsigned_integral
[`std::unsigned_integral`]: https://en.cppreference.com/w/cpp/concepts/unsigned_integral
[`std::pair`]: https://en.cppreference.com/w/cpp/utility/pair
