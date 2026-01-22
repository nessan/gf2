# The `BitPolynomial` Class

## Introduction

A `gf2::BitPolynomial` is a polynomial over [GF2]

$$
p(x) = c_0 + c_1 x + c_2 x^2 + ... + c_n x^n
$$

where each coefficient $c_i$ is either 0 or 1 and arithmetic is performed modulo 2.

The polynomial coefficients are stored as a `gf2::BitVector`, and every operation is word-oriented and fast.

## Declaration

`gf2::BitPolynomial` is declared as:

```cpp
template <Unsigned Word = usize>
class BitPolynomial {
 // ...
};
```

The polynomial's coefficients are stored in a `gf2::BitVector`, and the `Word` template parameter specifies the underlying unsigned integer type used to back that vector. The default `Word` is usually the most efficient type for the target platform.
On most modern CPU's, that [`usize`] will be a 64-bit unsigned integer.

If your application calls for a vast number of low-degree polynomials, you might consider using `std::uint8_t` as the `Word` type to save memory.

> [!NOTE]
> You might notice that many of the [doctests] in the library use 8-bit underlying words. <br>
> The reason is we want to exercise the various methods across word boundaries for modest, easily readable bit-polynomials.

## Constructors

The following constructors are available for `gf2::BitPolynomial` objects:

| Method Name                                                        | Description                                                                 |
| ------------------------------------------------------------------ | --------------------------------------------------------------------------- |
| `gf2::BitPolynomial::BitPolynomial()`                              | The default constructor creates the zero bit-polynomial $p(x) \coloneqq 0$. |
| `gf2::BitPolynomial::BitPolynomial(const BitVector<Word>& coeffs)` | Constructs a bit-vector by _copying_ the passed vector of coefficients.     |
| `gf2::BitPolynomial::BitPolynomial(BitVector<Word>&& coeffs)`      | Constructs a bit-vector by _moving_ the passed vector of coefficients.      |

If `c` is a `gf2::BitVector` representing the coefficients of a polynomial, then `gf2::BitPolynomial p{c}` creates a `gf2::BitPolynomial` which _copies_ its coefficients from `c`.
On the other hand, `gf2::BitPolynomial p{std::move(c)}` creates a `gf2::BitPolynomial` which _moves_ `c` into its coefficients, and leaving `c` in an unspecified state.

### Factory Constructors

Other construction methods are available as static _factory_ methods:

| Method Name                         | Description                                                                                             |
| ----------------------------------- | ------------------------------------------------------------------------------------------------------- |
| `gf2::BitPolynomial::zero`          | Returns the constant zero polynomial $p(x) \coloneqq 0$.                                                |
| `gf2::BitPolynomial::one`           | Returns the constant polynomial $p(x) \coloneqq 1$                                                      |
| `gf2::BitPolynomial::constant`      | Returns the constant polynomial $p(x) \coloneqq 0 \text{ or } 1$ depending on the passed value.         |
| `gf2::BitPolynomial::zeros`         | Returns the polynomial $p(x) \coloneqq 0 + 0x + 0x^2 + \ldots + 0x^n$ where $n$ is the passed argument. |
| `gf2::BitPolynomial::ones`          | Returns the polynomial $p(x) \coloneqq 1 + x + x^2 + \ldots + x^d$. where $d$ is the passed argument.   |
| `gf2::BitPolynomial::x_to_the`      | Returns the polynomial $p(x) \coloneqq x^n$ where $n$ is the passed argument.                           |
| `gf2::BitPolynomial::from`          | Returns a polynomial whose coefficients are determined by repeatedly calling the passed function.       |
| `gf2::BitPolynomial::random`        | Returns a random polynomial of a particular degree.                                                     |
| `gf2::BitPolynomial::seeded_random` | Returns a random polynomial of a particular degree, determined by an RNG seeded for repeatability.      |

It is worth noting that there are multiple representations possible for the zero polynomial $p(x) \coloneqq 0$.

In the library, the _empty polynomial_ (the one with no coefficients at all) is considered to be a zero polynomial.

So also is the polynomial $0 + 0x + 0x^2 + ... + 0x^n$ for any $n \geq 0$ -- this is what is returned by `gf2::BitPolynomial::zeros(n)`.

## Queries

There following methods query a `gf2::BitPolynomial` object:

| Method Name                       | Description                                                                |
| --------------------------------- | -------------------------------------------------------------------------- |
| `gf2::BitPolynomial::degree`      | Returns the degree of the polynomial (returns 0 for any zero polynomial).  |
| `gf2::BitPolynomial::size`        | Returns the number of polynomial coefficients.                             |
| `gf2::BitPolynomial::is_zero`     | Returns `true` if this is some form of the zero polynomial.                |
| `gf2::BitPolynomial::is_non_zero` | Returns `true` if this is not some form of the zero polynomial.            |
| `gf2::BitPolynomial::is_one`      | Returns `true` if this is the polynomial $p(x) \coloneqq 1$ .              |
| `gf2::BitPolynomial::is_constant` | Returns `true` if this is the polynomial $p(x) \coloneqq 0 \text{ or } 1$. |
| `gf2::BitPolynomial::is_monic`    | Returns `true` if there are no high order zero coefficients.               |
| `gf2::BitPolynomial::is_empty`    | Returns `true` if this polynomial has no coefficients                      |

A polynomial is considered _monic_ if its leading coefficient (the coefficient of the highest-degree term) is 1.

For example, the polynomial $p(x) = x^3 + x + 1$ is monic, while $q(x) = 0x^4 + x^3 + x + 1$ is not monic because its leading coefficient is 0.
Both those polynomials have degree 3.

## Coefficient Access

There are methods to access and modify the polynomial coefficients either individually or as a whole:

| Method Name                              | Description                                                                  |
| ---------------------------------------- | ---------------------------------------------------------------------------- |
| `gf2::BitPolynomial::operator[]() const` | Returns a polynomial coefficient as a read-only boolean.                     |
| `gf2::BitPolynomial::operator[]()`       | Returns a polynomial coefficient as a read-write `gf2::BitRef`.              |
| `gf2::BitPolynomial::coefficients const` | Returns a read-only reference to the coefficient bit-vector.                 |
| `gf2::BitPolynomial::coefficients`       | Returns a read-write reference to the coefficient bit-vector.                |
| `gf2::BitPolynomial::copy_coefficients`  | Copies the elements from the passed bit-store to the coefficient bit-vector. |
| `gf2::BitPolynomial::move_coefficients`  | Moves the elements from the passed bit-store to the coefficient bit-vector.  |
| `gf2::BitPolynomial::clear`              | Sets the polynomial back to $p(x) \coloneqq 0`$.                             |
| `gf2::BitPolynomial::resize`             | Resizes the polynomial to have the `n` coefficients (added ones are zeros).  |
| `gf2::BitPolynomial::shrink_to_fit`      | Calls `gf2::BitVector::shrink_to_fit` on the coefficient bit-vector.         |
| `gf2::BitPolynomial::make_monic`         | Kills any high order zero coefficients to make the polynomial _monic_.       |

## Arithmetic Operations

We have all the usual arithmetic operations defined for `gf2::BitPolynomial` objects where the addition and subtraction operations are identical since we are working over GF(2).

Multiplication of two arbitrary bit-polynomials, $p(x) q(x)$, is performed using `gf2::convolve` which implements efficient convolutions of bit-stores.

There are a couple of "fast" methods for common arithmetic operations:

| Method Name                          | Description                                                   |
| ------------------------------------ | ------------------------------------------------------------- |
| `gf2::BitPolynomial::squared`        | Computes the polynomial $p(x)^2$.                             |
| `gf2::BitPolynomial::times_x_to_the` | Performs the in-place operation $p(x) \rightarrow x^n p(x)$ . |

The squaring operation is optimised since in GF(2), squaring a polynomial simply involves inserting zero coefficients between each existing coefficient (see `gf2::riffle`).

The `gf2::BitPolynomial::squared` can be passed a pre-allocated polynomial to store the result --- this is important for algorithms that require repeated squaring to avoid unnecessary allocations. See for example the [modular reduction] technical note for details.

Multiplication by $x^n$ is also optimised since it simply involves bit-shifting the coefficients.

Those methods are much faster than using the general multiplication operator $p(x) * q(x)$ when $q(x) = x^n$ or $p(x)$.

## Polynomial Evaluation

There are methods to evaluate a bit-polynomial for a scalar value or for any _square_ bit-matrix:

| Method Name                                                | Description                                                   |
| ---------------------------------------------------------- | ------------------------------------------------------------- |
| `gf2::BitPolynomial::operator()(bool x)`                   | Evaluates the polynomial at the passed bit value `x`.         |
| `gf2::BitPolynomial::operator()(const BitMatrix<Word>& M)` | Evaluates the polynomial at the passed square bit-matrix `M`. |

Matrix evaluation uses [Horner's method] to evaluate `p(M)` where `M` is a square matrix.
The result is returned as a new bit-matrix.

## Modular Reduction

We have a method to compute $x^N \bmod{p(x)}$ where $p(x)$ is a bit-polynomial and $N$ is a potentially huge integer:

| Method Name                           | Description                                                               |
| ------------------------------------- | ------------------------------------------------------------------------- |
| `gf2::BitPolynomial::reduce_x_to_the` | Returns the polynomial $x^N \bmod{p(x)}$ where $N$ is the passed integer. |

This method can handle _very_ large values of $N$. <br>
See the [modular reduction] technical note for more details.

## Stringification

The following methods return a string representation for a bit-polynomial.

| Method                                          | Description                                                             |
| ----------------------------------------------- | ----------------------------------------------------------------------- |
| `gf2::BitPolynomial::to_string`                 | Returns a string representation where zero coefficients are not shown.  |
| `gf2::BitPolynomial::to_full_string`            | Returns a string representation where zero coefficients are all shown.. |
| `gf2::BitPolynomial::operator<<(std::ostream&)` | The usual output stream output stream operator for bit-polynomials.     |
| `struct std::formatter<gf2::BitPolynomial>`     | Specialisation of [`std::formatter`] for bit-polynomials.               |

The `to_string` and `to_full_string` methods can be passed a string argument to specify the variable name.
The default variable name is `x`.

```cpp
auto p = BitPolynomial<>::ones(3);
assert_eq(p.to_string(), "1 + x + x^2 + x^3");
assert_eq(p.to_string("M"), "1 + M + M^2 + M^3");
```

For [`std::formatter`] you can set the variable name using a format specifier:

```cpp
auto p = BitPolynomial<>::ones(2);
std::println("{:x}", p);      // prints "1 + x + x^2"
std::println("{:M}", p);      // prints "1 + M + M^2"
std::println("{:mat}", p);   // prints "1 + mat + mat^2"
```

## See Also

- `gf2::BitPolynomial` for detailed documentation of all class methods.
- [`BitStore`](BitStore.md) for the concept API shared by all bit-stores.
- [`BitArray`](BitArray.md) for fixed-size vectors of bits.
- [`BitVector`](BitVector.md) for dynamically-sized vectors of bits.
- [`BitSpan`](BitSpan.md) for non-owning views into any bit-store.
- [`BitMatrix`](BitMatrix.md) for matrices of bits.
- [Modular Reduction](Reduction) for details on the modular reduction $x^N \bmod{p(x)}$.

<!-- Reference Links -->

[GF2]: https://en.wikipedia.org/wiki/GF(2)
[`usize`]: https://en.cppreference.com/w/cpp/types/size_t
[`std::formatter`]: https://en.cppreference.com/w/cpp/utility/format/formatter
[doctests]: https://nessan.github.io/doxytest/
[Horner's method]: https://en.wikipedia.org/wiki/Horner%27s_method
[modular reduction]: Reduction
