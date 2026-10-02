"""f-repair3: every mutant that tools/mutate/mutate.py generates on the changed lines of the paired loop
(src/lfunc.c parity_centre, the loop and the odd final step), run against test_lfunc_trig and the
comparison program. Each mutant: one token change applied to a scratch copy of src/lfunc.c, compiled alone
(no sanitizer), put into a copy of the lane's archive in place of lfunc.o. Usage (from the worktree root):
    python3 lanes/f-repair3/loop_mutants.py <scratch dir> <first line> <last line>
"""
import os, shutil, subprocess, sys
sys.path.insert(0, 'tools/mutate')
import mutate

scratch, lo, hi = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
B = 'lanes/f-repair3/build'
text = open('src/lfunc.c').read()
ms = [m for m in mutate.mutants_of('src/lfunc.c', text) if lo <= m.line <= hi]
os.makedirs(scratch, exist_ok=True)
cc = ['cc', '-std=c11', '-O2', '-g', '-Iinclude', '-Isrc']
def run(cmd, t):
    try:
        return subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=t).returncode
    except subprocess.TimeoutExpired:
        return 'timeout'
tally = {}
for i, m in enumerate(ms):
    src = os.path.join(scratch, 'lfunc.c')
    open(src, 'w').write(text[:m.start] + m.replacement + text[m.end:])
    lib = os.path.join(scratch, 'lib.a')
    shutil.copy(B + '/libadelefeld.a', lib)
    obj = os.path.join(scratch, 'lfunc.o')
    if run(cc + ['-c', src, '-o', obj], 60) != 0:
        verdict = 'not compiled'
    else:
        run(['ar', 'r', lib, obj], 30)
        t = os.path.join(scratch, 't'); c = os.path.join(scratch, 'c')
        run(cc + ['-Itests', 'tests/test_lfunc_trig.c'] + [os.path.join(B, 'support', f) for f in
            sorted(os.listdir(B + '/support')) if f.endswith('.o')] + [lib, '-lflint', '-lgmp', '-lm', '-o', t], 60)
        run(cc + ['lanes/f-repair3/compare.c', B + '/ref_lfunc.o', lib, '-lflint', '-lgmp', '-lm', '-o', c], 60)
        rt = run([t], 120)
        rc = run([c], 120)
        verdict = 'killed' if rt != 0 else ('killed by compare only' if rc != 0 else 'survived')
        verdict += ' (test %s, compare %s)' % (rt, rc)
    tally[verdict.split(' (')[0]] = tally.get(verdict.split(' (')[0], 0) + 1
    print('%s  %s' % (str(m), verdict), flush=True)
print(tally)
