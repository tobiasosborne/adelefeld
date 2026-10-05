import subprocess, sys, os
exe, corp, a, b = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
lines = open(corp).read().split("\n")[a:b]
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1", UBSAN_OPTIONS="print_stacktrace=1")
bad = 0
for i, l in enumerate(lines):
    if not l:
        continue
    try:
        p = subprocess.run([os.path.join(os.path.dirname(os.path.abspath(__file__)), exe)], input=(lines[i-1] + "\n" if i else "") + l + "\n", capture_output=True, text=True, timeout=15, env=env)
        out = p.stdout.strip()
        if p.returncode != 0 or "CRASH" in out or "ROUNDTRIP" in out or "ERROR" in p.stderr or "runtime error" in p.stderr:
            bad += 1
            print("LINE", a + i, l[:200], "->", p.returncode, out[:300].replace("\n", " | "), p.stderr[:600].replace("\n", " | "))
    except subprocess.TimeoutExpired:
        bad += 1
        print("TIMEOUT", a + i, l[:200])
print("done", len(lines), "bad", bad)
