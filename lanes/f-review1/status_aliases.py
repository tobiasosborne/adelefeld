from oracle import Q, call, proc, SENTINEL

E = 2**60
one = (5, 1, Q(1), 0, 0)
zero = (5, 1, Q(0), 0, 0)
zb = (5, 0, Q(0), 0, 4)
fine = (5, 0, Q(1), 0, E)
outside = (5, 0, Q(1), 0, E+1)
other = (3, 1, Q(1), 0, 0)
cases = []
for op in ("add", "sub", "mul", "div"):
    cases += [(op, one, other, "DOMAIN"), (op, outside, one, "LIMIT")]
for op in ("neg", "inv"):
    cases += [(op, outside, one, "LIMIT")]
for op in ("inv", "div"):
    for d, status in ((zero, "NOT_UNIT"), (zb, "UNIT_NOT_CERTIFIED")):
        cases.append((op, d if op == "inv" else one, d, status))
cases += [("sub", fine, fine, "LIMIT"), ("neg", fine, one, "LIMIT")]
checks = 0
try:
    for op, x, y, st in cases:
        for alias, untouched in ((0, SENTINEL), (1, x), (2, y)):
            got = call(op, x, y, alias)
            assert got[:2] == (st, untouched), (op, x, y, alias, got)
            checks += 1
    for op in ("eq", "over", "inside", "ident"):
        assert call(op, one, other)[3] == 0
        checks += 1
    assert call("prec", one)[0:4:2] == ("DOMAIN", 987)
    checks += 1
    print(f"nonOK_cases={len(cases)} alias_status_calls={len(cases)*3} total_checks={checks} failures=0")
finally:
    proc.stdin.close()
    assert proc.wait(timeout=5) == 0
