#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// An include-the-lot header file for the `gf2` library.

#pragma once

// Utilities.
#include <gf2/assert.h>
#include <gf2/store_word.h>

// The bit-store base class and its derived vector-like classes.
#include <gf2/BitStore.h>
#include <gf2/BitSet.h>
#include <gf2/BitVec.h>
#include <gf2/BitSpan.h>

// Iterators and bit references for bit-stores.
#include <gf2/Iterators.h>
#include <gf2/BitRef.h>

// Polynomials over GF(2).
#include <gf2/BitPoly.h>

// Bit-matrices and related algorithms.
#include <gf2/BitMat.h>
#include <gf2/BitGauss.h>
#include <gf2/BitLU.h>
