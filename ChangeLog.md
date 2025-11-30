# Changelog

This file documents notable changes to the project.

## 10-Dec-2025

-   Complete rewrite --- renamed the library from `bit` to `gf2`.
-   Classes and functions have been moved into the `gf2` namespace.
-   A lot of the changes were inspired by the Rust version of the library: `gf2-rs`.
-   Added `gf2::BitStore` as a common interface for the vector-like classes:
    -   `gf2::BitSet` for fixed-size vectors of bits (backed by `std::array`).
    -   `gf2::BitVec` for dynamic-size vectors of bits (backed by `std::vector`).
    -   `gf2::BitSpan` for non-owning views into any bit-store.
-   Added iterators for bits, set bits, unset bits, and store words for all bit-stores.
-   `gf2::BitRef` allows references to individual bits.
-   `gf2::BitMat` and associated algorithms for matrices over GF(2).
-   Documentation now uses Doxygen with a custom theme.

## 30-Apr-2024

-   Moved the repo to github.
-   The documentation/site-builder was moved from AsciiDoc/Antora to Markdown/Quarto.
-   Added the `bit::polynomial` class for polynomials over GF(2).
-   Incorporated various fixes and speed-ups from @jason.

## Earlier (2022-23)

-   The library started as `GF2` which evolved to the `bit` library hosted on GitLab.
