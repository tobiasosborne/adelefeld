# Check commands

All commands ran from the repository root. The Python -c bodies are displayed with line breaks
instead of semicolon separators. These are the same statements as the executed one-line probes.
Read-only source inspection is not counted as a test. No C test or mutation run was performed.

## Dependency and API probe

```sh
timeout 15 python3 -B -c '
import mpmath, sympy, flint
print("mpmath", mpmath.__version__, "sympy", sympy.__version__, "python-flint", flint.__version__)
print("acb exp_pi_i", hasattr(flint.acb, "exp_pi_i"))
print("acb methods", [x for x in dir(flint.acb)
    if x in ("contains", "overlaps", "real", "imag", "rad", "mid", "sqrt", "exp", "conjugate")])
print("arb methods", [x for x in dir(flint.arb)
    if x in ("lower", "upper", "is_positive", "contains", "rad", "mid")])
'
```

Exit 0. Versions: mpmath 1.3.0, sympy 1.14.0, python-flint 0.8.0.
exp_pi_i=True. Eight requested acb methods and five requested arb methods were present.
The final oracle additionally prints the runtime FLINT version, 3.3.1.

## Oracle runs

The redirects below are literal. Every run ended or was killed within its 120-second timeout.

```sh
timeout 120 python3 -B proto/functions4_checks.py > lanes/d-functions4/oracle-first.log 2>&1
timeout 120 python3 -B proto/functions4_checks.py > lanes/d-functions4/oracle-second.log 2>&1
timeout 120 python3 -B -u proto/functions4_checks.py > lanes/d-functions4/oracle-third.log 2>&1
timeout 120 python3 -B -u proto/functions4_checks.py > lanes/d-functions4/oracle-fourth.log 2>&1
timeout 120 python3 -u proto/functions4_checks.py > lanes/d-functions4/oracle-fifth.log 2>&1
timeout 120 python3 -u proto/functions4_checks.py > lanes/d-functions4/oracle-sixth.log 2>&1
timeout 120 python3 proto/functions4_checks.py > lanes/d-functions4/oracle-final.log 2>&1
```

| Log | Exit | Result |
|---|---|---|
| first | 1 | evaluation assertion 604 failed: my expected {2} omitted index 5 |
| second | 124 | 120-second timeout; 2002 checks printed before the unbounded near-zero tail search |
| third | 0 | 2067 checks |
| fourth | 0 | 2099 checks |
| fifth | 0 | 2100 checks |
| sixth | 0 | 2230 checks |
| final | 0 | 2670 checks |

The second run had a mathematically terminating but impractical ratio search for A=10^-8.
The oracle now rejects that search before its long prefix, and checks a first-omitted-term lower bound.
The coefficient at n=4097 is still greater than 1e-20, so cutoff 4096 cannot meet that error target.
The design charges all prefix work; no convergence assertion was relaxed.

## Layout and syntax probes

```sh
timeout 5 python3 -B -c '
from pathlib import Path
p=Path("docs/api-4.md")
lines=p.read_text().splitlines()
print("lines",len(lines))
print("long",[(i,len(s)) for i,s in enumerate(lines,1) if len(s)>116])
'
```

Exit 0: 451 design lines; five lines exceeded 116 characters (lengths 157,179,157,174,160).
Those table rows were replaced by wrapped prose.

```sh
timeout 5 python3 -B -c '
from pathlib import Path
paths=[Path("docs/api-4.md"),Path("proto/functions4_checks.py"),Path("lanes/d-functions4/progress.md")]
print([(str(p),len(p.read_text().splitlines()),
    [(i,len(s)) for i,s in enumerate(p.read_text().splitlines(),1) if len(s)>116]) for p in paths])
'
```

Exit 0: respective line counts 456,622,14; zero overlength lines in all three files.

```sh
timeout 5 python3 -B -c '
from pathlib import Path
import ast
p=Path("proto/functions4_checks.py")
ast.parse(p.read_text())
d=Path("docs/api-4.md").read_text().splitlines()
print("syntax=1 design_lines="+str(len(d)))
print("long_lines="+str(sum(len(s)>116 for s in d)))
assert len(d)<=500 and all(len(s)<=116 for s in d)
'
```

Exit 0: one Python syntax tree parsed; design 457 lines; zero overlength lines.

```sh
timeout 5 python3 -B -c '
from pathlib import Path
paths=[Path("docs/api-4.md"),Path("proto/functions4_checks.py"),Path("lanes/d-functions4/progress.md")]
print("files",len(paths))
print("lines",[(str(p),len(p.read_text().splitlines())) for p in paths])
print("over_116",sum(len(s)>116 for p in paths for s in p.read_text().splitlines()))
'
```

Exit 0: three files; line counts 457,640,29; zero overlength lines.

Final artifact line check, before writing the report:

```sh
timeout 5 python3 -B -c '
from pathlib import Path
paths=[Path("docs/api-4.md"),Path("proto/functions4_checks.py"),
    Path("lanes/d-functions4/progress.md"),Path("lanes/d-functions4/check-commands.md")]
bad=[(str(p),i,len(s)) for p in paths
    for i,s in enumerate(p.read_text().splitlines(),1) if len(s)>116]
print("files",len(paths),"over_116",bad)
assert not bad
'
```

Exit 0: four files, zero overlength lines. This final command record was appended after that check.
