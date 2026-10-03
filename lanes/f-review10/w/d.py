import subprocess, sys
exe = sys.argv[1]
prec = "5"
for l in open(sys.argv[2]).read().split("\n"):
    if l.startswith("prec "):
        prec = l.split()[1]
    if not l.strip() or l.startswith("prec"):
        continue
    r = subprocess.run([exe], input="prec %s\n%s\n" % (prec, l), capture_output=True, text=True, timeout=60)
    print("[prec %s] %s\n    -> %s%s rc=%d" % (prec, l, r.stdout.strip(), (" ERR:" + r.stderr.strip()[:300]) if r.stderr.strip() else "", r.returncode))
