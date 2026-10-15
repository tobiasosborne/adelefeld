# lanes/ux-notebook/mutate.py: a bounded mutation run over the slice's Julia sources (CLAUDE.md rule 2).
# Each mutant is one textual change; the file is restored after its test run in every case.
import subprocess, sys, pathlib
S = pathlib.Path("ux/notebook/src")
ONLY = [int(a) for a in sys.argv[1:]]
M = [
 ("digits.jl", "x = div(x - dg * d, p)", "x = div(x + dg * d, p)"),
 ("digits.jl", "pre = vcat(zeros(Int, e.v), pre)", "pre = vcat(pre, zeros(Int, e.v))"),
 ("digits.jl", "v = (a == 0 || vp(a, p) >= k) ? k : vp(a, p)", "v = (a == 0 || vp(a, p) > k) ? k : vp(a, p)"),
 ("digits.jl", "    lo = min(v, 0)\n", "    lo = v\n"),
 ("digits.jl", "for p in SMALL_PRIMES", "for p in SMALL_PRIMES[2:end]"),
 ("valuetext.jl", 'const RE_URAT = r"^[0-9]+(/[0-9]+)?$"', 'const RE_URAT = r"^-?[0-9]+(/[0-9]+)?$"'),
 ("valuetext.jl", '_peek(c) === :end || _bad(str, "text after the value")', 'nothing'),
 ("strip.jl", "[numerator(N), denominator(N), denominator(c)]", "[numerator(N), denominator(N)]"),
 ("notebook.jl", 'lit !== nothing && val isa ADELIC && (PROVENANCE[m][val] = lit)', 'nothing'),
 ("driver.jl", 's = s == "+1" ? "1" : s', 's = s'),
]
killed = 0
for i, (f, a, b) in enumerate(M, 1):
    if ONLY and i not in ONLY:
        continue
    p = S / f; orig = p.read_text()
    assert orig.count(a) == 1, (f, a)
    try:
        p.write_text(orig.replace(a, b))
        r = subprocess.run(["sh", "ux/notebook/test.sh"], capture_output=True, text=True, timeout=300)
        dead = r.returncode != 0
    finally:
        p.write_text(orig)
    killed += dead
    print(f"mutant {i:2d} {f}: {'killed' if dead else 'SURVIVED'}: {b[:60]!r}", flush=True)
print(f"{killed} of {len(M)} killed")
