#!/usr/bin/env python3
"""Generate tests/ref/vectors/m1-modctx/modctx.jsonl from the Python reference
tests/ref/adfref/modctx_ref.py.

The C test tests/test_modctx.c reads the file with tests/support/jsonl.h and runs every
record.  Deterministic: a fixed seed and a fixed hand-written list.  Run from the
repository root:

    python3 lanes/m1-modctx-b/gen_modctx_vectors.py
"""
import json
import os
import random
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "tests", "ref"))
from adfref import modctx_ref as M  # noqa: E402

OUT = "tests/ref/vectors/m1-modctx/modctx.jsonl"


def result(fn, op, **fields):
    rec = {"op": op}
    rec.update(fields)
    try:
        K, blocks = fn()
        rec["result"] = {"K": K, "blocks": list(blocks)}
    except M.ModctxError as e:
        rec["status"] = e.status
    return rec


def rec_from_dump(text, occurrence=0, max_items=1048576, max_len=1048576):
    rec = {"op": "from_dump", "text": text, "occurrence": occurrence,
           "max_items": max_items, "max_len": max_len}
    try:
        K, blocks = M.new_from_dump(text, occurrence, max_len=max_len, max_items=max_items)
        rec["result"] = {"K": K, "blocks": list(blocks)}
    except M.ModctxError as e:
        rec["status"] = e.status
    return rec


def main():
    rng = random.Random(20260928)
    recs = []

    # --- new_blocks ---
    for q in ([], [6], [2, 3], [3, 2], [4, 9, 25, 49], [2, 2], [6, 10], [0], [1],
              [1, 2], [3, 5, 15]):
        recs.append(result(lambda q=q: M.new_blocks(list(q)), "new_blocks", q=list(q)))

    # --- new_prime_powers ---
    for p, e in (([], []), ([2], [5]), ([2, 3, 5], [1, 2, 3]), ([3, 2], [2, 2]),
                 ([4], [1]), ([2], [0]), ([2, 2], [1, 1]), ([2], [64]), ([3], [40]),
                 ([2, 3], [63, 40])):
        recs.append(result(lambda p=p, e=e: M.new_prime_powers(list(p), list(e)),
                           "new_prime_powers", p=list(p), e=list(e)))

    # --- new_fmpz ---
    for K in (0, 1, 2, 6, M.WORD - 1, M.WORD, M.WORD + 1, 10 ** 30):
        recs.append(result(lambda K=K: M.new_fmpz(K), "new_fmpz", K=K))

    # --- new_factorial ---
    for n in (0, 1, 2, 3, 5, 10, 20, 30, 50, 65, 66, 100):
        recs.append(result(lambda n=n: M.new_factorial(n), "new_factorial", n=n))

    # --- new_primorial_pow ---
    for n, e in ((0, 3), (1, 3), (2, 0), (2, 1), (2, 63), (2, 64), (3, 2), (5, 2),
                 (7, 1), (11, 2), (19, 1), (23, 1), (29, 1), (13, 1)):
        recs.append(result(lambda n=n, e=e: M.new_primorial_pow(n, e),
                           "new_primorial_pow", n=n, e=e))

    # --- round trips: 1, 2, 64, 128 blocks ---
    families = [
        [M.WORD - 1],
        [2, 3],
        M.primes_upto(400)[:64],
        M.primes_upto(800)[:128],
        [4, 9, 25, 49, 121],
    ]
    for blocks in families:
        K, blocks = M.new_blocks(blocks)
        vals = [0, 1, K - 1, K // 2, K // 3 + 1]
        vals += [rng.randrange(0, K) for _ in range(5)]
        for a in vals:
            res = M.reduce(K, blocks, a)
            back = M.recombine(K, blocks, res)
            recs.append({"op": "roundtrip", "K": K, "blocks": list(blocks), "a": a,
                         "result": {"res": res, "back": back}})

    # --- dump bodies ---
    for blocks in ([], [2, 3], [6], [4, 9, 25], M.primes_upto(200)[:16]):
        if blocks:
            K, blocks = M.new_blocks(blocks)
        else:
            K, blocks = 1, []
        recs.append({"op": "dump", "K": K, "blocks": list(blocks),
                     "result": M.dump_str(K, blocks)})
    K, blocks = M.new_fmpz(10 ** 30)
    recs.append({"op": "dump", "K": K, "blocks": list(blocks), "result": M.dump_str(K, blocks)})

    # --- from_dump: valid and every failure ---
    texts = ("adf1 Q modctx 6 2 2 3", "adf1 Q modctx 6 2 3 2", "adf1 Q modctx 1 0",
             "adf1 Q modctx 6 1 6", "adf1 Q modctx 10000000000000000 0",
             "adf1 Q modctx ffffffffffffffff 1 ffffffffffffffff",
             "adf1 Q modctx 6 0", "adf1 Q modctx 7 0",
             "adf1 Q modctx 0 0", "adf1 Q modctx 6 2 2 2", "adf1 Q modctx 7 2 2 3",
             "adf1 Q modctx 1 1 1", "adf1 Q modctx 10000000000000000 1 10000000000000000",
             "adf1 Q modctx 6 2 2 3 5", "adf1 Q modctx 6 2 2", "adf2 Q modctx 6 1 6",
             "adf1 K modctx 6 1 6", "adf1 Q modctx 07 1 6", "adf01 Q modctx 6 1 6",
             "adf1 Q modctx -6 1 6", "adf1 Q modctx 6 -1", "adf1 Q modctx 6 1 06",
             "adf1 Q modctx 6 2 2 3\x20", "adf1  Q modctx 6 1 6",
             "adf1 Q fball g 1 6 1")
    for text in texts:
        recs.append(rec_from_dump(text))
    recs.append(rec_from_dump("adf1 Q modctx 6 2 2 3", occurrence=1))
    recs.append(rec_from_dump("adf1 Q modctx 6 2 2 3", max_items=1))
    recs.append(rec_from_dump("adf1 Q modctx 6 2 2 3", max_len=5))
    recs.append(rec_from_dump("adf1 Q modctx 6 2 2 3", max_items=2))

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="ascii") as f:
        for r in recs:
            f.write(json.dumps(r, sort_keys=True) + "\n")
    print("wrote %d records to %s" % (len(recs), OUT))


if __name__ == "__main__":
    main()