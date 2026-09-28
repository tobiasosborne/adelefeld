"""Modulus contexts (docs/conventions.md 5.14; docs/proofs/policies.md section 4).

A context is `(K, q_1, ..., q_k)` with `K >= 1`, `k >= 0`, every block
`2 <= q_i < 2^64`, the blocks pairwise coprime, and their product equal to `K`
when `k >= 1`.  There is no condition between `K` and the empty block list:
`k = 0` is allowed for any `K >= 1` (the contents invariant of conventions 5.14
says "product K when k >= 1").  The family constructors happen to produce `K = 1`
or `K >= 2^64` when they produce no block, but the dump predicate does not.

This reference is written from the proof statements and the constructor table of
conventions 5.14, for clarity, not speed.  It is used to generate the vectors in
`tests/ref/vectors/m1-modctx/`; the C tests do not import it.

Residues and recombination use the CRT of Definition 16 and Lemma 17 of
`docs/proofs/policies.md`: an integer `a` maps to `a mod q_i`, and a residue list
maps back to the unique `A` in `[0, K)` with `A = res_i mod q_i`.
"""
from math import gcd

WORD = 1 << 64


class ModctxError(Exception):
    """A constructor status.  `status` is the name without the ADF_ prefix."""

    def __init__(self, status):
        super().__init__(status)
        self.status = status


def _domain():
    raise ModctxError("DOMAIN")


def _unsupported():
    raise ModctxError("UNSUPPORTED")


def _limit():
    raise ModctxError("LIMIT")


def _parse():
    raise ModctxError("PARSE")


# ----------------------------------------------------------------------------- primality


def is_prime(n):
    """Deterministic for n < 2^64 (Miller-Rabin with the seven standard bases)."""
    if n < 2:
        return False
    for p in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        if n % p == 0:
            return n == p
    d = n - 1
    r = 0
    while d % 2 == 0:
        d //= 2
        r += 1
    for a in (2, 325, 9375, 28178, 450775, 9780504, 1795265022):
        a %= n
        if a == 0:
            continue
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


def primes_upto(n):
    """The primes p <= n, increasing, by trial division (n is small in the tests)."""
    out = []
    p = 2
    while p <= n:
        if is_prime(p):
            out.append(p)
        p += 1
    return out


def _power_fits(p, e):
    """p^e < 2^64, as a Python integer; None if it does not fit."""
    if e >= 64:
        return None                       # p >= 2, so p^e >= 2^64
    r = 1
    for _ in range(e):
        r *= p
        if r >= WORD:
            return None
    return r


def _check_ctx(K, blocks):
    """The predicate of conventions 5.14; raises DOMAIN."""
    if K < 1:
        _domain()
    if blocks:
        prod = 1
        for i, a in enumerate(blocks):
            if not 2 <= a < WORD:
                _domain()
            for b in blocks[:i]:
                if gcd(a, b) != 1:
                    _domain()
            prod *= a
        if prod != K:
            _domain()
    return K, tuple(blocks)


# ----------------------------------------------------------------------------- constructors


def new_blocks(q, k=None):
    """adf_modctx_new_blocks.  `k = None` means len(q).  Statuses OK, DOMAIN."""
    if k is None:
        k = len(q)
    if k < 0:
        _domain()
    blocks = list(q[:k])
    if k == 0:
        return _check_ctx(1, [])
    prod = 1
    for i, a in enumerate(blocks):
        if a < 2:
            _domain()
        for b in blocks[:i]:
            if gcd(a, b) != 1:
                _domain()
        prod *= a
    return _check_ctx(prod, blocks)


def new_prime_powers(p, e, k=None):
    """adf_modctx_new_prime_powers.  Statuses OK, DOMAIN, UNSUPPORTED."""
    if k is None:
        k = len(p)
    if k < 0:
        _domain()
    if k == 0:
        return _check_ctx(1, [])
    blocks = []
    seen = set()
    for i in range(k):
        if e[i] == 0:
            _domain()
        if not is_prime(p[i]):
            _domain()
        if p[i] in seen:
            _domain()
        seen.add(p[i])
    for i in range(k):
        v = _power_fits(p[i], e[i])
        if v is None or v >= WORD:
            _unsupported()
        blocks.append(v)
    prod = 1
    for b in blocks:
        prod *= b
    return _check_ctx(prod, blocks)


def new_fmpz(K):
    """adf_modctx_new_fmpz.  Statuses OK, DOMAIN."""
    if K < 1:
        _domain()
    if 2 <= K < WORD:
        return _check_ctx(K, [K])
    return _check_ctx(K, [])


def new_factorial(n):
    """adf_modctx_new_factorial: K = n!, blocks the prime powers of n! in increasing
    order of the prime (conventions 5.14, CV-21).  Statuses OK, UNSUPPORTED."""
    if n < 2:
        return _check_ctx(1, [])
    blocks = []
    for p in primes_upto(n):
        v = 0
        pk = p
        while pk <= n:
            v += n // pk
            pk *= p
        b = _power_fits(p, v)
        if b is None or b >= WORD:
            _unsupported()
        blocks.append(b)
    prod = 1
    for b in blocks:
        prod *= b
    return _check_ctx(prod, blocks)


def new_primorial_pow(n, e):
    """adf_modctx_new_primorial_pow: K = (product of primes p <= n)^e, blocks p^e.
    Statuses OK, UNSUPPORTED."""
    if n < 2 or e == 0:
        return _check_ctx(1, [])
    if e >= 64:
        _unsupported()                    # 2^e >= 2^64
    blocks = []
    for p in primes_upto(n):
        b = _power_fits(p, e)
        if b is None or b >= WORD:
            _unsupported()
        blocks.append(b)
    prod = 1
    for b in blocks:
        prod *= b
    return _check_ctx(prod, blocks)


def reduce(K, blocks, a):
    """The residues of the integer a modulo the blocks (conventions 5.14; policies L17)."""
    return [a % q for q in blocks]


def recombine(K, blocks, res):
    """The unique A in [0, K) with A = res_i mod q_i (policies Lemma 17.1)."""
    A, M = 0, 1
    for r, q in zip(res, blocks):
        t = ((r - A) * pow(M % q, -1, q)) % q
        A += M * t
        M *= q
    return A % M if blocks else 0


# ----------------------------------------------------------------------------- dump body


def _is_h(s):
    return s == "0" or (s[:1] == "-" and len(s) > 1 and s[1] in "123456789abcdef"
                        and all(c in "0123456789abcdef" for c in s[2:])) \
        or (s[:1] in "123456789abcdef" and all(c in "0123456789abcdef" for c in s[1:]))


def dump_str(K, blocks):
    """The dump body text `adf1 Q modctx K k q_1 ... q_k` (conventions 10.1)."""
    parts = ["adf1", "Q", "modctx", format(K, "x"), format(len(blocks), "x")]
    parts += [format(q, "x") for q in blocks]
    return " ".join(parts)


def new_from_dump(text, occurrence=0, max_len=1048576, max_items=1048576):
    """A small model of adf_modctx_new_from_dump for a `modctx` dump.  Statuses OK, PARSE,
    LIMIT, UNSUPPORTED, DOMAIN, in the order of conventions 8.5.  The C code implements
    the same body by hand; this model exists to generate vectors."""
    if not isinstance(text, bytes):
        try:
            data = text.encode("ascii")
        except UnicodeEncodeError:
            _parse()
    else:
        data = text
    if len(data) > max_len:
        _limit()
    for c in data:
        if not 0x20 <= c <= 0x7E:
            _parse()
    s = data.decode("ascii")
    if not s.startswith("adf"):
        _parse()
    i = 3
    j = i
    while j < len(s) and s[j].isdigit():
        j += 1
    ver = s[i:j]
    if not ver:
        _parse()
    if ver != "0" and ver.startswith("0"):
        _parse()
    if ver != "1":
        _unsupported()
    if j >= len(s) or s[j] != " ":
        _parse()
    i = j + 1
    if not s[i:i + 1].isupper():
        _parse()
    j = i
    while j < len(s) and s[j] != " ":
        j += 1
    if s[i:j] != "Q":
        _unsupported()
    i = j + 1
    toks = s[i:].split(" ")
    if any(t == "" for t in toks):
        _parse()
    if not toks or toks[0] != "modctx":
        _parse()
    toks = toks[1:]
    if len(toks) < 2:
        _parse()
    if not _is_h(toks[0]) or not _is_h(toks[1]):
        _parse()
    K = int(toks[0], 16)
    k = int(toks[1], 16)
    if k < 0 or k > len(toks) - 2:
        _parse()
    blocks = []
    if k != len(toks) - 2:
        _parse()                         # trailing or missing block tokens
    for t in toks[2:2 + k]:
        if not _is_h(t):
            _parse()
        blocks.append(int(t, 16))
    if k > max_items:
        _limit()
    _check_ctx(K, blocks)
    if not 0 <= occurrence < 1:
        _domain()
    return K, tuple(blocks)