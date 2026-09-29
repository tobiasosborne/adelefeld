"""run every line of a file through ./kern separately; print line, rc, stdout, stderr tail"""
import subprocess, sys, os
HERE = os.path.dirname(os.path.abspath(__file__))
for ln in open(sys.argv[1]):
    ln = ln.strip()
    if not ln:
        continue
    try:
        o = subprocess.run([os.path.join(HERE, "kern")], input=ln + "\n", capture_output=True, text=True, timeout=60)
        print(ln, "->", "rc=%d" % o.returncode, o.stdout.strip()[:200], o.stderr.strip()[-200:])
    except subprocess.TimeoutExpired:
        print(ln, "-> TIMEOUT 60s")
