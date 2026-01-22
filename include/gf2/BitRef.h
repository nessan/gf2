#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// A `BitRef` is a proxy type that references a single bit in a bit-store. <br>
/// See the [BitRef](docs/pages/BitRef.md) page for more details.

#include <gf2/BitStore.h>

namespace gf2 {

/// A `BitRef` is a *proxy* class to reference a single bit in a bit-store.
///
/// If `v` is any non-const bit-store then `v[i]` is a `BitRef` that "references" the bit at index `i` in `v`.
/// It behaves as a proxy for that element and allows us to set or get the bit value via the `=` operator.
///
/// The end user can also treat the `BitRef` object as a reference to a boolean value and use it in boolean
/// expressions.
///
/// @note The underlying bit-store must live as long as the `BitRef` that refers to it.
///
/// # Example
/// ```
/// BitVector v{3};
/// assert_eq(v.to_string(), "000");
/// v[1] = true;
/// assert_eq(v.to_string(), "010");
/// v[2] = 1;
/// assert_eq(v.to_string(), "011");
/// ```
template<BitStore Store>
class BitRef {
private:
    // The bit-store we are referencing.
    Store* m_store;

    // The index of the bit of interest within that bit-store
    usize m_index;

public:
    /// Constructs a reference to the bit at `index` in the given bit-store.
    BitRef(Store* store, usize index) : m_store(store), m_index(index) {}

    // The default compiler generator implementations for most "rule of 5" methods will be fine ...
    constexpr BitRef() = default;
    constexpr BitRef(BitRef const&) = default;
    constexpr BitRef(BitRef&&) noexcept = default;
    ~BitRef() = default;

    /// Returns the boolean value of the bit at `index` in the referenced bit-store.
    ///
    /// # Example
    /// ```
    /// BitVector v{3};
    /// auto bit = ref(v, 0);
    /// assert_eq(static_cast<bool>(bit), false);
    /// ```
    constexpr operator bool() const { return gf2::get(*m_store, m_index); }

    /// Sets the bit at `index` in the referenced bit-store to the given value.
    ///
    /// # Example
    /// ```
    /// BitVector v{3};
    /// ref(v, 0) = true;
    /// assert_eq(to_string(v), "100");
    /// ```
    constexpr BitRef& operator=(bool rhs) {
        gf2::set(*m_store, m_index, rhs);
        return *this;
    }

    /// Sets the bit at `index` in the referenced bit-store to the value of the given `BitRef`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<>::zeros(3);
    /// auto w = BitVector<>::ones(3);
    /// ref(v, 0) = ref(w, 0);
    /// assert_eq(to_string(v), "100");
    /// ref(v, 1) = ref(w, 1);
    /// assert_eq(to_string(v), "110");
    /// ref(v, 2) = ref(w, 2);
    /// assert_eq(to_string(v), "111");
    /// ```
    constexpr BitRef& operator=(BitRef const& rhs) {
        gf2::set(*m_store, m_index, static_cast<bool>(rhs));
        return *this;
    }

    /// Flips the bit at `index` in the referenced bit-store.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<>::zeros(3);
    /// ref(v, 0).flip();
    /// assert_eq(to_string(v), "100");
    /// ```
    constexpr BitRef& flip() {
        gf2::flip(*m_store, m_index);
        return *this;
    }

    /// Performs an `AND` operation on the bit at `index` in the referenced bit-store with `rhs`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<>::ones(3);
    /// ref(v, 0) &= false;
    /// assert_eq(to_string(v), "011");
    /// ```
    constexpr BitRef& operator&=(bool rhs) {
        if (!rhs) gf2::set(*m_store, m_index, false);
        return *this;
    }

    /// Performs an `AND` operation on the bit at `index` in the referenced bit-store with `rhs`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<>::ones(3);
    /// auto w = BitVector<>::zeros(3);
    /// ref(v, 0) &= ref(w, 0);
    /// assert_eq(to_string(v), "011");
    /// ```
    constexpr BitRef& operator&=(BitRef const& rhs) {
        if (!static_cast<bool>(rhs)) gf2::set(*m_store, m_index, false);
        return *this;
    }

    /// Performs an `OR` operation on the bit at `index` in the referenced bit-store with `rhs`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<>::zeros(3);
    /// ref(v, 0) |= true;
    /// assert_eq(to_string(v), "100");
    /// ```
    constexpr BitRef& operator|=(bool rhs) {
        if (rhs) gf2::set(*m_store, m_index);
        return *this;
    }

    /// Performs an `OR` operation on the bit at `index` in the referenced bit-store with `rhs`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<>::zeros(3);
    /// auto w = BitVector<>::ones(3);
    /// ref(v, 0) |= ref(w, 0);
    /// assert_eq(to_string(v), "100");
    /// ```
    constexpr BitRef& operator|=(BitRef const& rhs) {
        if (static_cast<bool>(rhs)) gf2::set(*m_store, m_index);
        return *this;
    }

    /// Performs an `XOR` operation on the bit at `index` in the referenced bit-store with `rhs`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<>::ones(3);
    /// ref(v, 0) ^= true;
    /// assert_eq(to_string(v), "011");
    /// ```
    constexpr BitRef& operator^=(bool rhs) {
        if (rhs) gf2::flip(*m_store, m_index);
        return *this;
    }

    /// Performs an `XOR` operation on the bit at `index` in the referenced bit-store with `rhs`.
    ///
    /// # Example
    /// ```
    /// auto v = BitVector<>::ones(3);
    /// auto w = BitVector<>::ones(3);
    /// ref(v, 0) ^= ref(w, 0);
    /// assert_eq(to_string(v), "011");
    /// ```
    constexpr BitRef& operator^=(BitRef const& rhs) {
        if (static_cast<bool>(rhs)) gf2::flip(*m_store, m_index);
        return *this;
    }
};

} // namespace gf2

namespace std {

/// Specialize `std::formatter` for a bit-reference.
template<gf2::BitStore Store>
struct formatter<gf2::BitRef<Store>> : formatter<bool> {
    template<typename FormatContext>
    auto format(gf2::BitRef<Store> const& bit, FormatContext& ctx) const {
        return formatter<bool>::format(static_cast<bool>(bit), ctx);
    }
};

} // namespace std
