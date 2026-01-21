# The `BitMat` Class

## Introduction

A `gf2::BitMat` is a dynamically sized matrix of bit elements.

In mathematical terms, a bit-matrix is a matrix over [GF2], the simplest [Galois-Field] with just two elements, usually denoted 0 & 1, as the booleans true & false, or as the bits set & unset.
Arithmetic over GF(2) is mod 2, so addition/subtraction becomes the `XOR` operation while multiplication/division becomes `AND`.

> [!NOTE]
> Operations on and between bit-matrices and other objects in the `gf2` library are implemented using bitwise operations on whole underlying words at a time.
> These operations are highly optimised in modern CPUs, allowing for fast computation even on large bit-vectors.
> It also means we never have to worry about overflows or carries as we would with normal integer arithmetic.

A bit-matrix is stored in _row-major mode_ where each row is a single `gf2::BitVec`.
This means that arranging computations to work row by row instead of column by column is typically much more efficient.
The methods and functions in the library take this into consideration.

This bit-matrix class is a [`std::vector`] of rows where each row is a single bit-vector.
If the primary aim was to minimize storage, we would store the bit-matrix as a single long bit-vector with appropriate index operations.
However, in that case, matrix operations would often need to be done across word boundaries, which is much slower than doing things word-by-word.

> [!NOTE]
> Arbitrary $m \times n$ bit-matrices are supported, but some functions only make sense for square matrices where $n = m$.

The `gf2::BitMat` class has many of the same methods defined for `gf2::BitVec`, such as bitwise operations and stringification.
We also have methods for matrix-vector, vector-matrix, and matrix-matrix multiplication.

There are methods to solve linear systems of equations $A \cdot x = b$.

Danilevsky's method to compute characteristic polynomials (and the determinant) for a bit-matrix is available and works for quite large matrices (ones with millions of entries) that would choke a naive implementation that didn't take into account the nature of arithmetic over $\mathbb{F}_2$.

## Declaration

The declaration looks like this:

```cpp
template <Unsigned Word = usize>
class BitMat {
 // ...
};
```

The `Word` template parameter specifies the underlying unsigned integer type used by the row bit-vectors to store the bits elements, and the default is usually the most efficient type for the target platform.
On most modern platforms, that [`usize`] will be a 64-bit unsigned integer.

If your application calls for a vast number of small bit-matrices, you might consider using `std::uint8_t` as the `Word` type to save memory.

> [!NOTE]
> You might notice that many of the [doctests] in the library use 8-bit underlying words.
> The reason is we want to exercise the various functions across word boundaries for modest, easily readable bit-stores.

## Constructors

You can create a bit-matrix with the following constructors:

| Method Name                                         | Description                                     |
| --------------------------------------------------- | ----------------------------------------------- |
| `gf2::BitMat::BitMat`                               | Creates a matrix with a given size.             |
| `gf2::BitMat::BitMat(const std::vector<row_type>&)` | Creates a matrix by _copying_ a vector of rows. |
| `gf2::BitMat::BitMat(std::vector<BitVec<Word>>&&)`  | Creates a matrix by _moving_ a vector of rows.  |

If you create a matrix with a given size, all the bits are initialised to zero.
The number of rows and columns are specified as parameters, the default constructor creates an empty matrix with zero rows and zero columns.

If you create a matrix from a vector of rows, we check that all the rows have the same length and throw an exception if they do not. That check can be disabled by setting the `NDEBUG` preprocessor symbol.

If you move a vector of rows into the matrix, they are no longer valid after the move.

## Factory Constructors

The `BitMat` class provides a rich set of methods for constructing bit-matrices:

### Common Matrices

| Method Name                       | Description                                                  |
| --------------------------------- | ------------------------------------------------------------ |
| `gf2::BitMat::zeros`              | Creates the zero with all elements set to 0.                 |
| `gf2::BitMat::ones`               | Creates the matrix with all elements set to 1.               |
| `gf2::BitMat::alternating`        | Creates the matrix with elements in a checker-board pattern. |
| `gf2::BitMat::from_outer_product` | Creates a matrix as the outer product of two bit-vectors.    |
| `gf2::BitMat::from_outer_sum`     | Creates a matrix as the outer sum of two bit-vectors.        |
| `gf2::BitMat::from`               | Creates a matrix by repeatedly invoking a function $f(i,j)$. |
| `gf2::BitMat::random`             | Creates a matrix with a random fill.                         |
| `gf2::BitMat::seeded_random`      | Creates a matrix with a repeatable random fill.              |
| `gf2::BitMat::biased_random`      | Creates a matrix with a biased random fill.                  |

Some of these methods come in two versions -- one that takes the number of rows and columns as parameters, and another that creates square matrices of a given size.

### Special Matrices

We have methods to create some special matrices:

| Method Name                   | Description                                                         |
| ----------------------------- | ------------------------------------------------------------------- |
| `gf2::BitMat::zero`           | The square zero matrix.                                             |
| `gf2::BitMat::identity`       | The square identity matrix.                                         |
| `gf2::BitMat::left_shift`     | The `n x n` shift-left by `p` places matrix.                        |
| `gf2::BitMat::right_shift`    | The `n x n` shift-right by `p` places matrix.                       |
| `gf2::BitMat::left_rotation`  | The `n x n` rotate-left by `p` places matrix.                       |
| `gf2::BitMat::right_rotation` | The `n x n` rotate-right by `p` places matrix.                      |
| `gf2::BitMat::companion`      | A square matrix with a top-row and a sub-diagonal that is all ones. |

### Reshaping from Vectors

We have methods to reshape a bit-vector into a matrix, either by rows or by columns:

| Method Name                        | Description                                     |
| ---------------------------------- | ----------------------------------------------- |
| `gf2::BitMat::from_vector_of_rows` | Reshapes a bit-vector of rows into a matrix.    |
| `gf2::BitMat::from_vector_of_cols` | Reshapes a bit-vector of columns into a matrix. |

These can fail so we return a `BitMat` wrapped in a `std::optional` and `std::nullopt` if the size of the vector is not compatible with the requested matrix shape.

### From Strings

Finally, we have a method to construct a bit-matrix from one of its string representations:

| Method Name                | Description                             |
| -------------------------- | --------------------------------------- |
| `gf2::BitMat::from_string` | Tries to create a matrix from a string. |

This can fail so return a a `BitMat` wrapped in a `std::optional` and `std::nullopt` if we fail to parse the string.

The rows in the matrix string can be separated by newlines, white space, commas, single quotes, or semicolons.
Each row should be a binary or hex string representation of a bit-vector.
See [constructing bit-vectors from strings](BitVec.md#construction-from-strings) for details of the accepted formats for each row.

## Resizing and Reshaping

We have methods to resize the matrix where current elements are preserved as much as possible and any added elements are initialised to zero.

| Method Name                | Description                                  |
| -------------------------- | -------------------------------------------- |
| `gf2::BitMat::resize`      | Resizes the matrix to the passed dimensions. |
| `gf2::BitMat::clear`       | This is equivalent to `resize(0,0)`.         |
| `gf2::BitMat::make_square` | This is equivalent to `resize(m,m)`.         |

## Appending and Removing Rows/Columns

We have methods to append and remove rows and columns from the matrix:

| Method Name                | Description                                                                                         |
| -------------------------- | --------------------------------------------------------------------------------------------------- |
| `gf2::BitMat::append_row`  | Appends a row to the end of the matrix either by copying or moving a bit-vector.                    |
| `gf2::BitMat::append_rows` | Appends rows to the end of the matrix either by copying or moving a standard vector of bit-vectors. |
| `gf2::BitMat::append_col`  | Appends a column to the right of the matrix by copying a bit-vector.                                |
| `gf2::BitMat::append_cols` | Appends columns to the right of the matrix by copying standard vector of bit-vectors.               |
| `gf2::BitMat::remove_row`  | Removes a row from the end of the matrix and returns it as a bit-vector.                            |
| `gf2::BitMat::remove_rows` | Removes multiple rows from the end of the matrix and returns them as a bit-matrix.                  |
| `gf2::BitMat::remove_col`  | Removes a column from the right of the matrix and returns it as a bit-vector.                       |

The `remove` methods return an `std::optional` wrapped result as the request can fail.

## Size and Type Queries

We have methods to query the matrix dimensions and check if it is "special" in some way:

| Method Name                 | Description                                                              |
| --------------------------- | ------------------------------------------------------------------------ |
| `gf2::BitMat::rows`         | Returns the number of rows in the matrix.                                |
| `gf2::BitMat::cols`         | Returns the number of columns in the matrix.                             |
| `gf2::BitMat::size`         | Returns the number of elements in the matrix.                            |
| `gf2::BitMat::is_empty`     | Returns `true` if the matrix has no elements.                            |
| `gf2::BitMat::is_square`    | Returns `true` if the number of rows equals the number of columns.       |
| `gf2::BitMat::is_zero`      | Returns `true` if this is a _square_ zero matrix.                        |
| `gf2::BitMat::is_identity`  | Returns `true` if this is a _square_ identity matrix.                    |
| `gf2::BitMat::is_symmetric` | Returns `true` if this is a _square_ symmetric matrix $M(i,j) = M(j,i)$. |

## Bit Counts

We have methods to count the number of set and unset elements in the matrix, as well as on the main diagonal.

| Method Name                           | Description                                                      |
| ------------------------------------- | ---------------------------------------------------------------- |
| `gf2::BitMat::count_ones`             | Returns the overall number of set elements in the matrix.        |
| `gf2::BitMat::count_zeros`            | Returns the overall number of unset elements in the matrix.      |
| `gf2::BitMat::count_ones_on_diagonal` | Returns the overall number of set elements on the main diagonal. |
| `gf2::BitMat::trace`                  | Returns the "sum" of the main diagonal elements.                 |
| `gf2::BitMat::any`                    | Returns `true` if the matrix has any set elements.               |
| `gf2::BitMat::all`                    | Returns `true` if the matrix is all ones.                        |
| `gf2::BitMat::none`                   | Returns `true` if the matrix is all zeros.                       |

## General Access

There are methods to access individual elements, rows and columns, and the entire matrix.

| Method Name                 | Description                                                             |
| --------------------------- | ----------------------------------------------------------------------- |
| `gf2::BitMat::get`          | Access an individual matrix element either as a bool or a `gf2::BitRef` |
| `gf2::BitMat::operator()()` | Access an individual matrix element either as a bool or a `gf2::BitRef` |
| `gf2::BitMat::set`          | Set an individual matrix element to a value that defaults to `true`     |
| `gf2::BitMat::flip`         | Flips the value of an individual matrix element.                        |
| `gf2::BitMat::row`          | Returns a _reference_ to a matrix row as a `gf2::BitVec`.               |
| `gf2::BitMat::operator[]()` | Returns a _reference_ to a matrix row as a `gf2::BitVec`.               |
| `gf2::BitMat::col`          | Returns a _copy_ of a matrix column as a `gf2::BitVec`.                 |
| `gf2::BitMat::set_all`      | Sets all matrix elements to a value that defaults to `true`             |
| `gf2::BitMat::flip_all`     | Flips the values of all matrix elements.                                |

### Diagonal Access

We have methods to access and modify the main diagonal, super-diagonals, and sub-diagonals of a square matrix:

| Method Name                        | Description                                                                       |
| ---------------------------------- | --------------------------------------------------------------------------------- |
| `gf2::BitMat::set_diagonal`        | Sets all the main diagonal elements to a value that defaults to `true`            |
| `gf2::BitMat::flip_diagonal`       | Flips the values of all the main diagonal elements.                               |
| `gf2::BitMat::set_super_diagonal`  | Sets all elements of a super-diagonal elements to a value that defaults to `true` |
| `gf2::BitMat::flip_super_diagonal` | Flips the values of all elements on a super-diagonal.                             |
| `gf2::BitMat::set_sub_diagonal`    | Sets all elements of a sub-diagonal elements to a value that defaults to `true`   |
| `gf2::BitMat::flip_sub_diagonal`   | Flips the values of all elements on a sub-diagonal.                               |

## Sub-Matrices

There are methods to extract and replace sub-matrices:

| Method Name                       | Description                                |
| --------------------------------- | ------------------------------------------ |
| `gf2::BitMat::sub_matrix`         | Extracts a sub-matrix as a new bit-matrix. |
| `gf2::BitMat::replace_sub_matrix` | Replaces a sub-matrix.                     |

These methods panic if the requested sub-matrix is out of bounds.

### Triangular Sub-Matrices

We have methods to extract triangular sub-matrices:

| Method Name                   | Description                                                             |
| ----------------------------- | ----------------------------------------------------------------------- |
| `gf2::BitMat::lower`          | Returns the lower triangular sub-matrix including the main diagonal.    |
| `gf2::BitMat::upper`          | Returns the upper triangular sub-matrix including the main diagonal.    |
| `gf2::BitMat::strictly_lower` | Returns the lower triangular sub-matrix excluding the main diagonal.    |
| `gf2::BitMat::strictly_upper` | Returns the upper triangular sub-matrix excluding the main diagonal.    |
| `gf2::BitMat::unit_lower`     | Returns the lower triangular sub-matrix with ones on the main diagonal. |
| `gf2::BitMat::unit_upper`     | Returns the upper triangular sub-matrix with ones on the main diagonal. |

The triangular extraction methods do not need the matrix to be square.
The returned sub-matrix will have the same number of rows and columns as the original matrix, with the appropriate elements zeroed out.

## Elementary Operations

We have methods to perform elementary row and column operations as well as adding the identity matrix in-place:

| Method Name                 | Description                                           |
| --------------------------- | ----------------------------------------------------- |
| `gf2::BitMat::swap_rows`    | Swaps two rows in a matrix.                           |
| `gf2::BitMat::swap_cols`    | Swap two columns in a matrix.                         |
| `gf2::BitMat::add_identity` | Adds the identity matrix to a square matrix in-place. |

These operations are fundamental to many matrix algorithms.

## Transposition

We have methods to transpose a matrix either in-place (for square matrices) or to return a new transposed matrix:

| Method Name               | Description                                                                |
| ------------------------- | -------------------------------------------------------------------------- |
| `gf2::BitMat::transpose`  | Transposes a square matrix in place.                                       |
| `gf2::BitMat::transposed` | Returns a new matrix that is the transpose of this arbitrarily shaped one. |

The `gf2::BitMat::transposed` method works for non-square matrices by creating a new matrix with the appropriate dimensions and filling it in.

## Exponentiation

| Method Name           | Description                                              |
| --------------------- | -------------------------------------------------------- |
| `gf2::BitMat::to_the` | Returns a new matrix that is this one raised to a power. |

We efficiently compute $M^e$ by using a square and multiply algorithm, where $e = n$ or $2^n$ for some $n$.

## Matrix Inversion

We have methods to reduce a matrix to echelon form, reduced echelon form, and to compute the inverse of a square matrix:

| Method Name                            | Description                                                                        |
| -------------------------------------- | ---------------------------------------------------------------------------------- |
| `gf2::BitMat::to_echelon_form`         | Reduces a matrix to echelon form in-place.                                         |
| `gf2::BitMat::to_reduced_echelon_form` | Reduces a matrix to reduced echelon form in-place.                                 |
| `gf2::BitMat::inverse`                 | Returns the inverse of a matrix or `std::nullopt` on failure.                      |
| `gf2::BitMat::probability_invertible`  | Returns the probability of a fair random $n \times n$ matrix being invertible.     |
| `gf2::BitMat::probability_singular`    | Returns the probability of a fair random $n \times n$ matrix not being invertible. |

The inversion method can fail so we return an `std::optional` wrapped result.

## Bitwise & Arithmetic Operations

| Method Name                                     | Description                                                |
| ----------------------------------------------- | ---------------------------------------------------------- |
| `gf2::BitMat::operator^=(const BitMat<Word>&)`  | In-place `XOR` with another matrix.                        |
| `gf2::BitMat::operator&=(const BitMat<Word>&)`  | In-place `AND` with another matrix.                        |
| `gf2::BitMat::operator\|=(const BitMat<Word>&)` | In-place `OR` with another matrix.                         |
| `gf2::BitMat::operator^(const BitMat<Word>&)`   | Returns a new matrix that is the `XOR` of two others.      |
| `gf2::BitMat::operator&(const BitMat<Word>&)`   | Returns a new matrix that is the `AND` of two others.      |
| `gf2::BitMat::operator\|(const BitMat<Word>&)`  | Returns a new matrix that is the `OR` of two others.       |
| `gf2::BitMat::operator~`                        | Returns a new matrix that is the `NOT` of this one.        |
| `gf2::BitMat::operator+=(const BitMat<Word>&)`  | In-place addition with another matrix.                     |
| `gf2::BitMat::operator-=(const BitMat<Word>&)`  | In-place subtraction with another matrix.                  |
| `gf2::BitMat::operator+(const BitMat<Word>&)`   | Returns a new matrix that is the sum of two others.        |
| `gf2::BitMat::operator-(const BitMat<Word>&)`   | Returns a new matrix that is the difference of two others. |

These operations act element-by-element and panic if the matrices are not the same size.

In GF(2) addition and subtraction are both equivalent to `XOR`.

| Method Name      | Description                                                                          |
| ---------------- | ------------------------------------------------------------------------------------ |
| `gf2::dot`       | Overloaded to handle vector-matrix, matrix-vector, and matrix-matrix multiplication. |
| `gf2::operator*` | Another way to call `gf2::dot`.                                                      |

## Linear System Solvers

| Method Name                                    | Description                                                                |
| ---------------------------------------------- | -------------------------------------------------------------------------- |
| `gf2::BitMat::LU`                              | Returns a `gf2::BitLU` object for the matrix.                              |
| `gf2::BitMat::solver_for(const BitVec<Word>&)` | Returns a `gf2::BitGauss` object for the matrix and given right hand side. |
| `gf2::BitMat::x_for(const BitVec<Word>&)`      | Tries to fund a solution to the system of linear equations.                |

## Characteristic Polynomials

| Method Name                              | Description                                               |
| ---------------------------------------- | --------------------------------------------------------- |
| `gf2::BitMat::characteristic_polynomial` | Returns the characteristic polynomial of a square matrix. |
| `gf2::BitMat::frobenius_form`            | Returns the Frobenius normal form of a square matrix.     |

The characteristic polynomial is computed using Danilevsky's method, which is not well known but is efficient for bit-matrices.
It words by reducing the matrix to Frobenius normal form using a series of similarity transformations implemented using row and column operations.
See the [Danilevsky's Algorithm](Danilevsky) page for more details.

## Stringification

The following methods return a string representation for a bit-matrix.
The string can be in the obvious binary format or a more compact hex format.

| Method                                   | Description                                                               |
| ---------------------------------------- | ------------------------------------------------------------------------- |
| `gf2::BitMat::to_binary_string`          | Returns a configurable binary string representation for a bit-matrix.     |
| `gf2::BitMat::to_compact_binary_string`  | Returns a one-line minimal binary string representation for a bit-matrix. |
| `gf2::BitMat::to_string`                 | Returns a default (binary) string representation for a bit-matrix.        |
| `gf2::BitMat::to_pretty_string`          | Returns a "pretty" string representation for a bit-matrix.                |
| `gf2::BitMat::to_hex_string`             | Returns a hex string representation for a bit-matrix.                     |
| `gf2::BitMat::to_compact_hex_string`     | Returns a one-line minimal hex string representation for a bit-matrix.    |
| `gf2::BitMat::operator<<(std::ostream&)` | The usual output stream output stream operator for bit-matrices.          |
| `struct std::formatter<gf2::BitMat>`     | Specialisation of [`std::formatter`] for bit-matrices.                    |

See the [stringification](BitStore.md#stringification) section in the `gf2::BitStore` documentation for details on the various options available for formatting the rows of the matrix.

### Example

```cpp
#include <gf2/namespace.h>
int
main() {
    auto M = BitMat<>::random(5, 15);
    std::println("M.to_string():\n{}\n", M.to_string());
    std::println("M.to_binary_string():\n{}\n", M.to_binary_string());
    std::println("M.to_compact_binary_string():\n{}\n", M.to_compact_binary_string());
    std::println("M.to_hex_string():\n{}\n", M.to_hex_string());
    std::println("M.to_compact_hex_string():\n{}\n", M.to_compact_hex_string());
    std::println("M.to_pretty_string():\n{}", M.to_pretty_string());
}
```

That might produce output like this:

```txt
M.to_string():
101100101010001
100110111100010
000000010100010
001111011110001
001110010010110

M.to_binary_string():
101100101010001
100110111100010
000000010100010
001111011110001
001110010010110

M.to_compact_binary_string():
101100101010001 100110111100010 000000010100010 001111011110001 001110010010110

M.to_hex_string():
B2A1.8
9BC2.8
0142.8
3DE1.8
3926.8

M.to_compact_hex_string():
B2A1.8 9BC2.8 0142.8 3DE1.8 3926.8

M.to_pretty_string():
│1 0 1 1 0 0 1 0 1 0 1 0 0 0 1│
│1 0 0 1 1 0 1 1 1 1 0 0 0 1 0│
│0 0 0 0 0 0 0 1 0 1 0 0 0 1 0│
│0 0 1 1 1 1 0 1 1 1 1 0 0 0 1│
│0 0 1 1 1 0 0 1 0 0 1 0 1 1 0│
```

## Utility String Functions

We have a set of utility functions to get string representations for matrices & vectors side-by-side:

| Method                                                            | Description                                           |
| ----------------------------------------------------------------- | ----------------------------------------------------- |
| `gf2::string_for(const BitMat&, BitStore&)`                       | A string for a matrix and vector side-by-side.        |
| `gf2::string_for(const BitMat&, BitStore&, BitStore&)`            | A string for a matrix and two vectors side-by-side.   |
| `gf2::string_for(const BitMat&, BitStore&, BitStore&, BitStore&)` | A string for a matrix and three vectors side-by-side. |
| `gf2::string_for(const BitMat&, BitMat&)`                         | A string for two matrices side-by-side.               |
| `gf2::string_for(const BitMat&, BitMat&, BitMat&)`                | A string for two matrices side-by-side.               |

Here is an example that shows a matrix alongside its lower and upper triangular parts:

```cpp
#include <gf2/namespace.h>
int
main() {
    auto M = BitMat<>::ones(7, 7);
    std::println("    M      L       U");
    std::println("{}", string_for(M, M.lower(), M.strictly_upper()));
}
```

Output:

```txt
    M      L       U
1111111 1000000 0111111
1111111 1100000 0011111
1111111 1110000 0001111
1111111 1111000 0000111
1111111 1111100 0000011
1111111 1111110 0000001
1111111 1111111 0000000
```

## See Also

- `gf2::BitMat` for detailed documentation of all class methods.
- [`BitStore`](BitStore.md) for the concept API shared by all bit-stores.
- [`BitArray`](BitArray.md) for fixed-size vectors of bits.
- [`BitVec`](BitVec.md) for dynamically-sized vectors of bits.
- [`BitSpan`](BitSpan.md) for non-owning views into any bit-store.
- [`BitPoly`](BitPoly.md) for polynomials over GF(2).
- [Danilevsky's method](Danilevsky) for computing characteristic polynomials.

<!-- Reference Links -->

[GF2]: https://en.wikipedia.org/wiki/GF(2)
[Galois-Field]: https://en.wikipedia.org/wiki/Galois_field
[`std::vector`]: https://en.cppreference.com/w/cpp/container/vector
[doctests]: https://nessan.github.io/doxytest/
