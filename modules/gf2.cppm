// SPDX-FileCopyrightText: 2026 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// A module encompassing the entire gf2 library.

module;

#include <gf2/gf2.h>
#include <gf2/RNG.h>

export module gf2;

export namespace gf2 {
using gf2::BitArray;
using gf2::BitGauss;
using gf2::BitLU;
using gf2::BitMatrix;
using gf2::BitPolynomial;
using gf2::BitRef;
using gf2::Bits;
using gf2::BitSpan;
using gf2::BitStore;
using gf2::BitVector;
using gf2::RNG;
using gf2::SetBits;
using gf2::UnsetBits;
using gf2::Unsigned;
using gf2::Words;

using gf2::u16;
using gf2::u32;
using gf2::u64;
using gf2::u8;
using gf2::usize;

using gf2::all;
using gf2::any;
using gf2::back;
using gf2::bit_offset;
using gf2::bits;
using gf2::convolve;
using gf2::copy;
using gf2::count_ones;
using gf2::count_zeros;
using gf2::describe;
using gf2::dot;
using gf2::fill_random;
using gf2::first_set;
using gf2::first_unset;
using gf2::flip;
using gf2::flip_all;
using gf2::front;
using gf2::get;
using gf2::highest_set_bit;
using gf2::highest_unset_bit;
using gf2::index_and_mask;
using gf2::index_and_offset;
using gf2::is_empty;
using gf2::join;
using gf2::last_set;
using gf2::last_unset;
using gf2::leading_ones;
using gf2::leading_zeros;
using gf2::lowest_set_bit;
;
using gf2::lowest_unset_bit;
using gf2::next_set;
using gf2::next_unset;
using gf2::none;
using gf2::previous_set;
using gf2::previous_unset;
using gf2::ref;
using gf2::replace_bits;
using gf2::reset_bits;
using gf2::reset_except_bits;
using gf2::reverse_bits;
using gf2::riffle;
using gf2::set;
using gf2::set_all;
using gf2::set_bits;
using gf2::set_except_bits;
using gf2::span;
using gf2::split;
using gf2::store_words;
using gf2::sub;
using gf2::swap;
using gf2::to_binary_string;
using gf2::to_hex_string;
using gf2::to_pretty_string;
using gf2::to_string;
using gf2::to_words;
using gf2::trailing_ones;
using gf2::trailing_zeros;
using gf2::unset_bits;
using gf2::with_set_bits;
using gf2::with_unset_bits;
using gf2::word_index;
using gf2::words_needed;

using gf2::ALTERNATING;
using gf2::BITS;
using gf2::MAX;
using gf2::ONE;
using gf2::ZERO;

using gf2::assert_eq_failed;
using gf2::assert_failed;
using gf2::basename;

using gf2::operator&;
using gf2::operator&=;
using gf2::operator*;
using gf2::operator+;
using gf2::operator+=;
using gf2::operator-;
using gf2::operator-=;
using gf2::operator<<;
using gf2::operator<<=;
using gf2::operator>>;
using gf2::operator>>=;
using gf2::operator^;
using gf2::operator^=;
using gf2::operator|;
using gf2::operator|=;
using gf2::operator~;
} // namespace gf2

export namespace std {
using std::formatter;
} // namespace std
