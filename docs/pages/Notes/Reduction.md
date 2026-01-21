# Modular Reduction {#Reduction}

## Introduction

We are interested in computing the modular reduction of $x^N$ by a polynomial $p(x)$ over [GF2](<https://en.wikipedia.org/wiki/GF(2)>), where $N$ is a potentially huge integer.

Let $P(x)$ be a nonzero polynomial of degree $n$ over $\mathbb{F}_2$.

Any other polynomial $h(x)$ over $\mathbb{F}_2$ can be decomposed as:

$$
    h(x) = q(x) P(x) + r(x),
$$

where $q(x)$ is the _quotient_ polynomial, and $r(x)$ is the _remainder_ polynomial with degree less than $n$.

We say that $r(x)$ is the _modular reduction_ of $h(x)$ by $P(x)$

$$
r(x) = h(x) \mid {P(x)}.
$$

## The Polynomial $x^N$

The simplest, single-term polynomial $h(x) = x^N$ is of particular importance, especially for cases where $N \gg 1$.

This is because some numerical algorithms have a critical iteration step that can formally be written as:

$$
\mathbf{v} \gets M \cdot \mathbf{v},
$$

where $\mathbf{v}$ is a bit-vector bucket of $n$ bits and $M$ is an $n \times n$ bit-matrix.

For example, many well-known random number generators can be cast into this form, where a _state vector_ $\mathbf{v}$ is advanced at each step before it is reduced to the following random number.
We note in passing that the generator is unlikely to be coded as matrix-vector multiply in GF(2) --- $M$ It is typically rather sparse and special so that the iteration can be carried out much more efficiently by other means.
Nevertheless, the mathematical analysis of the generator will depend on the structure of $M$.

Now you want to jump very far ahead in a random number stream.
This lets one start a parallel thread of computation using the same set of random numbers but so far ahead that there is no danger of overlaps.
To jump $N$ steps ahead where $N \gg 1$ we need to compute

$$
M^N \cdot \mathbf{v}.
$$

Even if $M$ is sparse and special; there usually is no easy way to compute $M^N$.

But suppose that $P(x)$ is the known degree $n$ characteristic polynomial for $M$ then the [Cayley Hamilton theorem](https://en.wikipedia.org/wiki/Cayley–Hamilton_theorem) tells us that:

$$
P(M) = 0.
$$

We can use that as follows --- first, express $x^N$ as

$$
    x^N = q(x)P(x) + r(x),
$$

then using Cayley Hamilton we get

$$
    M^N = q(M)P(M) + r(M) = r(M).
$$

So we can replace $M^N$ by $r(M)$ where the degree of $r$ is less than $n$ and typically $N \gg n$.

Thus, once we know $r(x) = x^N \mid P(x)$, we can jump $N$ steps ahead in the algorithm by computing the inexpensive polynomial sum $r(M)$.

For more details see [this paper](https://www.iro.umontreal.ca/~lecuyer/myftp/papers/jumpmt.pdf).

## An Iterative Technique

We will describe an iterative technique to compute $x^N \mid P(x)$ for arbitrary non-negative integers $N$.

Suppose $P(x)$ has degree $n$ so there is a polynomial $p(x)$ of degree less than $n$ such that

$$
    P(x) = p(x) + x^n = p_0 + p_1 x + \cdots + p_{n-1} x^{n-1} + x^n.
$$

Then $p(x)$ can be represented as the vector of its coefficients:

$$
    p(x) \sim \mathbf{p} = \lbrack p_0 p_1 \ldots p_{n-1} \rbrack .
$$

There are three cases to consider as we compute $x^N \mid P(x)$.

### Case $N < n$

If $N < n$ then $P(x)$ does not divide $x^N$ so

$$
    x^N \mid P(x) = x^N.
$$

Defining $\mathbf{u}_N$ as the _unit_ bit-vector of size $n$, which is all zeros except for a one in the $N^{\mathrm{th}}$ slot, we can write:

$$
    x^N \mid P(x) \sim \mathbf{u}_N \text{ if } N < n.
$$

### Case $N = n$

In this case $P(x) = p(x) + x^N$ so $x^N = P(x) - p(x)$.

Therefore

$$
    x^N \mid P(x) = -p(x).
$$

In $\mathbb{F}_2$ we can ignore that minus sign and write

$$
    x^N \mid P(x) \sim \mathbf{p}  \text{ if } N = n.
$$

### Case $N > n$

It remains to determine $x^N \mid P(x)$ for $N > n$.

Now _any_ polynomial $h(x)$ over $\mathbb{F}_2$ can be written as some multiple of $P(x)$ plus a remainder term:

$$
    h(x) = q(x) P(x) + r(x)
$$

where the _quotient_ $q(x)$ and _remainder_ $r(x)$ are polynomials over $\mathbb{F}_2$ and the degree of $r(x)$ is strictly less than $n$.

$$
    r(x) \equiv h(x) \mid P(x)
$$

Suppose we already know the explicit form for $r(x) = h(x) \mid P(x)$

$$
    r(x) = r_0 + r_1 x + \cdots + r_{n-2} x^{n-2} + r_{n-1} x^{n-1}.
$$

That is, we know the elements in the bit-vector of coefficients for $r(x)$

$$
    r(x) \sim \mathbf{r} = \lbrack r_0 r_1 \ldots r_{n-1} \rbrack.
$$

Now

$$
    x\,h(x) = x\,q(x) P(x) + x\,r(x) \implies x\,h(x) \mid P(x) = x\,r(x) \mid P(x).
$$

Thus

$$
    x\,h(x) \mid P(x) = \left(r_0 x + r_1 x^2 + \cdots + r_{n-2} x^{n-1}\right) \mid P(x) + r_{n-1} x^n \mid P(x).
$$

Using our two known cases for $N < n$ and $N = n$ we get

$$
    x\,h(x) \mid P(x) \sim \lbrack 0 r_0 \ldots r_{n-2} \rbrack + r_{n-1} \mathbf{p}.
$$

Thus if we know that $h(x) \mid P(x) \sim \mathbf{r}$ then

$$
    x\,h(x) \mid P(x) \sim (\mathbf{r} \gg 1 ) \; \wedge \; r_{n-1} \mathbf{p}.
$$

Here $\mathbf{r} \gg 1$ means we shift $\mathbf{r}$ one place to the right and introduce a zero on the left.

### Summary

Using the notation

$$
    x^N \mid P(x) = r^N(x) \sim \mathbf{r}^N,
$$

where $\mathbf{r}^N$ is a bit-vector of size $n$:

$$
    \mathbf{r}^N = \lbrack r^N_0 r^N_1 \ldots r^N_{n-1} \rbrack,
$$

we can compute $\mathbf{r}^N$ directly for small values of $N$ and iteratively for larger values of $N$:

$$
\mathbf{r}^N =
\begin{cases}
    \mathbf{u}_N, & N < n \\\\[.5em]
    \mathbf{p},   & N = n \\\\[.5em]
    \left(\mathbf{r}^{N-1} \gg 1 \right) \; \wedge \; r_{n-1}^{N-1} \, \mathbf{p} & N > n
\end{cases}
$$

## A Multiply & Square Technique

For cases of practical interest where $N \gg 1$, the iterative scheme outlined above is much too slow.

We can speed it up considerably by using a "multiply & square" approach -- there are variations on the theme but observe that we can always write:

$$
    x^N =
    \begin{cases}
        x \, \left( x^{\frac{N-1}{2}} \right)^2, & N \text{ odd} \\\\[.5em]
        \left( x^{\frac{N}{2}} \right)^2         & N \text{ even}
    \end{cases}
$$

In our case, we want to compute $x^N \mid P(x)$ as opposed to just computing the value of $x^N$ but we can still use a fast [exponentiation technique](https://en.wikipedia.org/wiki/Modular_exponentiation).

If $g$ and $h$ are polynomials over $\mathbb{F}_2$ with the remainders on division by $P(x)$ given by $r_g$ and $r_h$ respectively:

$$
\begin{align}
  r_g &= g(x) \mid P(x), \\\
  r_h &= h(x) \mid P(x)
\end{align}
$$

then it is easily verified that

$$
    g(x) h(x) \mid P(x) = r_g(x) r_h(x) \mid P(x).
$$

So while the polynomial product $g h$ may have a very high degree, we can instead work with the much remainder product $r_g r_h$ whose degree is at most $2n -2$.

In our case, suppose we already know $r(x) = x^k \mid P(x)$ for some power $k$ i.e. we know the coefficients $\mathbf{r}$ of the polynomial $r(x)$:

$$
    r(x) \sim \mathbf{r} = [r_0 r_1 \ldots r_{n-1}].
$$

To get to $x^N \mid P(x)$ from there, the multiply and square algorithm requires two procedures:

|      Step      | Assuming the degree of $r(x) < n$ and $r(x) \sim \mathbf{r}$. |
| :------------: | :-----------------------------------------------------------: |
| `MultiplyStep` |         Computes $\mathbf{r} \gets x r(x) \mid P(x)$          |
|  `SquareStep`  |         Computes $\mathbf{r} \gets r(x)^2 \mid P(x)$          |

With those in place we can proceed as shown in the following sketch:

<div class="pseudocode">
1. _Input:_ A coefficient bit-vector $\mathbf{p}$ of size $n$ where $P(x) = x^n + p(x)$
2. _Input:_ An integer $N$ where we wish to compute $x^N \mid P(x)$.
3. Initialise a result bit-vector $\mathbf{r}$ of size $n$ to $\mathbf{0}$.
4. Set $r_1 = 1$.
5. While $N > 0$ do:
    1. **if** $N$ is odd, perform a `MultiplyStep` on $\mathbf{r}$.
    2. Perform a `SquareStep` on $\mathbf{r}$.
    3. $N \gets N/2$.
6. When the loop ends, $\mathbf{r}$ contains the coefficients of $x^N \mid P(x)$.
 </div>

The full code handles the decomposition of $P(x)$ into the $x^n + p(x)$ and manages edge cases such as $P(x) = 1$.
It also handles the trivial cases where $N \le n$.
For larger values of $N$ it uses its binary representation in the main loop.
Nevertheless, the sketch shows the importance of the two sub-procedures, `MultiplyStep` and `SquareStep` which we discuss next.

### The Multiply Step

If $q(x)$ is a polynomial of degree less than $n$ so that

$$
    q(x) \mid P(x) = q(x),
$$

then the following procedure performs the step

$$
    q(x) \gets x q(x) \mid P(x),
$$

<div class="pseudocode">
1. _Input:_ A coefficient bit-vector $\mathbf{p}$ of size $n$ where $P(x) = x^n + p(x)$.
2. _Input:_ A coefficient bit-vector $\mathbf{q}$ of size $n$ where $q(x) \sim \mathbf{q}$.
    1. $tmp \gets q_{n-1}$
    2. $\mathbf{q} \gets \mathbf{q} \gg 1$
    3. **if** $tmp$ **then** $\mathbf{q} \gets \mathbf{q} \wedge \mathbf{p}$
3. On return $\mathbf{q}$ has the coefficients for $x q(x) \mid P(x)$.
 </div>

### The Square Step

In GF(2) if a polynomial $q(x)$ is represented by the coefficient bit-vector $\mathbf{q} = [q_0, q_1, q_2, \ldots, q_{n-1}]$:

$$
    q(x) = q_0 + q_1 x + q_2 x^2 + \ldots q_{n-1} x^{n-1},
$$

one can easily show that

$$
    q(x)^2 = q_0 + q_1 x^2 + q_2 x^4 + \cdots + q_{n-1} x^{2n-2},
$$

so $s(x) = q(x)^2$ is represented by _riffling_ the bit-vector $\mathbf{q}$

$$
    s(x) = q(x)^2 \sim \mathbf{s} = [q_0, 0, q_1, 0, q_2, \ldots, 0, q_{n-1}],
$$

i.e. the bit-vector we get by interspersing the elements of $\mathbf{q}$ with zeros.

Riffling can be done very efficiently word by word.
The `gf2::BitVec::riffle_into` method takes a bit-vector $\mathbf{q}$ and fills a destination bit-vector $\mathbf{s}$ with a _riffled_ version of $\mathbf{q}$.

The `gf2::BitVec::split_at` method takes a bit-vector $\mathbf{s}$, a number $n$, and then fills two other bit-vectors $\mathbf{l}$ and $\mathbf{h}$ where $\mathbf{l}$ gets the first $n$ elements in $\mathbf{s}$ and $\mathbf{h}$ gets the rest.

$$
\begin{align}
    \mathbf{l} &= [s_0, s_1, \ldots, s_{n-1}], \\\
    \mathbf{h} &= [s_n, s_{n+1}, \dots].
\end{align}
$$

In polynomial terms this is equivalent to the decomposition:

$$
    s(x) = l(x) + x^n \, h(x),
$$

where the degree of $l(x)$ is less than $n$.

Given that $s(x) = q(x)^2$ we have

$$
    q(x)^2 \mid P(x) = s(x) \mid P(x) = l(x) \mid P(x) + x^n h(x) \mid P(x),
$$

and because the degree of $l(x)$ is less than $n$ we have

$$
    q(x)^2 \mid P(x) = l(x) + x^n h(x) \mid P(x).
$$

Writing $h(x)$ as

$$
h(x) = \sum_{i=0}^{n-1} h_i x^i
$$

it follows that

$$
    q(x)^2 \mid P(x) =  l(x) + \sum_{i=0}^{n} h_i x^{n + i} \mid P(x).
$$

Define the bit-vectors $\mathbf{x}^i$ by the equivalence:

$$
    \mathbf{x}^i \sim x^{n+i} \mid P(x) \text{ for } i = 0, \ldots, n-1.
$$

Now we know that $x^n \mid P(x) = p(x)$ so

$$
    \mathbf{x}^0 = \mathbf{p}.
$$

With that starting point, we can easily fill in bit-vectors $\mathbf{x}^i$ for $i = 1, \ldots, n-1$ by using the `MultiplyStep` algorithm.

The squaring step for a polynomial $q(x)$ looks like the following:

<div class="pseudocode">
1. _Input:_ A coefficient bit-vector $\mathbf{p}$ of size $n$ where $P(x) = x^n + p(x)$.
2. _Input:_ A coefficient bit-vector $\mathbf{q}$ of size $n$ where $q(x) \sim \mathbf{q}$.
3. _Precomputed:_ Bit-vectors $\mathbf{x}^i$, where $\mathbf{x}^i \sim x^{n+i} \mid P(x)$.
4. _Workspace:_ Bit-vector $\mathbf{s}$ of size $2n$, bit-vectors $\mathbf{l}, \mathbf{h}$ of size $n$.
    1. Riffle $\mathbf{q}$ into $\mathbf{s}$.
    2. Split $\mathbf{s}$ at $n$ into $\mathbf{l}$ and $\mathbf{h}$.
    3. $\mathbf{q} \gets \mathbf{l}$.
    4. **for** $i \gets 0, n-1$
        1. **if** $h_i$ **then** $\mathbf{q} \gets \mathbf{q} \wedge \mathbf{x}^i$
5. On return $\mathbf{q}$ has the coefficients for $q(x)^2 \mid P(x)$.
 </div>

The full code can implement some efficiencies in this algorithm's loop -- for example, at most every second element in $\mathbf{h}$ is ever set.
