#!/usr/bin/env python3
"""dump_diff.py: differential test of adf_modctx_new_from_dump (through dump_run) against
(1) my own reading of conventions 10.1, 10.2, 8.4, 8.5 for the body "modctx" (function own()),
and (2) the project reference proto/text_grammar.py modctx_new_from_dump (a second opinion).
Inputs: valid modctx dumps from random contexts, then mutations (byte changes, insertions,
deletions, truncations, token swaps), header variants, limits, occurrence indices, and valid
dumps of other bodies that record a context (fball l, scaled).
usage: python3 dump_diff.py BINARY [N]"""
import random
import re
import subprocess
import sys
from math import gcd

sys.path.insert(0, "proto")
import text_grammar as tg  # noqa: E402

W = 1 << 64
HEX = re.compile(r"(0|-?[1-9a-f][0-9a-f]*)\Z")


def own(data, occ, max_items):
    """My reading of the rules for a modctx dump. Returns "!STATUS" or (K, blocks)."""
    if len(data) > 1048576:
        return "!LIMIT"
    if any(not 0x20 <= b <= 0x7E for b in data):
        return "!PARSE"          # stage 2 (TAB/LF/CR are allowed bytes but no dump token has them)
    s = data.decode()
    m = re.match(r"adf([0-9]+)( |\Z)", s)
    if not m:
        return "!PARSE"
    v = m.group(1)
    if len(v) > 1 and v[0] == "0":
        return "!PARSE"
    if v != "1":
        return "!UNSUPPORTED"
    f = re.match(r"([A-Z][^ ]*)( |\Z)", s[m.end():])
    if not f:
        return "!PARSE"
    if f.group(1) != "Q":
        return "!UNSUPPORTED"
    toks = s[m.end() + f.end():].split(" ")
    if any(t == "" for t in toks) or toks[0] != "modctx":
        return "!PARSE"      # other bodies: see the separate report
    t = toks[1:]
    if len(t) < 2 or not all(HEX.match(x) for x in t):
        return "!PARSE"
    k = int(t[1], 16)
    if k < 0 or len(t) != 2 + k:
        return "!PARSE"
    if k > max_items:
        return "!LIMIT"
    K = int(t[0], 16)
    q = [int(x, 16) for x in t[2:]]
    if K < 1:
        return "!DOMAIN"
    if q:
        if any(not 2 <= a < W for a in q):
            return "!DOMAIN"
        if any(gcd(q[i], q[j]) != 1 for i in range(len(q)) for j in range(i)):
            return "!DOMAIN"
        p = 1
        for a in q:
            p *= a
        if p != K:
            return "!DOMAIN"
    if occ != 0:
        return "!DOMAIN"
    return (K, tuple(q))


def hx(n):
    return ("-" if n < 0 else "") + format(abs(n), "x")


def rand_ctx(r):
    c = r.random()
    if c < 0.15:
        K = r.choice([1, 6, 2 ** 64, 2 ** 64 + 1, 3 ** 50, r.getrandbits(200) + 1])
        return K, []
    k = r.randint(1, 6)
    q, pool = [], [2, 3, 4, 5, 7, 8, 9, 11, 13, 25, 27, 49, 97, 2 ** 63, 3 ** 40, W - 1, W - 59, 65537]
    r.shuffle(pool)
    for a in pool:
        if len(q) >= k:
            break
        if all(gcd(a, b) == 1 for b in q):
            q.append(a)
    K = 1
    for a in q:
        K *= a
    return K, q


def body(K, q):
    return "adf1 Q modctx " + " ".join([hx(K), hx(len(q))] + [hx(a) for a in q])


def mutate(r, s):
    b = bytearray(s.encode())
    for _ in range(r.randint(1, 3)):
        op = r.randrange(7)
        i = r.randrange(len(b) + 1)
        if op == 0 and b:
            b[min(i, len(b) - 1)] = r.choice(b"0123456789abcdefgAQ -\t\x00\x7f\xff ")
        elif op == 1:
            b[i:i] = bytes([r.choice(b"0123456789abcdef -Q\t")])
        elif op == 2 and b:
            del b[min(i, len(b) - 1)]
        elif op == 3:
            b = b[:i]
        elif op == 4:
            toks = bytes(b).split(b" ")
            if len(toks) > 3:
                j = r.randrange(3, len(toks))
                toks[j] = r.choice([b"0", b"-1", b"1", b"2", b"ffffffffffffffff", b"10000000000000000",
                                    b"-0", b"00", b"A", b"", b"6", b"3", b"2"])
                b = bytearray(b" ".join(toks))
        elif op == 5:
            b = b + b" " + r.choice([b"2", b"3", b"0", b""])
        else:
            b = bytearray(b.replace(b"adf1", r.choice([b"adf2", b"adf01", b"adf0", b"adf", b"adf1x",
                                                       b"adf2x", b"adf10", b"Adf1"]), 1))
    return bytes(b)


def main():
    binary = sys.argv[1]
    N = int(sys.argv[2]) if len(sys.argv) > 2 else 20000
    r = random.Random(20260928)
    cases = []
    fixed = [b"", b"adf", b"adf1", b"adf1 ", b"adf1 Q", b"adf1 Q ", b"adf1 Q modctx", b"adf1 Q modctx ",
             b"adf1 Q modctx 1", b"adf1 Q modctx 1 0", b"adf1 Q modctx 1 0 ", b"adf1 Qx modctx 1 0",
             b"adf1 q modctx 1 0", b"adf2 Q modctx 1 0", b"adf2x Q modctx 1 0", b"adf2\tQ modctx 1 0",
             b"adf1\tQ modctx 1 0", b"adf1 Q\tmodctx 1 0", b"adf1 Q modctx 1 0\n", b"adf9 garbage",
             b"adf1 R", b"adf1 Q modctxx 1 0", b"adf1 Q modct 1 0", b"adf1 Q modctx 6 1 6",
             b"adf1 Q modctx 6 2 2 3", b"adf1 Q modctx 5 0", b"adf1 Q modctx -1 0", b"adf1 Q modctx 0 0",
             b"adf1 Q modctx 6 -1", b"adf1 Q modctx 6 -1 6", b"adf1 Q modctx 6 1 -6",
             b"adf1 Q modctx 10000000000000000 1 10000000000000000",
             b"adf1 Q modctx 6 fffffffffffffffff 6", b"adf1 Q modctx 6 10000000000000001 6",
             b"adf1 Q  modctx 1 0", b" adf1 Q modctx 1 0", b"adf1 Q modctx 1  0",
             b"adf1 Q fball l 1 6 1 6 0", b"adf1 Q fball l 1 6 2 2 3 0 0", b"adf1 Q scaled x 1 1 6 1 6",
             b"adf1 Q scaled s 1 1 0 6 2 2 3", b"adf1 Q rat 1 1", b"adf1 Q fball g 0 1 1"]
    for f in fixed:
        for occ in (0, 1):
            cases.append((f, occ, -2))
    for _ in range(N):
        K, q = rand_ctx(r)
        s = body(K, q)
        c = r.random()
        if c < 0.25:
            cases.append((s.encode(), 0 if r.random() < 0.9 else r.choice([1, 2, 2 ** 63]), -2))
        elif c < 0.35:
            cases.append((s.encode(), 0, r.choice([-1, 0, 1, 2, 3, len(q), max(len(q) - 1, 0)])))
        else:
            cases.append((mutate(r, s), 0, -2))
    # a context with many blocks, near the default max_items is not tried (1048576 blocks > max_len)
    inp = "".join("%d %d %s\n" % (occ, mi, d.hex() or "-") for d, occ, mi in cases)
    res = subprocess.run([binary], input=inp.encode(), capture_output=True)
    out = res.stdout.decode().splitlines()
    sys.stderr.write(res.stderr.decode()[-2000:])
    assert len(out) == len(cases), (len(out), len(cases))
    diff_own, diff_ref, dump_bad = [], [], 0
    for (d, occ, mi), line in zip(cases, out):
        lim_items = 1048576 if mi == -2 else mi
        if line.startswith("OK"):
            _, K, qs, dump = line.split(" ", 3)
            got = (int(K), tuple(int(x) for x in qs.split(",") if x))
            want_dump = "DUMP=" + body(got[0], list(got[1]))
            if dump != want_dump:
                dump_bad += 1
        elif line.startswith("OUT-TOUCHED") or line.startswith("DISAGREE"):
            got = line
        else:
            got = "!" + line.replace("ADF_", "")
        mine = own(d, occ, lim_items)
        ref = tg.modctx_new_from_dump(d, occ, tg.Limits(max_items=lim_items))
        if isinstance(ref, tuple):
            ref = (ref[0], tuple(ref[1]))
        if got != mine:
            diff_own.append((d, occ, mi, got, mine))
        if got != ref:
            diff_ref.append((d, occ, mi, got, ref))
    print("inputs:", len(cases), " C != own reading:", len(diff_own), " C != reference:", len(diff_ref),
          " dump_str not canonical:", dump_bad)
    ver = re.compile(rb"adf([0-9]+)[^0-9 ]")
    cat_v = sum(1 for d, *_ in diff_own if ver.match(d) and ver.match(d).group(1) != b"1")
    other_body = sum(1 for d, occ, mi, got, want in diff_ref
                     if re.match(rb"adf1 Q (fball|scaled|adele|cadele|qclass) ", d))
    print("  C != own reading with a version other than 1 followed by a byte that is not a space or digit:",
          cat_v, "of", len(diff_own))
    print("  C != reference on a valid-looking dump of another body with a context:", other_body, "of",
          len(diff_ref))
    seen = set()
    for tag, lst in (("own", diff_own), ("ref", diff_ref)):
        for d, occ, mi, got, want in lst:
            key = (tag, str(got)[:12], str(want)[:12])
            if key in seen:
                continue
            seen.add(key)
            print("  [%s] input=%r occ=%d max_items=%d: C %s, expected %s" % (tag, d[:70], occ, mi, str(got)[:60],
                                                                              str(want)[:60]))
    sys.exit(0)


main()
