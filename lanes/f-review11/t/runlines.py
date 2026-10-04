import subprocess, sys
exe, fn = sys.argv[1], sys.argv[2]
for line in open(fn).read().split("\n"):
    if not line.strip(): continue
    r = subprocess.run([exe], input=line + "\n", capture_output=True, text=True, timeout=20)
    print("%-72s -> %s%s" % (line[:72], r.stdout.strip().replace("\n"," / ")[:60], (" STDERR:"+r.stderr.strip()[:300]) if r.stderr.strip() else "") + ("  [rc=%d]"%r.returncode if r.returncode else ""))
