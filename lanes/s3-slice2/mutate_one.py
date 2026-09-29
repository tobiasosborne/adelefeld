import sys

# usage: mutate_one.py <file> <from1> <to1> [<from2> <to2> ...]; every pattern must occur exactly once
path = sys.argv[1]
pairs = sys.argv[2:]
s = open(path).read()
for i in range(0, len(pairs), 2):
    a, b = pairs[i], pairs[i + 1]
    if s.count(a) != 1:
        sys.exit(1)
    s = s.replace(a, b)
open(path, "w").write(s)
