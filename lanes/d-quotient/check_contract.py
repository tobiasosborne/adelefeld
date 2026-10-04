"""Small independent contract harness; run from repository root with timeout."""
import sys
from fractions import Fraction as F
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "proto"))
from quotient3_checks import Ball, Adele, Piece, reduce, psi, compare, phase_arcs

x = Adele(F(1), F(1, 10), Ball(0, 2))
p = reduce(x.mid, x.rad, x.fin)
assert p == [Piece(F(0), F(1, 10), 1, 2), Piece(F(9, 10), F(1), 0, 2)]
assert psi(Adele(0, 0, Ball(F(1, 3), 0))) == F(1, 3)
assert phase_arcs(psi(Adele(0, 0, Ball(0, F(1, 2))))) == [(F(0), F(0)), (F(1, 2), F(1, 2))]
assert compare([Piece(1, 1, 0, 0)], [Piece(0, 0, -1, 0)]) == (True, True, True)
assert compare([Piece(0, 0, 0, 0)], [Piece(0, 0, 0, 1)]) == (False, True, True)
print("PASS contract: 5 assertions")
