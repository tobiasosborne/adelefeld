#!exit 1
# Slice 3.1-f, docs/api-3.md 2.4 and Q3: qadd, qneg. Expected lines derived by hand before the commands.
# [1/2,1] x (0 mod 3) + [0,1] x (1 mod 3) = [1/2,2] x (1 mod 3); l = -1/2, h = 1: two pieces, crossing 1.
digits 2
qadd (0.75 +/- 0.25 ; 0 mod 3) + Q with (0.5 +/- 0.5 ; 1 mod 3) + Q with 2
qadd (0.75 +/- 0.25 ; 0 mod 3) + Q with (0.5 +/- 0.5 ; 1 mod 3) + Q with 1
# Negation reverses the end points: [-1,-1/2] x (0 mod 3) is [0,1/2] x (1 mod 3); the glued (1,0) maps to (0,1).
qneg (0.75 +/- 0.25 ; 0 mod 3) + Q with 1
qneg (0.25 ; 1/2) + Q with 1
# [-2,0] x (0 mod 1): two constructed pieces, one after deduplication; the limit counts before.
qneg (1 +/- 1 ; 0 mod 1) + Q with 2
qneg (1 +/- 1 ; 0 mod 1) + Q with 1
# Fractional radius 1/2 plus the zero class: two fibres, three pieces.
qadd (0.5 +/- 0.5 ; 0 mod 1/2) + Q with (0 ; 0) + Q with 3
qadd (0.5 +/- 0.5 ; 0 mod 1/2) + Q with (0 ; 0) + Q with 2
# Independent variation: [0,1] x (2 mod 3), an integral upper end adds no piece.
qadd (0.25 +/- 0.25 ; 1 mod 3) + Q with (0.25 +/- 0.25 ; 1 mod 3) + Q with 1
qadd (0.5 ; 1/2) + Q with (0.5 ; 1/2) + Q with 1
qadd (0 ; 0) + Q with (0 ; 0) + Q with 1
# gcd(4,6) = 2: (0 + 4 Zhat) + (1 + 6 Zhat) = 1 + 2 Zhat.
qadd (0 ; 0 mod 4) + Q with (0 ; 1 mod 6) + Q with 1
qneg (0 ; 1 mod 3) + Q with 1
qadd (0 ; 0) + Q with (0 ; 0) + Q with 0
qneg (0 ; 0) + Q with -1
qadd (0 ; 0) + Q with (0 ; 0) + Q with 1000000000000000000000000000000
qneg (0 ; 0) + Q with 1/2
qadd 0 with (0 ; 0) + Q with 1
qadd (0 ; 0) + Q with 0 with 1
qneg 0 with 1
qadd (0 ; 0) + Q with (0 ; 0) + Q
