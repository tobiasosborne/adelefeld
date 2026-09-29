"""Plants faults in the Python reference of the section f-slice2 (by monkeypatching) and checks that the finite checks of
the section reject each one. Run from the repository root: python3 -B lanes/f-slice2/plant_ref.py"""
import sys
from fractions import Fraction as F
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "proto"))
import functions_checks as R  # noqa: E402

orig_real_op = R.sb_real_op
orig_project = R.lb_ref_project
orig_image = R.rf_image


def bad_sub(op, x, y=None):
    if op == "sub":
        return (x.lo - y.lo, x.hi - y.hi)          # wrong: not lo - hi'
    return orig_real_op(op, x, y)


def bad_mul(op, x, y=None):
    if op == "mul":
        return (x.lo * y.lo, x.hi * y.hi)          # wrong: not the min and max of the four products
    return orig_real_op(op, x, y)


def bad_project(p, A, H, d):
    r = orig_project(p, A, H, d)
    if H > 0 and not r.exact:
        return R.lb_ball(p, F(A, d), R.vp(H, p) - R.vp(d, p) + 1)   # one digit too fine
    return r


def bad_image(f, n, lo, hi):
    a, b = orig_image(f, n, lo, hi)
    return (a, b - (b - a) / 2) if f == "exp" else (a, b)            # an upper bound that is too small


plants = [
    ("sb_real_op: sub uses lo - lo'", check := R.check_sball_tuple_enumeration, "sb_real_op", bad_sub),
    ("sb_real_op: mul uses lo lo', hi hi'", R.check_sball_tuple_enumeration, "sb_real_op", bad_mul),
    ("lb_ref_project: one digit too fine", R.check_sball_projection_enumeration, "lb_ref_project", bad_project),
    ("rf_image: exp upper bound too small", R.check_real_reference_against_arb, "rf_image", bad_image),
]
rejected = 0
for name, check, attr, fn in plants:
    saved = getattr(R, attr)
    setattr(R, attr, fn)
    try:
        check()
        print(f"NOT rejected: {name}")
    except AssertionError:
        print(f"rejected: {name}")
        rejected += 1
    finally:
        setattr(R, attr, saved)
print(f"{rejected} of {len(plants)} planted faults rejected")
sys.exit(0 if rejected == len(plants) else 1)
