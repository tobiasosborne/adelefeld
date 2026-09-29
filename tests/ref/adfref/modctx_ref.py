"""Modulus contexts: the reference for the constructors and for the two conversions.

Written from `docs/conventions.md` 5.14 (contents and constructors), `docs/proofs/policies.md`
Definition 16 and Lemma 17 (line 305 and line 312: a local value is residues of one integer
modulo pairwise coprime blocks, and recombination is CRT), `docs/SPEC.md` 4.1 (the modulus data
of the local backend) and `docs/SPEC.md` 15.1 (the modulus families). Not written from the C.

What a context is (conventions 5.14): a modulus `K >= 1` and `k >= 0` word blocks
`q_1, ..., q_k`, pairwise coprime, `2 <= q_i < 2^64`, in the order supplied, with product `K`
when `k >= 1`. Every constructor returns `(status, K, blocks)`; `status` is one of "OK",
"DOMAIN", "UNSUPPORTED" (conventions 3.2, row "Raw context constructors"), and on a failure
`K` and `blocks` are `None`. The families are:

    new_blocks(q)               the blocks as supplied; k = 0 gives K = 1
    new_prime_powers(p, e)      the blocks p[i]^e[i] in the order supplied; k = 0 gives K = 1
    new_fmpz(K)                 one block K if 2 <= K < 2^64; no block if K = 1 or K >= 2^64
    new_factorial(n)            the prime powers of n! as blocks, increasing in the prime
    new_primorial_pow(n, e)     the blocks p^e for the primes p <= n, increasing in the prime

The two conversions (policies Definition 16, Lemma 17): `reduce(a, blocks)` gives the residues
`a mod q_i` in `[0, q_i)`; `combine(residues, blocks)` gives the unique integer of `[0, K)`
with those residues (the CRT of Lemma 17.1). For no blocks, reduce gives `[]` and combine
gives 0 (K = 1).

Validation of the constructors (conventions 5.14 and the header of modctx.h):

- new_blocks: every block >= 2 and pairwise coprime, else "DOMAIN".
- new_prime_powers: exponent 0, a composite or repeated prime: "DOMAIN"; a power p^e >= 2^64:
  "UNSUPPORTED".
- new_fmpz: K < 1 is "DOMAIN".
- new_factorial: a prime power of n! >= 2^64 is "UNSUPPORTED".
- new_primorial_pow: n < 2 or e = 0 gives K = 1 without blocks; a prime power p^e >= 2^64 is
  "UNSUPPORTED".

Two choices are not written in the conventions and are recorded here so that the vectors cover
them (HEADER-FINDING of lane m1-modctx, see its report):

1. Too many blocks. A constructor never accepts more than `MAX_BLOCKS` blocks and never reads a
   block list longer than that; a longer list gives "UNSUPPORTED" before any block is read. The
   bound is `max_items` of conventions 8.4 (1048576), which bounds the block count of a context
   occurrence in text. The same bound stops `new_primorial_pow` on a huge `n` (the primes
   p <= n are then more than MAX_BLOCKS). Conventions 3.2 has no LIMIT status for a raw context
   constructor, so the request is reported as "valid request that version 1 does not implement".
2. A missing array (blocks, p, e) with a positive count is "DOMAIN" (modctx.h: "q is read only
   (it may be NULL when k = 0)").

Primality: `is_prime` is a deterministic Miller-Rabin for n < 2^64 with the bases 2, 3, 5, 7,
11, 13, 17, 19, 23, 29, 31, 37. The base set is not quoted from a source on disk
[source pending: a reference for the deterministic Miller-Rabin base set below 2^64]; the C
tests compare it against FLINT's n_is_prime (modctx.h names that function), so the two oracles
are cross-checked on every vector.
"""

MAX_BLOCKS = 1048576
WORD_BITS = 64
WORD_MAX = (1 << WORD_BITS) - 1

_SMALL_PRIMES = (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37)


def is_prime(n):
    """Deterministic Miller-Rabin for 0 <= n < 2**64 (see the module docstring)."""
    n = int(n)
    if n < 2:
        return False
    for p in _SMALL_PRIMES:
        if n % p == 0:
            return n == p
    d = n - 1
    r = 0
    while d % 2 == 0:
        d //= 2
        r += 1
    for a in _SMALL_PRIMES:
        x = pow(a, d, n)
        if x == 1 or x == n - 1:
            continue
        for _ in range(r - 1):
            x = x * x % n
            if x == n - 1:
                break
        else:
            return False
    return True


def next_prime(p):
    """The smallest prime strictly above the integer p >= 1."""
    p = int(p) + 1
    if p <= 2:
        return 2
    if p % 2 == 0:
        p += 1
    while not is_prime(p):
        p += 2
    return p


def primes_upto(n):
    """All primes <= n, in increasing order."""
    out = []
    p = 2
    while p <= n:
        out.append(p)
        p = next_prime(p)
    return out


def pow_word(p, e):
    """(ok, p**e): ok is False exactly when p**e >= 2**64. e >= 0, p >= 2."""
    if e >= WORD_BITS:
        return False, None                    # p >= 2, so p^e >= 2^64 without computing it
    r = 1
    for _ in range(int(e)):
        if r > WORD_MAX // p:
            return False, None
        r *= p
    return True, r


def valuation_factorial(n, p):
    """v_p(n!) = sum of the floors n/p^i (Legendre)."""
    e = 0
    t = n // p
    while t:
        e += t
        t //= p
    return e


def new_blocks(q):
    """The context with the blocks q in the order supplied (conventions 5.14)."""
    q = [int(x) for x in q]
    if len(q) > MAX_BLOCKS:
        return "UNSUPPORTED", None, None
    for x in q:
        if x < 2:
            return "DOMAIN", None, None
    for i in range(len(q)):
        for j in range(i):
            if gcd(q[i], q[j]) != 1:
                return "DOMAIN", None, None
    K = 1
    for x in q:
        K *= x
    return "OK", K, q


def new_prime_powers(p, e):
    """The context with the blocks p[i]^e[i] in the order supplied (conventions 5.14)."""
    p = [int(x) for x in p]
    e = [int(x) for x in e]
    if len(p) != len(e):
        raise ValueError("p and e must have the same length")
    if len(p) > MAX_BLOCKS:
        return "UNSUPPORTED", None, None
    seen = set()
    blocks = []
    for i in range(len(p)):
        if e[i] == 0:
            return "DOMAIN", None, None
        if not is_prime(p[i]):
            return "DOMAIN", None, None
        if p[i] in seen:
            return "DOMAIN", None, None
        seen.add(p[i])
        ok, block = pow_word(p[i], e[i])
        if not ok:
            return "UNSUPPORTED", None, None
        blocks.append(block)
    K = 1
    for x in blocks:
        K *= x
    return "OK", K, blocks


def new_fmpz(K):
    """The context of modulus K (modctx.h: one block if 2 <= K < 2^64, none otherwise)."""
    K = int(K)
    if K < 1:
        return "DOMAIN", None, None
    if K == 1:
        return "OK", 1, []
    if K < (1 << 64):
        return "OK", K, [K]
    return "OK", K, []


def new_factorial(n):
    """The context of modulus n!, blocks the prime powers of n! (conventions 5.14, CV-21)."""
    n = int(n)
    if n < 2:
        return "OK", 1, []
    # 2^v_2(n!) is the smallest prime power of n!; if it leaves a word, every larger one does,
    # and v_2(n!) >= 64 holds for every n >= 66, so this check also bounds the loop below.
    ok, _ = pow_word(2, valuation_factorial(n, 2))
    if not ok:
        return "UNSUPPORTED", None, None
    blocks = []
    for p in primes_upto(n):
        ok, block = pow_word(p, valuation_factorial(n, p))
        if not ok:
            return "UNSUPPORTED", None, None
        blocks.append(block)
    K = 1
    for x in blocks:
        K *= x
    return "OK", K, blocks


def new_primorial_pow(n, e):
    """The context of modulus (product of the primes p <= n)^e, blocks p^e (PLAN 4)."""
    n = int(n)
    e = int(e)
    if n < 2 or e == 0:
        return "OK", 1, []
    blocks = []
    p = 2
    while p <= n:
        if len(blocks) == MAX_BLOCKS:
            return "UNSUPPORTED", None, None
        ok, block = pow_word(p, e)
        if not ok:
            return "UNSUPPORTED", None, None
        blocks.append(block)
        p = next_prime(p)
    K = 1
    for x in blocks:
        K *= x
    return "OK", K, blocks


def reduce(a, blocks):
    """The residues of the integer a modulo each block, each in [0, q_i) (policies Definition 16)."""
    a = int(a)
    return [a % q for q in (int(x) for x in blocks)]


def combine(residues, blocks):
    """The unique integer of [0, K) with these residues (policies Lemma 17.1, CRT)."""
    residues = [int(r) for r in residues]
    blocks = [int(q) for q in blocks]
    if len(residues) != len(blocks):
        raise ValueError("one residue per block")
    if not blocks:
        return 0
    K = 1
    for q in blocks:
        K *= q
    A = 0
    for r, q in zip(residues, blocks):
        Ki = K // q
        # inverse of Ki modulo q (the blocks are pairwise coprime)
        inv = pow(Ki, -1, q)
        A += r * Ki * inv
    return A % K


def gcd(a, b):
    while b:
        a, b = b, a % b
    return abs(a)
