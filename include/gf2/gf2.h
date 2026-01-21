#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// An include-the-lot header file for the `gf2` library.

#pragma once

// Utilities.
#include <gf2/assert.h>
#include <gf2/Unsigned.h>

// The vector-like types & the bit-store concept they all satisfy.
#include <gf2/BitStore.h>
#include <gf2/BitArray.h>
#include <gf2/BitVec.h>
#include <gf2/BitSpan.h>

// The bit reference type to access individual bits
#include <gf2/BitRef.h>

// Iterators over bit-stores: `Bits`, `SetBits`, `UnsetBits`, `Words`
#include <gf2/Iterators.h>

// The polynomial type
#include <gf2/BitPoly.h>

// tTe matrix types & helpers
#include <gf2/BitMat.h>
#include <gf2/BitLU.h>
#include <gf2/BitGauss.h>
