#!/usr/bin/env python3
"""Build a Place Atlas scene from one bounded driver session. Python standard library only."""
import argparse
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
RAT = r"-?\d+(?:/\d+)?"
# The drawing limits of atlas.js GEOMETRY (test_scene.py checks that the two agree through node):
# a tree row holds ROW_CAPACITY circles; MAX_PRIME is the largest prime p with 2p <= ROW_CAPACITY.
ROW_CAPACITY = 27
MAX_PRIME = 13
MISSING_REAL = "no real coordinate in the input"
STATUS_CLASSES = {
    "OK": "ok",
    "NOT_DETERMINED": "open", "UNIT_NOT_CERTIFIED": "open",
    "NEEDS_SPLIT": "open", "NOT_UNIQUE": "open",
    "DOMAIN": "proved no", "NO_SOLUTION": "proved no", "NOT_UNIT": "proved no",
    "LIMIT": "budget", "PARSE": "not mathematics", "UNSUPPORTED": "not mathematics",
}


class Refusal(ValueError):
    """An input outside what this slice can draw; scene.py exits 2."""


def parse_value(text):
    """Restrict input to conventions 9.2's rational and finite-ball start symbols."""
    if any(c in text for c in "\r\n\x00"):
        raise ValueError("a value must occupy one line")
    text = text.strip()
    body = text
    wrapped = re.fullmatch(r"\(\s*\*\s*;\s*(.*?)\s*\)", text)
    if wrapped:
        body = wrapped[1]
    match = re.fullmatch(rf"({RAT})(?:\s+mod\s+(\d+(?:/\d+)?))?", body)
    if not match:
        raise ValueError("expected a rational or a finite ball, for example (* ; 2 mod 30)")
    centre = Fraction(match[1])
    modulus = Fraction(match[2]) if match[2] else Fraction(0)
    return {"text": text, "centre": centre, "modulus": modulus,
            "kind": "fball" if wrapped or match[2] else "rational"}


def prime_list(bound):
    sieve = bytearray(b"\1") * bound
    sieve[:2] = b"\0\0"
    for p in range(2, bound):
        if sieve[p]:
            yield p
            sieve[p * p:bound:p] = b"\0" * len(range(p * p, bound, p))


def fog_count(value, drawn):
    modulus = value["modulus"]
    if not modulus:
        return {"count": "infinitely many", "label": "infinitely many undrawn primes",
                "scope": "input value; no lens computation at undrawn places"}
    # For rational N, p divides the modulus's support iff it divides numerator or denominator.
    support = [p for p in prime_list(1000)
               if p not in drawn and (modulus.numerator % p == 0 or modulus.denominator % p == 0)]
    return {"count": "every other p", "label": "every other p",
            "undrawn_modulus_primes_below_1000": support,
            "undrawn_modulus_prime_count_below_1000": len(support),
            "scope": "input modulus support below 1000; larger factors not examined"}


def row_sizes(paths, p):
    """The number of circles in each row of a tree, as chooseNodes in atlas.js draws them."""
    rows = {0: {()}}
    for path in paths:
        for i in range(len(path)):
            children = range(p) if i < 6 else [path[i]]
            rows.setdefault(i + 1, set()).update(tuple(path[:i]) + (d,) for d in children)
    return [len(rows[k]) for k in range(len(rows))]


def check_rows(paths, p, where=""):
    widest = max(row_sizes(paths, p))
    if widest > ROW_CAPACITY:
        raise Refusal(f"a row of {widest} circles{where} exceeds the capacity {ROW_CAPACITY} of a tree row "
                      "in atlas.js")


def polynomial_text(coefficients):
    """Integer coefficients, constant first, as the driver's roots command takes them, written X^n first."""
    terms = []
    for k, c in reversed(list(enumerate(int(c) for c in coefficients.split()))):
        if c:
            monomial = "" if k == 0 else "X" if k == 1 else f"X^{k}"
            terms.append(("-" if c < 0 else "+", (str(abs(c)) if k == 0 or abs(c) != 1 else "") + monomial))
    if not terms:
        return "0"
    text = ("-" if terms[0][0] == "-" else "") + terms[0][1]
    return text + "".join(f" {sign} {body}" for sign, body in terms[1:])


def valuation(q, p):
    if q == 0:
        return 0
    n, d, v = abs(q.numerator), q.denominator, 0
    while n % p == 0:
        n //= p
        v += 1
    while d % p == 0:
        d //= p
        v -= 1
    return v


def digit_path(centre, p, precision):
    """Low digit first, from a driver's rational centre; never use a saved expansion.

    Source: refs/src/thorne-padic/jackthornenotes.txt:183-188, Proposition 1.10:
    "there exists a unique sequence" of digits in 0..p-1 giving the p-adic expansion.
    Own finite computation: let s=min(0,v_p(q)), q=p^s*n/d with p not dividing d.
    Invert d modulo p^(N-s), obtaining the unique residue r. Repeated divmod(r,p)
    gives r=sum digit_i*p^i. Thus every prefix is the centre of q+p^k Z_p.
    """
    q = Fraction(centre)
    start = min(0, valuation(q, p), precision)
    if precision < start:
        return {"start_exponent": start, "digits": [], "digit_string": "",
                "nodes": [], "end_exponent": precision}
    scale = Fraction(p) ** start
    unit = q / scale
    modulus = p ** (precision - start)
    residue = (unit.numerator * pow(unit.denominator, -1, modulus)) % modulus if modulus > 1 else 0
    digits, nodes, prefix = [], [], Fraction(0)
    for exponent in range(start, precision):
        residue, digit = divmod(residue, p)
        digits.append(digit)
        prefix += digit * Fraction(p) ** exponent
        k = exponent + 1
        nodes.append({"exponent": k, "centre": str(prefix),
                      "value_form": f"[p={p}: {prefix} + O({p}^{k})]"})
    return {"start_exponent": start, "end_exponent": precision, "digits": digits,
            "digit_string": " ".join(map(str, digits)), "nodes": nodes}


def parse_real(text):
    dec = r"-?\d+(?:\.\d+)?(?:e[+-]?\d+)?"
    match = re.fullmatch(rf"({dec})(?: \+/- ({dec}))?", text)
    if not match:
        raise ValueError(f"unrecognised real-ball output: {text}")
    mid = Fraction(match[1])
    rad = Fraction(match[2]) if match[2] else Fraction(0)
    # Decimal enclosures do not identify arb's stored dyadic endpoints (conventions 9.5).
    dyadic = not rad and (mid.denominator & (mid.denominator - 1)) == 0
    return {"printed": text, "midpoint_printed": match[1], "radius_printed": match[2] or "0",
            "dyadic_endpoints": [text, text] if dyadic else "not given",
            "exact_point": str(mid) if not rad else None}


def parse_output(line, prime=None):
    """Parse polynomial certificates, seeded local branches, partial components, or an error."""
    result = {"status": "OK", "status_class": "ok", "balls": [], "printed": line,
              "failing_place": "not given"}
    if line.startswith("error: "):
        status = line[7:]
        if status not in STATUS_CLASSES:
            raise ValueError(f"unknown driver status: {status}")
        result.update(status=status, status_class=STATUS_CLASSES[status])
        return result
    if line == "none":
        return result
    for entry in line.split("; "):
        original, seed = entry, "not given"
        seeded = re.fullmatch(r"(\d+) \[([^]]+)\]: (.+)", entry)
        if seeded:
            if int(seeded[1]) != prime:
                raise ValueError("branch belongs to another prime")
            seed, entry = seeded[2], seeded[3]
        elif re.match(r"\[([^]]+)\] ", entry):
            seed, entry = re.fullmatch(r"\[([^]]+)\] (.+)", entry).groups()
        else:
            labelled = re.fullmatch(r"(\d+): (.+)", entry)
            if labelled:
                if int(labelled[1]) != prime:
                    raise ValueError("component belongs to another prime")
                entry = labelled[2]
        cert = re.fullmatch(rf"({RAT}) mod (\d+)\^(-?\d+)", entry)
        local = re.fullmatch(rf"({RAT}) \+ O\((\d+)\^(-?\d+)\)", entry)
        match = cert or local
        if match:
            if int(match[2]) != prime:
                raise ValueError("ball base differs from the place")
            centre, precision = match[1], int(match[3])
        elif re.fullmatch(RAT, entry):
            centre, precision = entry, None
        else:
            raise ValueError(f"unrecognised local-ball output: {original}")
        result["balls"].append({"centre": centre, "precision": precision,
                                "seed": seed, "printed": original})
    return result


class Session:
    def __init__(self):
        self.lines = []
        self.commands = []

    def setting(self, text):
        self.lines.append(text)

    def add(self, command):
        self.lines.append(command)
        self.commands.append(command)
        return len(self.commands) - 1

    def run(self, driver):
        stdin = "\n".join(self.lines) + "\n"
        run = subprocess.run(["timeout", "120", str(driver)], input=stdin, text=True,
                             capture_output=True, timeout=125, cwd=ROOT, check=False)
        if run.returncode not in (0, 1):
            raise RuntimeError(f"driver exit {run.returncode}: {run.stderr}")
        outputs = run.stdout.splitlines()
        if len(outputs) != len(self.commands):
            raise RuntimeError(f"driver wrote {len(outputs)} lines for {len(self.commands)} commands")
        self.records = [{"command": c, "output": o} for c, o in zip(self.commands, outputs)]
        return {"stdin": stdin, "stdout": run.stdout, "stderr": run.stderr,
                "exit_code": run.returncode, "records": self.records,
                "driver_source": "tools/adf/adf.c",
                "driver_source_sha256": hashlib.sha256((ROOT / "tools/adf/adf.c").read_bytes()).hexdigest(),
                "build_stamp": "not given"}

    def output(self, record):
        return self.records[record]["output"]


def polynomial_for(value, degree):
    q = value["centre"]
    coefficients = [-q.numerator] + [0] * (degree - 1) + [q.denominator]
    return " ".join(map(str, coefficients))


def decorate(result, prime, depth, record, seeds=None):
    result["record"] = record
    result["requested_depth"] = depth
    result["paths"] = []
    for ball in result.pop("balls"):
        precision = depth if ball["precision"] is None else min(depth, ball["precision"])
        path = digit_path(ball["centre"], prime, precision)
        ball.update(path)
        ball["certificate_precision"] = ball.pop("precision")
        if seeds:
            match_precision = ball["certificate_precision"] or precision
            matching = []
            for branch in seeds["balls"]:
                if branch["precision"] is not None and branch["precision"] < match_precision:
                    continue
                difference = Fraction(branch["centre"]) - Fraction(ball["centre"])
                if difference == 0 or valuation(difference, prime) >= match_precision:
                    matching.append(branch["seed"])
            if len(matching) == 1:
                ball["seed"] = matching[0]
                ball["seed_source_record"] = seeds["record"]
        ball["source_record"] = record
        result["paths"].append(ball)
    result["branch_count"] = len(result["paths"])
    result["lit_path_count"] = len({(p["start_exponent"], tuple(p["digits"])) for p in result["paths"]})
    result["node_balls"] = node_balls(result["paths"], prime)
    return result


def node_balls(paths, p):
    """Precompute even unlit siblings' hover text. The page only chooses and lays out nodes."""
    nodes = {}
    start = min((path["start_exponent"] for path in paths), default=0)
    nodes[""] = f"[p={p}: 0 + O({p}^{start})]"
    for path in paths:
        prefix = Fraction(0)
        digits = path["digits"]
        for i, digit in enumerate(digits):
            exponent = path["start_exponent"] + i
            for sibling in (range(p) if i < 6 else [digit]):
                key = ".".join(map(str, digits[:i] + [sibling]))
                centre = prefix + sibling * Fraction(p) ** exponent
                nodes[key] = f"[p={p}: {centre} + O({p}^{exponent + 1})]"
            prefix += digit * Fraction(p) ** exponent
    return nodes


DERIVED = {
    "rule": "D-UX5 as amended 2026-10-16: these numbers are computed in scene.py by a finite computation "
            "from a printed driver line, and each names that line by its index in provenance.records",
    "digits": "trees[].depths[].paths[]: digits, digit_string, nodes and end_exponent are the base-p digits "
              "(digit_path) of centre, printed in provenance.records[source_record].output",
    "node_balls": "trees[].depths[].node_balls: the drawn sibling balls, from the digits of the paths of the "
                  "same depth; source provenance.records[paths[].source_record]",
    "counts": "trees[].depths[]: branch_count and lit_path_count count the balls of "
              "provenance.records[record].output and their distinct digit paths",
    "fog": "fog: the primes below 1000 dividing the modulus printed in provenance.records[fog.source_record]"
           ".output, and their count",
    "modular_digits": "modular_solutions[].digits: the first digits of the components printed in "
                      "provenance.records[records[3]].output",
}


def build_scene(value_text, lens, primes, d_min, d_max, driver, polynomial=None, modular=None):
    value = parse_value(value_text)
    primes = sorted(primes)
    if not 1 <= len(primes) <= 4 or len(set(primes)) != len(primes):
        raise ValueError("choose one to four distinct primes")
    if any(p < 2 or any(p % d == 0 for d in range(2, int(p ** 0.5) + 1)) for p in primes):
        raise ValueError("each place must be a prime")
    for p in primes:
        if p > MAX_PRIME:
            raise Refusal(f"prime {p} is above the drawing limit {MAX_PRIME} of this slice: a tree row in "
                          f"atlas.js holds {ROW_CAPACITY} circles and two branches put 2p in one row")
    if not 1 <= d_min <= d_max <= 5:
        raise ValueError("depth range must satisfy 1 <= min <= max <= 5")
    if lens not in ("identity", "exp", "log") and not re.fullmatch(r"roots [1-9]\d*", lens):
        raise ValueError("lens is identity, roots n, exp, or log")
    degree = int(lens.split()[1]) if lens.startswith("roots ") else None
    if degree and degree > 64:
        raise ValueError("this slice caps root degree at 64")
    if polynomial is not None:
        if not degree or not re.fullmatch(r"-?\d+(?: -?\d+)*", polynomial):
            raise ValueError("--polynomial requires roots n and integer coefficients, constant first")
    elif degree and not value["modulus"]:
        polynomial = polynomial_for(value, degree)
    if modular and (polynomial != "0 -1 1" or modular > 1000 or modular < 2):
        raise ValueError("--modular is bounded to 2..1000 and polynomial 0 -1 1")
    session = Session()
    session.setting("prec 64")
    session.setting("digits 20")
    canonical = session.add(f"show {value_text}")
    projection = session.add(f"project {value_text} with {' '.join(map(str, primes))}")
    real_projection = session.add(f"project {value_text} with real")
    if degree and polynomial:
        real_record = session.add(f"realroots {polynomial}")
    elif lens in ("exp", "log"):
        real_record = session.add(f"{lens}_at {value_text} with real")
    else:
        real_record = real_projection
    thread_records = []
    if value["kind"] == "rational" and degree and polynomial == polynomial_for(value, degree):
        for sign in ([1, -1] if degree % 2 == 0 else [1]):
            thread_records.append(session.add(f"root {value_text} with {degree} with {sign}"))
    local_records = {}
    for depth in range(d_min, d_max + 1):
        session.setting(f"prec {depth}")
        for p in primes:
            seed_record = None
            if degree and polynomial:
                record = session.add(f"roots {polynomial} with {p} with {depth}")
                # Also ask the branch API: polynomial certificates have no seed field.
                if polynomial == polynomial_for(value, degree) and value["kind"] == "rational":
                    seed_record = session.add(f"roots_at {value_text} with {p} with {degree}")
                    if valuation(value["centre"], p) < 0:
                        record = seed_record  # polynomial roots lists Z_p only; roots_at also reaches Q_p.
            elif degree:
                record = session.add(f"roots_at {value_text} with {p} with {degree}")
            elif lens == "identity":
                record = session.add(f"project {value_text} with {p}")
            else:
                record = session.add(f"{lens}_at {value_text} with {p}")
            local_records[p, depth] = record, seed_record
    modular_records = []
    if modular:
        # Own exhaustive proof for this bounded modulus: try each residue once. Keep only
        # residues for which m divides e(e-1). Driver calls below independently check each.
        for e in range(modular):
            if (e * e - e) % modular == 0:
                show = session.add(f"show {e}")
                square = session.add(f"mul {e} with {e}")
                contained = session.add(f"contains {e * e} with {e} mod {modular}")
                project = session.add(f"project {e} with {' '.join(map(str, primes))}")
                modular_records.append((show, square, contained, project))
    provenance = session.run(driver)
    if session.output(canonical).startswith("error:"):
        raise ValueError(f"input rejected by driver: {session.output(canonical)}")
    trees = []
    for p in primes:
        depths = {}
        for depth in range(d_min, d_max + 1):
            record, seed_record = local_records[p, depth]
            seeds = parse_output(session.output(seed_record), p) if seed_record is not None else None
            if seeds is not None:
                seeds["record"] = seed_record
            depths[str(depth)] = decorate(parse_output(session.output(record), p), p, depth, record, seeds)
            check_rows([path["digits"] for path in depths[str(depth)]["paths"]], p,
                       f" at prime {p}, depth {depth}")
        trees.append({"prime": p, "depths": depths})
    real_line = session.output(real_record)
    if real_line.startswith("error: "):
        real = parse_output(real_line)
        real.pop("balls")
        real["balls"] = []
    else:
        text = real_line.removeprefix("real: ")
        real = {"status": "OK", "status_class": "ok", "printed": real_line,
                "balls": [] if text == "none" else [parse_real(t) for t in text.split("; ")]}
    real.update(record=real_record, failing_place="not given", coordinate="real")
    # docs/conventions.md:1183, fball_v = "(" "*" ";" fin ")": a printed "(* ; ...)" has no real coordinate.
    # The reason is given only when the driver also refused the input's own projection to the real place.
    if (real["status"] != "OK" and session.output(canonical).startswith("(* ;")
            and session.output(real_projection).startswith("error: ")):
        real.update(reason=MISSING_REAL, reason_records=[canonical, real_projection])
    threads = []
    if value["kind"] == "rational" and lens == "identity":
        thread_records = [canonical]
    thread_candidates = [(session.output(record), record) for record in thread_records]
    if value["kind"] == "rational" and lens in ("exp", "log") and len(real["balls"]) == 1:
        candidate = real["balls"][0]["exact_point"]
        if candidate is not None and all(
            len(t["depths"][str(d_max)]["paths"]) == 1
            and t["depths"][str(d_max)]["paths"][0]["certificate_precision"] is None
            and t["depths"][str(d_max)]["paths"][0]["centre"] == candidate for t in trees
        ):
            thread_candidates.append((candidate, real_record))
    for q_text, record in thread_candidates:
        if not re.fullmatch(RAT, q_text) or any(t["value"] == q_text for t in threads):
            continue
        threads.append({"value": q_text, "record": record, "real_point": q_text,
                        "places": {str(p): {str(d): digit_path(q_text, p, d)
                                           for d in range(d_min, d_max + 1)} for p in primes}})
    modular_solutions = []
    for show, square, contains, project in modular_records:
        if session.output(contains) != "true":
            raise RuntimeError("driver rejected an enumerated modular idempotent")
        e_text = session.output(show)
        entries = session.output(project).split("; ")
        residues = [digit_path(entry.split(": ")[1], p, 1)["digits"][0]
                    for entry, p in zip(entries, primes)]
        modular_solutions.append({"residue": e_text, "digits": residues,
                                  "square": session.output(square), "contains": session.output(contains),
                                  "records": [show, square, contains, project]})
    fog = fog_count(parse_value(session.output(canonical)), primes)
    fog["source_record"] = canonical
    return {"schema": "adelefeld-place-atlas-1", "value": session.output(canonical), "lens": lens,
            "polynomial": polynomial, "solved_polynomial": polynomial_text(polynomial) if polynomial else None,
            "places": ["real"] + primes,
            "depth_range": [d_min, d_max], "trees": trees, "ruler": real, "threads": threads,
            "fog": fog, "modulus": modular, "modular_solutions": modular_solutions,
            "projection": {"finite_record": projection, "real_record": real_projection},
            "notes": ["Low digit at the root; certificates may be deeper than the requested drawing.",
                      "Seed not given means the polynomial solver printed no branch identifier.",
                      "Depth handles select stored results. The viewer performs no arithmetic on values."],
            "derived": DERIVED,
            "provenance": provenance}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("value")
    parser.add_argument("--lens", default="identity")
    parser.add_argument("--primes", nargs="+", type=int, default=[2, 3, 5])
    parser.add_argument("--depth-min", type=int, default=1)
    parser.add_argument("--depth-max", type=int, default=5)
    parser.add_argument("--polynomial", help="override X^n - value; integer coefficients, constant first")
    parser.add_argument("--modular", type=int, help="enumerate and check the idempotents of this modulus")
    parser.add_argument("--driver", type=Path, default=ROOT / "build/adf")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    try:
        scene = build_scene(args.value, args.lens, args.primes, args.depth_min, args.depth_max,
                            args.driver.resolve(), args.polynomial, args.modular)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(scene, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    except Refusal as refusal:
        parser.exit(2, f"scene: {refusal}\n")
    except (ValueError, OSError, RuntimeError, subprocess.TimeoutExpired, ZeroDivisionError) as error:
        parser.exit(1, f"scene: {error}\n")


if __name__ == "__main__":
    main()
