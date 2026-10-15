# lanes/ux-notebook/wrap.py FILE...: break lines over 116 characters at a ", " (code) or "; " (CSS) outside
# string literals, continuing with the indentation of the line plus four spaces.
import sys
LIM = 116
def split_points(line, css):
    pts, q, esc = [], None, False
    for i, c in enumerate(line):
        if esc: esc = False; continue
        if c == '\\': esc = True; continue
        if q:
            if c == q: q = None
            continue
        if c in '"\'' and not css: q = c; continue
        if line[i:i+2] == (('; ') if css else (', ')): pts.append(i + 1)
    return pts
for f in sys.argv[1:]:
    out, incss = [], False
    for line in open(f).read().split('\n'):
        if 'raw"""' in line: incss = True
        css = incss and line.strip().endswith(('}', ';')) and '{' in line or (incss and line.startswith('  --'))
        while len(line) > LIM:
            pts = [p for p in split_points(line, css) if p <= LIM - 1]
            if not pts: break
            p = pts[-1]
            ind = len(line) - len(line.lstrip())
            out.append(line[:p].rstrip())
            line = ' ' * (ind + (2 if css else 4)) + line[p:].lstrip()
        out.append(line)
        if line.strip() == '"""': incss = False
    open(f, 'w').write('\n'.join(out))
