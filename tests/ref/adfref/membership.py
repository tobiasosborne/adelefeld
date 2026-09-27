"""Membership of a rational in a finite ball, from the definition, without ball arithmetic.

docs/proofs/precision.md, Lemma 1 (lines 13-18): for a positive rational R, R Zhat meets Q in
R Z; a rational lies in Zhat exactly when it is an integer (no prime divides its denominator).
Hence, for H > 0, the rational x lies in (A + H Zhat)/d exactly when

    (x - A/d) / (H/d) = (x d - A) / H

has no prime in its denominator. H = 0 is the single point A/d.
"""
from fractions import Fraction


def rational_in_fball(x, ball):
    """Independent membership test for the set (A + H Zhat)/d."""
    x = Fraction(x)
    A, H, d = ball.A, ball.H, ball.d
    if H == 0:
        return x == Fraction(A, d)
    return ((x * d - A) / H).denominator == 1
