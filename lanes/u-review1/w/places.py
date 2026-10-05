import random, subprocess, os, sys
import ref as R
root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
exe = os.path.join(root, "build", "adf-san")
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1")
rng = random.Random(3)
primes = [p for p in range(2, 8000) if R.is_prime(p)][:1000]
comp = {}
for p in primes:
    if rng.random() < .5:
        comp[p] = f"p={p}: {rng.randint(1, 50)}/{rng.choice([1, 3, 7]) if p not in (3, 7) else 1}"
    else:
        N = rng.randint(1, 6)
        comp[p] = f"p={p}: {rng.randint(1, p**N - 1) if p**N > 1 else 1} + O({p}^{N})"
    if comp[p].split(": ")[1].split("/")[0].isdigit() and p in (3, 7) and "/" in comp[p]:
        comp[p] = f"p={p}: 5"
S = "{inf: 3 +/- 1; " + "; ".join(comp[p] for p in primes) + "}"
lines = ["type " + S, "show " + S]
perm = primes[:]
rng.shuffle(perm)
lines.append("project " + S + " with " + " ".join(map(str, perm)))                       # all, permuted
lines.append("project " + S + " with real " + " ".join(map(str, perm)))                  # all + real
lines.append("project " + S + " with " + " ".join(map(str, perm + [perm[500]])))         # a repeat
lines.append("project " + S + " with " + " ".join(map(str, perm[:3] + [8009])))          # a prime not in it
lines.append("project " + S + " with " + " ".join(map(str, perm[:1000]) ) + " 7919")     # 1001 places, 1000 + 7919
lines.append("project " + S + " with real real")
lines.append("project " + S + " with " + " ".join(map(str, perm[:5])) + " real " + " ".join(map(str, perm[5:8])))
lines.append("project " + S + " with 4")
lines.append("project " + S + " with 0")
lines.append("project " + S + " with 18446744073709551629")                              # > 2^64, not a prime
lines.append("project " + S + " with 18446744073709551557")                              # 2^64-59 prime, not in the ball
lines.append("prec 2000000")
lines.append("project " + S + " with real")
lines.append("prec 2097153")
lines.append("project " + S + " with real")
p = subprocess.run([exe, "-v"], input="\n".join(lines) + "\n", capture_output=True, text=True, timeout=160, env=env)
out = p.stdout.split("\n")
print("rc", p.returncode, "stderr sanitizer lines:", sum(1 for l in p.stderr.split("\n") if "ERROR" in l or "runtime error" in l))
for l in p.stderr.split("\n")[:20]:
    print(l[:150])
print(out[0], "|", out[1][:80], len(out[1]))
exp_all = "real: 3 +/- 1; " in out[3] or True
for i in range(2, len(out)):
    print(i, out[i][:110], len(out[i]))
# check line 2: components
o = out[2]
want = "; ".join(f"{p}: " for p in primes)
got = [c.split(":")[0] for c in o.split("; ")]
print("project all permuted: components", len(got), "in increasing order:", got == [str(p) for p in primes])
s1 = out[1].strip("{}").replace("p=", "").split("; ")
s3 = out[2].split("; ")
print("components equal between show and project(all, permuted):", s1[1:] == s3, "lengths", len(s1) - 1, len(s3))
s4 = out[3].split("; ")
print("with real:", s4[0], s1[0], s4[1:] == s3)
