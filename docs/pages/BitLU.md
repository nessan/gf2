# The `BitLU` Decomposition

## Introduction

A `gf2::BitLU` object computes and stores the [LU decomposition] of a square bit-matrix over [GF2]:

$$
    P \cdot A = L \cdot U
$$

where $A$ is an $n \times n$ _square_ bit-matrix, $P$ is the permutation matrix, $U$ is an upper-triangular bit-matrix, and $L$ is a unit-lower-triangular bit-matrix.

In practice, $L$ and $U$ are packed compactly into an $n \times n$ bit-matrix, and the permutation "matrix" is stored as a vector.

## Construction

If `A` is a square `gf2::BitMat` you can create a `gf2::BitLU` object either by

- A direct call to the class constructor `gf2::BitLU(A)`, or
- Using the factory method `gf2::BitMat::LU` on the matrix object by a call like `auto LU = A.LU();` on the matrix object `A`.

The decomposition always works even if $A$ is singular, but other class methods will not.

If $A$ is $n \times n$, then construction is an $\mathcal{O}(n^3)$ operation (though due to the nature of $\mathbf{F}_2$, things are done in words at a time).
There are sub-cubic ways of doing this work using various block-iterative methods, but those methods have not been implemented here yet.

> [!NOTE]
> There are generalizations of the [LU decomposition] that handle rectangular matrices, but we have not implemented those yet.

## Queries

Most of the work is done at construction time.
After that, the following query methods are available:

| Method                      | Description                                                              |
| --------------------------- | ------------------------------------------------------------------------ |
| `gf2::BitLU::rank()`        | Returns the [rank] of the matrix $A$.                                    |
| `gf2::BitLU::is_singular()` | Return `true` if the matrix $A$ is not invertible.                       |
| `gf2::BitLU::determinant()` | Returns the scalar boolean that is the determinant of $A$.               |
| `gf2::BitLU::L()`           | Returns the unit lower triangular matrix `L` in the LU decomposition.    |
| `gf2::BitLU::U()`           | Returns the upper triangular matrix `U` in the LU decomposition.         |
| `gf2::BitLU::LU()`          | Read-only access to the packed `LU` form of the bit-matrix where `[L\U]` |

## Permutations

You can access permutation information using the following methods:

| Method                               | Description                                                                                        |
| ------------------------------------ | -------------------------------------------------------------------------------------------------- |
| `gf2::BitLU::swaps()`                | Returns a reference to the row swap instructions in [`LAPACK`] form.                               |
| `gf2::BitLU::permutation_vector()`   | Returns the permutation matrix as a vector of showing the index positions of the non-zero entries. |
| `gf2::BitLU::permute(BitMat<Word>&)` | Permutes the rows of the input matrix in-place using our row-swap instruction vector.              |
| `gf2::BitLU::permute(BitVec<Word>&)` | Permute the rows of the input vector in-place using our row-swap instruction vector.               |

A permutation matrix is just some row permutation of the identity matrix, so it has a single non-zero, 1, entry in each row or column.
You don't need to store the entire matrix but instead store the locations of those 1's.

In the literature, the permutation vector is often given as a permutation of the index vector.
For example, the permutation vector `[0,2,1,4,3]` tells you that elements/rows 1 and 2 are swapped, as are elements/rows 3 and 4.
This form is easy to interpret at a glance. However, it is tedious to use as a guide to actually executing the permutations in place.

The [`LAPACK`] style `swaps` vector is an alternate, equally compact, form of the permutation matrix.
Our previous example becomes `[0,2,2,4,4]`.
This is interpreted as follows:

- No swap for row 0.
- Swap row 1 with row 2.
- No swap for row 2.
- Swap row 3 with row 4.
- No swap for row 4.

## Linear System Solving

| Method                                  | Description                                                        |
| --------------------------------------- | ------------------------------------------------------------------ |
| `gf2::BitLU::operator()(const BitVec&)` | Returns a solution to the equation $A \cdot x = b$.                |
| `gf2::BitLU::operator()(const BitMat&)` | Returns a solution to the collection of equations $A \cdot X = B$. |

In the second case, each column of the bit-matrix `B` is considered a separate right-hand side, and the corresponding column of $X$ is the solution vector.

Once you have the [LU decomposition] of $A$, it is easy to solve systems like these.
If $A$ is $n \times n$ each system solution takes just $\mathcal{O}(n^2)$ operations.

These methods return a [`std::optional`] wrapping a `gf2::BitVec` or a `gf2::BitMat` or [`std::nullopt`] if $A$ is singular.
You can avoid that by first calling the `gf2::BitLU::is_singular` method.

> [!WARNING]
> Both methods throw an exception if the number of elements in $b$ or rows in $B$ does not match the number of rows in $A$.
> They could instead return a [`std::nullopt`], but a dimension mismatch is likely an indication of a coding error somewhere.

## Matrix Inversion

| Method                  | Description                                                                   |
| ----------------------- | ----------------------------------------------------------------------------- |
| `gf2::BitLU::inverse()` | Returns the inverse of the matrix $A$ or [`std::nullopt`] if $A$ is singular. |

### Example

Here is a simple example program that uses the `gf2::BitLU` class to compute the inverse of random bit-matrices and check that the result is correct:

```cpp
#include <gf2/namespace.h>
int
main() {
    // Number of trials
    usize trials = 100;

    // Each trial will run on a bit-matrix of this size
    usize N = 30;

    // Number of non-singular matrices
    usize singular = 0;

    // Start the trials
    for (usize n = 0; n < trials; ++n) {

        // Create a random matrix & decompose it
        auto A = gf2::BitMat<>::random(N, N);
        auto LU = A.LU();

        // Is the matrix singular?
        if (LU.is_singular()) {
            singular++;
        } else {
            // If not, check that A.Inverse[A] == I
            auto A_inv = LU.inverse().value();
            auto I = dot(A, A_inv);
            std::println("A.Inverse[A] == I? {}", I.is_identity() ? "YES" : "NO");
        }
    }

    // Stats
    auto p = BitMat<>::probability_singular(N);     // <1>
    std::println();
    std::println("Singularity stats ...");
    std::println("bit::matrix size: {} x {}", N, N);
    std::println("P[singular]: {}%", 100 * p);
    std::println("Trials:      {}", trials);
    std::println("Singular:    {} times", singular);
    std::println("Expected:    {} times", int(p * double(trials)));
    return 0;
}
```

1. See `BitMat::probability_singular` for details.

**Sample output:**

```txt
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES
A.Inverse[A] == I? YES

Singularity stats
bit::matrix size: 30 x 30
P[singular]: 71.1212%
Trials:      100
Singular:    68 times
Expected:    71 times
```

## See Also

- `gf2::BitLU` for detailed documentation of all class methods.
- [`BitMat`](BitMat.md) for creating and manipulating bit-matrices.
- [`BitGauss`](BitGauss.md) for a Gaussian elimination solver for $A \cdot x = b$.

<!-- Reference Links -->

[GF2]: https://en.wikipedia.org/wiki/GF(2)
[LU decomposition]: https://en.wikipedia.org/wiki/LU_decomposition
[rank]: https://en.wikipedia.org/wiki/Rank_(linear_algebra)
[`LAPACK`]: https://www.netlib.org/lapack/
[`std::optional`]: https://en.cppreference.com/w/cpp/utility/optional
[`std::nullopt`]: https://en.cppreference.com/w/cpp/utility/optional/nullopt
