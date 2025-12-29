#pragma once
// Internal file for the gf2 library.
//
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <random>

namespace gf2::details {

// A trivial `SplitMix64` random number generator for internal use in the `gf2` library.
class SplitMix64 {
public:
    // Constructs a new `SplitMix64` random number generator seeded from a random device.
    SplitMix64() { seed_from_entropy(); }

    // Constructs a new `SplitMix64` random number generator with the given seed.
    SplitMix64(std::uint64_t seed) : m_state(seed) {}

    // Returns the next random 64-bit unsigned integer.
    constexpr std::uint64_t u64() { return next(); }

    // Advances the state of the random number generator and returns the next random 64-bit unsigned integer.
    constexpr std::uint64_t next() {
        std::uint64_t z = (m_state += 0x9e3779b97f4a7c15);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        z = (z ^ (z >> 31));
        return z;
    }

    // Sets the seed to the given value.
    constexpr void set_seed(std::uint64_t new_seed) { m_state = new_seed; }

    // Returns the current seed.
    constexpr std::uint64_t seed() const { return m_state; }

    // Sets the state from a random device.
    void seed_from_entropy() {
        // Some std::random_device implementations will return a 32-bit unsigned integer.
        std::random_device rd;
        if constexpr (sizeof(std::uint64_t) <= sizeof(std::random_device::result_type)) {
            m_state = static_cast<std::uint64_t>(rd());
        } else {
            m_state = static_cast<std::uint64_t>(rd()) << 32 | rd();
        }
    }

private:
    std::uint64_t m_state;
};

} // namespace gf2::details

namespace gf2 {

// The default random number generator type used in the `gf2` library for generating random bit-vectors & matrices.
// We generally just use `RNG` in code as a type alias for whichever RNG we are actually using.
using RNG = gf2::details::SplitMix64;

} // namespace gf2