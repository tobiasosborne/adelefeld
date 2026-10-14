#!/usr/bin/env python3
"""lanes/j-slice1/faults.py: plant one fault at a time in julia/Adelefeld/src, run the gate
(lanes/j-slice1/gate.sh), record the test summary, restore the file. Run from the repository root."""
import re, subprocess, sys

S = 'julia/Adelefeld/src/'
FAULTS = [
    ('F1 byte count by characters', S + 'rat.jl',
     'pointer(str), ncodeunits(str),', 'pointer(str), length(str),'),
    ('F2 release without the claim (double clear)', S + 'rat.jl',
     '        claimed || return nothing\n', ''),
    ('F3 free the adjusted pointer', S + 'rat.jl', '        Libc.free(x.base)\n', '        Libc.free(x.data)\n'),
    ('F4 ignore the status of div', S + 'rat.jl',
     '    return _result(z, raw, :div)', '    return AdfResult{Rat}(OK, z)'),
    ('F5 return the output on failure', S + 'rat.jl',
     '        return AdfResult{Rat}(to_status(raw, op), nothing)',
     '        return AdfResult{Rat}(to_status(raw, op), z)'),
    ('F6 layout gate checks size only', S + 'load.jl',
     '        align == r.align || loadfail(', '        true || loadfail('),
    ('F7 status gate skips the count', S + 'load.jl', '    (n == 11 && length', '    (true || length'),
    ('F8 negative denominator not normalized', S + 'rat.jl', '    if D < 0\n', '    if false\n'),
    ('F9 closed handle not checked', S + 'rat.jl',
     '    (@atomic x.state) == STATE_LIVE || throw(', '    true || throw('),
    ('F10 status name gate skipped', S + 'load.jl',
     '        got == string(s) || loadfail(', '        true || loadfail('),
]

out = open('lanes/j-slice1/faults.log', 'w')
for name, path, old, new in FAULTS:
    src = open(path).read()
    assert src.count(old) == 1, name
    open(path, 'w').write(src.replace(old, new))
    try:
        r = subprocess.run(['sh', 'lanes/j-slice1/gate.sh'], capture_output=True, text=True)
        text = r.stdout + r.stderr
        m = re.search(r'Some tests did not pass: (\d+) passed, (\d+) failed, (\d+) errored', text)
        if m:
            res = 'caught: %s passed, %s failed, %s errored' % m.groups()
        elif 'tests passed' in text:
            res = 'NOT CAUGHT: all tests passed'
        else:
            tail = [l for l in text.splitlines() if l.strip() and not l.startswith('\x1b')][-3:]
            res = 'caught: exit %d, %s' % (r.returncode, ' / '.join(tail)[:300])
    finally:
        open(path, 'w').write(src)
    print(name, '->', res, file=out, flush=True)
out.close()
