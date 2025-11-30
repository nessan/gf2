#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// A `BitRef` is a proxy type that references a single bit in a `BitStore`. <br>
/// See the [BitRef](docs/pages/BitRef.md) page for more details.

#include <gf2/store_word.h>
#include <format>

namespace gf2 {

// Forward declaration of the `BitStore` class which generates `BitRef` instances from index operations.
template<typename Store>
class BitStore;

/// A `BitRef` is a *proxy* class to reference a single bit in a bit-store.
///
/// If `v` is any non-const bit-store then `v[i]` is a `BitRef` that references the bit at index `i` in `v`.
/// It behaves as a proxy for that element and allows us to set or get the bit value via the `=` operator.
///
/// The end user can also treat the `BitRef` object as a reference to a boolean value and use it in boolean
/// expressions.
///
/// @note The underlying bit-store must live as long as the `BitRef` that refers to it.
///
/// # Example
/// ```
/// BitVec v{3};
/// assert_eq(v.to_string(), "000");
/// v[1] = true;
/// assert_eq(v.to_string(), "010");
/// v[2] = 1;
/// assert_eq(v.to_string(), "011");
/// ```
template<typename Store>
class BitRef {
private:
    // The bit-store we are referencing.
    BitStore<Store>* m_store;

    // The index of the bit of interest within that bit-store
    usize m_index;

public:
    /// Constructs a reference to the bit at `index` in the given `BitStore`.
    BitRef(BitStore<Store>* store, usize index) : m_store(store), m_index(index) {}

    // The default compiler generator implementations for most "rule of 5" methods will be fine ...
    constexpr BitRef() = default;
    constexpr BitRef(const BitRef&) = default;
    constexpr BitRef(BitRef&&) noexcept = default;
    ~BitRef() = default;

    /// Returns the boolean value of the bit at `index` in the referenced bit-store.
    constexpr operator bool() const { return m_store->get(m_index); }

    /// Sets the bit at `index` in the referenced bit-store to the given value.
    constexpr BitRef& operator=(bool rhs) {
        m_store->set(m_index, rhs);
        return *this;
    }

    /// Sets the bit at `index` in the referenced bit-store to the value of the given `BitRef`.
    constexpr BitRef& operator=(const BitRef& rhs) {
        m_store->set(m_index, static_cast<bool>(rhs));
        return *this;
    }

    /// Flips the bit at `index` in the referenced bit-store.
    constexpr BitRef& flip() {
        m_store->flip(m_index);
        return *this;
    }

    /// Performs an `AND` operation on the bit at `index` in the referenced bit-store with `rhs`.
    constexpr BitRef& operator&=(bool rhs) {
        if (!rhs) m_store->set(m_index, false);
        return *this;
    }

    /// Performs an `AND` operation on the bit at `index` in the referenced bit-store with `rhs`.
    constexpr BitRef& operator&=(const BitRef& rhs) {
        if (!static_cast<bool>(rhs)) m_store->set(m_index, false);
        return *this;
    }

    /// Performs an `OR` operation on the bit at `index` in the referenced bit-store with `rhs`.
    constexpr BitRef& operator|=(bool rhs) {
        if (rhs) m_store->set(m_index);
        return *this;
    }

    /// Performs an `OR` operation on the bit at `index` in the referenced bit-store with `rhs`.
    constexpr BitRef& operator|=(const BitRef& rhs) {
        if (static_cast<bool>(rhs)) m_store->set(m_index);
        return *this;
    }

    /// Performs an `XOR` operation on the bit at `index` in the referenced bit-store with `rhs`.
    constexpr BitRef& operator^=(bool rhs) {
        if (rhs) m_store->flip(m_index);
        return *this;
    }

    /// Performs an `XOR` operation on the bit at `index` in the referenced bit-store with `rhs`.
    constexpr BitRef& operator^=(const BitRef& rhs) {
        if (static_cast<bool>(rhs)) m_store->flip(m_index);
        return *this;
    }
};

} // namespace gf2

/// Specialize `std::formatter` for `BitRef<Store>`
///
/// Apart from anything else, this allows `gf2_assert_eq` to work happily with `BitRef` instances.
template<typename Store>
struct std::formatter<gf2::BitRef<Store>> : std::formatter<bool> {
    template<typename FormatContext>
    auto format(const gf2::BitRef<Store>& bit, FormatContext& ctx) const {
        return std::formatter<bool>::format(static_cast<bool>(bit), ctx);
    }
};