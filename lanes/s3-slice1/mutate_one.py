import sys

path, a, b = sys.argv[1], sys.argv[2], sys.argv[3]
s = open(path).read()
if s.count(a) != 1:
    sys.exit(1)
open(path, "w").write(s.replace(a, b))
