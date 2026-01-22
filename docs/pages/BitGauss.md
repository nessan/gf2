# The `BitGauss` Solver

## Introduction

A `gf2::BitGauss` object is a [Gaussian elimination] solver for systems of linear equations over [GF(2)]:

$$
    A \cdot x = b
$$

where $A$ is an $n \times n$ _square_ bit-matrix, $b$ is a known bit-vector of length $n$, and $x$ is the unknown bit-vector to be solved for.

On construction, the `gf2::BitGauss` object captures copies of $A$ and $b$.
Then, it uses [elementary row operations] to transform the left-hand side matrix to [reduced row echelon form] while simultaneously performing identical operations to the right-hand side vector.
With those in place, the solver can quickly produce solutions $x$ by simple back-substitution.

As well as getting solutions for the system $A \cdot x = b$, the `gf2::BitGauss` object can be queried for other helpful information, such as the [rank] of $A$, whether the system is consistent (i.e., whether any solutions exist), and so on.
See the complete list below.

Recognizing that often one wants to find a solution to $A \cdot x = b$ with a minimum of palaver, the `gf2::BitMatrix` has a method to do just that.
It can be invoked as follows:

```cpp
auto x = A.x_for(b);    // Try to solve A·x = b
if(x) ...               // Solution found
```

The `x` here is a `gf2::BitVector` wrapped in a [`std::optional`].
If no solution exists, `x` will be a [`std::nullopt`].

## Multiple Solutions

A system of linear equations over $\mathbb{R}$ has either no solutions, one solution, or, if the system is under-determined, infinite solutions.

If there are $m$ independent and consistent equations for $n$ unknowns and $n > m$, there are $k = n - m$ _free_ variables.

Reducing the matrix to echelon form lets you determine how many independent equations exist and check whether the system is _consistent_.
Over $\mathbb{R}$, a free variable can take on any value; hence, consistent systems with free variables have an infinite number of solutions.

The situation is different for $\mathbb{F}_2$, because a free variable can only take on one of the values 0 and 1.
If the system has $k$ free variables, it will have $2^k$ possible solutions.
If no free variables exist, a consistent system over $\mathbb{F}_2$ will always have one unique solution.

With $k$ free variables, the $x$ in the above example will be one of those $2^k$ possible solutions randomly picked.

We also provide a way to iterate through the solutions --- though not necessarily all of them because if $k$ is large, the number of potential solutions will explode.
If `solver` is the `gf2::BitGauss` object for the consistent system $A \cdot x = b$ with $k$ free variables, then the call `solver()` will return one of the possible $2^k$ solutions picked entirely randomly. Calling `solver()` again may return a different but equally valid solution.

On the other hand, a call to `solver(n)`, where $n < 2^k$, will produce a specific solution.
There are many ways to produce an ordering amongst the possible solutions, but in any case, calling the `solver(n)` will always return the same solution.

## Constructors

If `A` is a square `gf2::BitMatrix` and `b` is a compatibly-sized `gf2::BitVector` then you can create a `gf2::BitGauss` object either by

- A direct call to the class constructor `gf2::BitGauss(A, b)`, or
- Using the factory method `gf2::BitMatrix::solver_for` with a call like `auto solver = A.solver_for(b)` on the matrix object `A`.

The constructor checks squareness and size compatibility and then performs Gauss Jordan elimination.
Internally, it stores the reduced matrix, the reduced RHS, a pivot mask, and the list of free-variable indices.

> [!NOTE]
> If $A$ is $n \times n$, then construction is an $\mathcal{O}(n^3)$ operation (though due to the nature of $\mathbf{F}_2$, things are done in whole words at a time).
> There are potentially sub-cubic ways of doing this work using various block-iterative methods that have not yet been implemented.

## Queries

Most of the work is done at construction time.
After that, the following query methods are available:

| Method                              | Description                                                         |
| ----------------------------------- | ------------------------------------------------------------------- |
| `gf2::BitGauss::rank`               | Returns the [rank] of $A$.                                          |
| `gf2::BitGauss::free_count`         | Returns the number of free variables in the system $A \cdot x = b$. |
| `gf2::BitGauss::is_underdetermined` | Returns `true` if there are any free variables.                     |
| `gf2::BitGauss::is_consistent`      | Returns `true` if there is at least one solution for the system.    |
| `gf2::BitGauss::solution_count`     | Returns the maximum number of solutions we can index into.          |

The `solution_count` may be $0$, $1$ or $2^k$ for some $k$ where $k$ is the number of free variables in an underdetermined system.
On a modern 64-but system, `solution_count = min(2^k, 2^63)`.

## Solution Access

| Method                                                  | Description                                              |
| ------------------------------------------------------- | -------------------------------------------------------- |
| `gf2::BitGauss::operator()() const`                     | Returns a solution to the system $A \cdot x = b$.        |
| `gf2::BitMatrix::x_for(const BitVector<Word>& b) const` | Returns a solution to the system $A \cdot x = b$.        |
| `gf2::BitGauss::operator()(usize) const`                | Returns the i'th solution to the system $A \cdot x = b$. |

> [!NOTE]
> These methods all return a [`std::optional`] wrapping a `gf2::BitVector` which is a solution to the system $A \cdot x = b$, or [`std::nullopt`] if no solution exists.

If there are $k$ free variables, the call to `gf2::BitGauss::operator()()` will return one of the $2^k$ possible solutions picked randomly.
The call to `gf2::BitGauss::operator()(i)` will return the i'th solution in some ordering of the $2^k$ possible solutions.

The ordering is not specified, but it is guaranteed that calling `gf2::BitGauss::operator()(i)` multiple times with the same `i` will always return the same solution.

The `gf2::BitMatrix::x_for(const BitVector<Word>& b) const` method is a convenience method that creates a `gf2::BitGauss` object internally and returns a solution to the system $A \cdot x = b$.
It is handy for quick one-off solves.

## A Simple Example

```cpp
#include <gf2/namespace.h>
int
main() {
    usize m = 12;

    auto A = BitMatrix<>::random(m, m); // Create a random m x m bit-matrix
    auto b = BitVector<>::random(m);    // Create a random right-hand side vector
    auto x = A.x_for(b);             // Try to solve A·x = b

    // Successful solution?
    if (x) {
        // Check that x is indeed a solution by computing A.x and comparing that to b
        auto Ax = dot(A, *x);
        std::println("Matrix A, solution vector x, product A.x, and right hand side b:");
        std::println("      A         x      A.x      b");
        std::println("{}", string_for(A, *x, Ax, b));
        std::println("Check A.x == b? {}", (Ax == b ? "PASS" : "FAIL"));
    } else {
        std::println("System A.x = b has NO solutions for A and b as follows:");
        std::println("      A         b");
        std::println("{}", string_for(A, b));
    }
    return 0;
}
```

- This program creates a random 12 x 12 bit-matrix `A` and a random right-hand side vector `b`.
- It then tries to solve the system $A \cdot x = b$ for $x$.
- If a solution is found, it verifies that $A \cdot x = b$.
- If no solution exists, it prints out the matrix and right-hand side for inspection.

The output varies from run to run because of the random generation of `A` and `b`.

**Sample output from a consistent system:**

```txt
Matrix A, solution vector x, product A.x, and right hand side b:
      A         x      A.x      b
111000011111    1       0       0
100111111011    0       0       0
101101111000    0       1       1
001010011011    0       1       1
000100100010    0       0       0
100111101100    0       1       1
101010011010    0       1       1
010011000010    0       0       0
010101100101    0       1       1
110110010100    0       1       1
101010100111    0       0       0
000001100110    1       0       0
Check A.x == b? PASS
```

**Sample output from an inconsistent system:**

```txt
System A.x = b has NO solutions for A and b as follows:
      A         b
100111001010    0
100110111111    1
111100010010    0
000010000101    0
011101100110    1
111101100010    1
001000000001    1
110111101100    1
101010111100    0
110110101001    0
110001111110    1
101111011000    0
```

## See Also

- `gf2::BitGauss` for detailed documentation of all class methods.
- [`BitArray`](BitArray.md) for creating and manipulating fixed-size bit-vectors.
- [`BitVector`](BitVector.md) for creating and manipulating dynamically-sized bit-vectors.
- [`BitMatrix`](BitMatrix.md) for creating and manipulating bit-matrices.
- [`BitLU`](BitLU.md) for LU factorisation-based solving of systems of linear equations over GF(2).

> [!TIP]
> For repeated solves with the same matrix, the [`BitLU`](BitLU.md) factorisation is typically faster.

<!-- Reference Links -->

[GF2]: https://en.wikipedia.org/wiki/GF(2)
[Gaussian elimination]: https://en.wikipedia.org/wiki/Gaussian_elimination
[`std::optional`]: https://en.cppreference.com/w/cpp/utility/optional
[`std::nullopt`]: https://en.cppreference.com/w/cpp/utility/optional/nullopt
[elementary row operations]: https://en.wikipedia.org/wiki/Elementary_matrix
[reduced row echelon form]: https://en.wikipedia.org/wiki/Row_echelon_form
[rank]: https://en.wikipedia.org/wiki/Rank_(linear_algebra)
