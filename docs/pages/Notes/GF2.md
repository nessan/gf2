# Working in GF(2) {#GF2}

## Introduction

`gf2` is a header-only C++ library that, amongst other things, provides classes for bit-vectors and bit-matrices.

In the jargon of professional mathematics, the classes make it possible to perform linear algebra over [GF2], the simplest [Galois field] with just two elements 0 & 1.
In [GF2], also commonly known as $\mathbb{F}_2$, addition/subtraction and multiplication/division operations are all done mod two, which keeps everything closed in the set {0,1}.

This document contains some technical notes on the joys and travails of mathematics using vectors and matrices where the elements are all just zeros and ones and where all arithmetic is mod 2.

## Differences from Real Arithmetic

Over $\mathbb{R}$, the _only_ self-orthogonal vector is the zero vector.

If $\mathbf{x}$ is a vector over $\mathbb{R}$ then

$$
    \mathbf{x} \cdot \mathbf{x} = 0 \iff \mathbf{x} = \mathbf{0},
$$

Put another way, the only vector of size 0 over $\mathbb{R}$ is the zero vector.

That is _not_ true for vectors over $\mathbb{F}_2$.

For example, if $\mathbf{v} = \{1, 1\}$ is thought of as a vector over $\mathbb{F}_2$ then $\mathbf{v} \cdot \mathbf{v} = \{1, 1\} \cdot \{1, 1\} = 1 + 1 = 2 \rightarrow 0 \text{ mod 2}.$ <br>
So $\mathbf{v}$ is non-zero but self-orthogonal.

Let $\mathbf{v}$ be a general $n$-dimensional vector over $\mathbb{F}_2$ then

$$
    \mathbf{v} \cdot \mathbf{v} = v_1 v_1 + v_2 v_2 + \cdots v_n v_n.
$$

Now

$$
    v_i v_i =
    \begin{cases}
        1 & \text{if} & v_i = 1, \\\
        0 & \text{if} & v_i = 0
    \end{cases}
$$

It follows that

$$
    \mathbf{v} \cdot \mathbf{v} = v_1 + v_2 + \cdots v_n,
$$

where of course those additions in $\mathbb{F}_2$ are done modulo 2.
Hence

$$
\mathbf{v} \cdot \mathbf{v} =
    \begin{cases}
        0 & \text{if the number of ones in the vector is even}, \\\
        1 & \text{if the number of ones in the vector is odd}.
    \end{cases}
$$

Half of all vectors over $\mathbb{F}_2$ will be self-orthogonal!

> [!NOTE]
> This property has important implications for linear algebra over $\mathbb{F}_2$.
> Some of the best-known algorithms over $\mathbb{R}$ rely on [Gram-Schmidt].
> A critical step in Gram-Schmidt is to _normalize_ a vector by simply _dividing_ each element by the norm $\lVert \mathbf{x}  \rVert = \sqrt{\mathbf{x} \cdot \mathbf{x}}$.
> However, this will not work in $\mathbb{F}_2$ as the norm will be zero 50% of the time.
> All those algorithms must be modified to work for vectors and matrices over $\mathbb{F}_2$.

## Simplifications in $\mathbb{F}_2$

Recall that if $A x = b$ represents a system of linear equations over $\mathbb{R}$, you can accomplish quite a lot using three _elementary row operations_.

---

**Elementary Row Operations for** $\mathbb{R}$

_swap two rows_: Swap the positions of any two rows.

_scale a row_: Multiply a row by a non-zero number.

_add or subtract two rows_: Add one row to another row.

---

However, in $\mathbb{F}_2$, the only non-zero scalar is one, and addition is the same as subtraction, so for matrices over $\mathbb{F}_2$, there are just _two_ elementary row operations:

---

**Elementary Row Operations for** $\mathbb{F}_2$

_swap two rows_: Swap the positions of any two rows.

_add two rows_: Add one row to another row.

---

## Gaussian Elimination in $\mathbb{F}_2$

Suppose that $A$ is an $n \times n$ matrix over $\mathbb{F}_2$ and $b$ is a compatibly sized bit-vector where we are interested in finding an $x$ satisfying $A \cdot x = b$.
Then the pseudocode for [Gaussian elimination] looks like:

<div class="pseudocode">
1. _Input_: An $n \times n$ matrix $A$ and a vector $b$ of size $n$ over $\mathbb{F}_2$.
2. **for** $j$ from $0$ to $n-1$ **do**
   1. Set $s = j$.
   2. **while** $A(s,j) == 0$ **do**
      1. Set $s = s + 1$.
   3. **if** $s > n$ **then**
      1. **continue** to the next iteration of the outer loop.
   4. **if** $s \ne j$ **then**
      1. Swap rows $s$ and $j$ in the matrix $A$.
      2. Swap elements $s$ and $j$ in the vector $b$.
   5. **for** $i$ from $j+1$ to $n$ **do**
      1. **if** $A(i,j) == 1$ **then**
         1. Replace row $i$ in $A$ with the sum of rows $i$ and $j$.
         2. Replace element $i$ in $b$ with the sum of elements $i$ and $j$.
3. _Output_: The modified matrix $A$ in row echelon form and the modified vector $b$.
</div>

<!-- Reference Links -->

[GF2]: https://en.wikipedia.org/wiki/GF(2)
[Galois Field]: https://en.wikipedia.org/wiki/Galois_field
[Gaussian elimination]: https://en.wikipedia.org/wiki/Gaussian_elimination
[Gram-Schmidt]: https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process
