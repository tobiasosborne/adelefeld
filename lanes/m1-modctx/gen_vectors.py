#!/usr/bin/env python3
"""Deterministic test vectors for tests/test_modctx.c (lane m1-modctx).

Writes JSON lines to tests/ref/vectors/m1-modctx/. Every line has an "op" field and the inputs
and the expected output of one call. The vectors come from tests/ref/adfref/modctx_ref.py, which
is written from docs/conventions.md 5.14 and docs/proofs/policies.md Definition 16 and Lemma 17.

Run from the repository root:

    python3 lanes/m1-modctx/gen_vectors.py

The output is deterministic: the seed is fixed, so two runs are byte-identical.

Files and records:

    ctx_construct.jsonl    one constructor call
        {"op": "new_blocks",        "q": [..], "status": s, "K": n, "blocks": [..]}
        {"op": "new_prime_powers",  "p": [..], "e": [..], "status": s, "K": n, "blocks": [..]}
        {"op": "new_fmpz",          "input_K": n, "status": s, "K": n, "blocks": [..]}
        {"op": "new_factorial",     "n": n, "status": s, "K": n, "blocks": [..]}
        {"op": "new_primorial_pow", "n": n, "e": n, "status": s, "K": n, "blocks": [..]}
        On a failure "K" and "blocks" are absent and "status" names the failure
        ("DOMAIN", "UNSUPPORTED"), so the C test also checks that no output is written.

    ctx_roundtrip.jsonl    one conversion of an integer to residues and back
        {"op": "reduce_combine", "blocks": [..], "a": n, "residues": [..], "combined": n}
        "combined" is the integer of [0, K) with these residues (policies Lemma 17.1).
"""
import json
import os
import random
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tests", "ref"))

from adfref import modctx_ref as m  # noqa: E402

VECTORS = os.path.join(ROOT, "tests", "ref", "vectors", "m1-modctx")

SEED = 20260928

WORD64 = (1 << 64) - 1
TWO64 = 1 << 64
MERSENNE61 = (1 << 61) - 1


def record_construct(op, args, result):
    status, K, blocks = result
    rec = {"op": op}
    rec.update(args)
    rec["status"] = status
    if status == "OK":
        rec["K"] = K
        rec["blocks"] = blocks
    return rec


def construct_records():
    recs = []

    # ---- the family "list of pairwise coprime blocks" (conventions 5.14) ----
    for q in ([], [2], [2, 3, 5], [5, 2], [4, 9, 25], [6, 35, 11],
              [2 ** 63], [WORD64], [3, 5, 7, 11, 13, 17, 19, 23],
              [2 ** 32 - 5, 2 ** 32 - 17, 2 ** 32 - 23]):
        recs.append(record_construct("new_blocks", {"q": q}, m.new_blocks(q)))
    # failures: a block of 0 or 1; blocks that are not pairwise coprime
    for q in ([0], [1], [2, 0], [2, 1], [0, 3], [4, 6], [2, 9, 15], [65537, 65537],
              [9, 25, 15], [2, 4]):
        recs.append(record_construct("new_blocks", {"q": q}, m.new_blocks(q)))

    # ---- the family "list of prime powers" ----
    pp_ok = [
        ([], []),
        ([2], [1]),
        ([3, 2], [1, 1]),                 # the order supplied is kept
        ([2, 3], [3, 2]),                 # 8, 9
        ([2], [63]),                      # 2^63 fits a word
        ([3], [40]),                      # 3^40 < 2^64
        ([2, 5, 11], [10, 3, 2]),
        ([2305843009213693951], [1]),     # 2^61 - 1 is prime
    ]
    pp_bad = [
        ([2], [0], "DOMAIN"),             # exponent 0
        ([2, 3], [2, 0], "DOMAIN"),
        ([4], [2], "DOMAIN"),             # 4 is not prime
        ([2, 4], [1, 1], "DOMAIN"),
        ([0], [1], "DOMAIN"),
        ([1], [1], "DOMAIN"),
        ([2, 2], [1, 2], "DOMAIN"),       # the prime repeats
        ([3, 3], [2, 2], "DOMAIN"),
        ([2], [64], "UNSUPPORTED"),       # 2^64 is not below 2^64
        ([3], [41], "UNSUPPORTED"),       # 3^41 >= 2^64
        ([2305843009213693951], [2], "UNSUPPORTED"),
    ]
    for p, e in pp_ok:
        recs.append(record_construct("new_prime_powers", {"p": p, "e": e},
                                     m.new_prime_powers(p, e)))
    for p, e, want in pp_bad:
        got = m.new_prime_powers(p, e)
        assert got[0] == want, (p, e, got)
        recs.append(record_construct("new_prime_powers", {"p": p, "e": e}, got))

    # ---- the family "arbitrary integer" ----
    for K in (1, 2, 7, 65537, WORD64, TWO64, TWO64 + 1,
              2 ** 128 + 1, -(10 ** 30), 0, -5,
              2 ** 4096 + 73):            # thousands of bits
        recs.append(record_construct("new_fmpz", {"input_K": K}, m.new_fmpz(K)))

    # ---- the family "factorial" ----
    for n in (0, 1, 2, 5, 10, 20, 25, 61, 63, 64, 65, 66, 120, 2 ** 40):
        recs.append(record_construct("new_factorial", {"n": n}, m.new_factorial(n)))

    # ---- the family "power of a primorial" ----
    ppw_ok = [
        (0, 3), (1, 3), (2, 0), (10 ** 9, 0),       # K = 1 without blocks
        (2, 1), (2, 63), (3, 1), (10, 1), (10, 2), (13, 1), (100, 1), (1000, 1),
        (7, 6),
    ]
    ppw_bad = [
        (3, 63, "UNSUPPORTED"),                     # 3^63 leaves a word
        (2, 64, "UNSUPPORTED"),
        (2, 2 ** 64 - 1, "UNSUPPORTED"),
        (10 ** 9, 32, "UNSUPPORTED"),               # 5^32 leaves a word
        (2 ** 24, 1, "UNSUPPORTED"),                # more than MAX_BLOCKS primes below 2^24
    ]
    for n, e in ppw_ok:
        recs.append(record_construct("new_primorial_pow", {"n": n, "e": e},
                                     m.new_primorial_pow(n, e)))
    for n, e, want in ppw_bad:
        got = m.new_primorial_pow(n, e)
        assert got[0] == want, (n, e, got)
        recs.append(record_construct("new_primorial_pow", {"n": n, "e": e}, got))

    return recs


def word_blocks(count, rng):
    """`count` pairwise coprime word blocks: distinct primes just below 2^32."""
    if count == 0:
        return []
    primes = []
    p = (1 << 32) - 5 - 2 * rng.randrange(0, 1000)
    while len(primes) < count:
        if m.is_prime(p):
            primes.append(p)
        p -= 2
    rng.shuffle(primes)
    return primes


def roundtrip_records(rng):
    recs = []
    block_sets = [
        [2],
        [4],                                   # a block that is not a prime power
        [2, 3],
        [6, 35, 11],
        [8, 9, 5, 7],
        [2 ** 63],                             # the largest one-word power of 2
        [WORD64],                              # the largest block
        [3, 5, 7, 11, 13, 17, 19, 23, 29, 31],
    ]
    block_sets.append(word_blocks(64, rng))
    block_sets.append(word_blocks(128, rng))

    for blocks in block_sets:
        K = 1
        for q in blocks:
            K *= q
        # the edge integers, then random integers up to the modulus, then beyond it
        values = [0, 1, K - 1, K, K + 1, -1, -K, -K - 1]
        for _ in range(12):
            values.append(rng.randrange(0, K))
        for _ in range(4):
            values.append(rng.randrange(K, K * (1 << 64)))
        for _ in range(4):
            values.append(-rng.randrange(1, K + 1))
        for _ in range(2):
            values.append(rng.getrandbits(4096))
        for a in values:
            residues = m.reduce(a, blocks)
            combined = m.combine(residues, blocks)
            recs.append({"op": "reduce_combine", "blocks": blocks, "a": a,
                         "residues": residues, "combined": combined})
    return recs


def main():
    os.makedirs(VECTORS, exist_ok=True)
    rng = random.Random(SEED)

    recs = construct_records()
    with open(os.path.join(VECTORS, "ctx_construct.jsonl"), "w", encoding="utf-8") as fh:
        for rec in recs:
            fh.write(json.dumps(rec, sort_keys=True) + "\n")
    print("ctx_construct.jsonl: %d records" % len(recs))

    recs = roundtrip_records(rng)
    with open(os.path.join(VECTORS, "ctx_roundtrip.jsonl"), "w", encoding="utf-8") as fh:
        for rec in recs:
            fh.write(json.dumps(rec, sort_keys=True) + "\n")
    print("ctx_roundtrip.jsonl: %d records" % len(recs))


if __name__ == "__main__":
    main()
