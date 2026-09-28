#!/usr/bin/env python3
"""Print the lines of the FLINT documentation and of the headers that src/rat.c, src/fball.c
(global part), src/adele.c and src/recon.c cite, so that each citation can be read against its
claim. Run from the root of the worktree (refs/src must exist)."""
D = "refs/src/flint-3.0.1/"
cites = {
    D + "arb.rst": [11, 12, 13, 14, 23, 24, 84, 89, 102, 137, 141, 167, 168, 169, 461, 462, 463, 464,
                    465, 466, 468, 469, 470, 472, 473, 474, 606, 623, 729, 767, 785, 798, 856],
    D + "acb.rst": [58, 62, 104, 114, 128, 138, 224, 240, 411, 429, 441, 463, 469, 521],
    D + "fmpq.rst": [19, 20, 21, 22, 23, 24, 25, 26, 196, 197, 198, 400, 408, 409, 410, 419, 420,
                     475, 476, 477, 478, 519],
    "/usr/include/flint/fmpq.h": [28, 32, 34, 38, 40, 45, 52, 56, 58, 61, 63, 66, 78, 82, 84, 88,
                                  90, 94, 115, 117, 118, 136, 139, 142, 200, 209, 230, 245, 248],
    "/usr/include/flint/fmpz.h": [472, 473],
}
for f, lines in cites.items():
    txt = open(f).read().split("\n")
    print("==", f)
    for n in lines:
        print("%5d: %s" % (n, txt[n - 1]))
