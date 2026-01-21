#pragma once
// SPDX-FileCopyrightText: 2025 Nessan Fitzmaurice <nzznfitz+gh@icloud.com>
// SPDX-License-Identifier: MIT

/// @file
/// An "include everything" header for the `gf2` library that also exposes the namespace `gf2`.

/// @namespace gf2
/// The namespace for the `gf2` library.
namespace gf2 {
}

// Include everything ...
#include <gf2/gf2.h>

// Expose the namespace
using namespace gf2;