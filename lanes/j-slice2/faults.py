#!/usr/bin/env python3
"""lanes/j-slice2/faults.py: plant one fault at a time in a SCRATCH COPY of julia/Adelefeld (under
lanes/j-slice2/scratch), run the gate (lanes/j-slice2/gate.sh with PKGDIR set to the copy and ADELEFELD_LIB to
build/libadelefeld.so), record the test summary in lanes/j-slice2/faults.log, delete the copy. The package
sources themselves are never edited. Run from the repository root: python3 lanes/j-slice2/faults.py [ids...]"""
import os, re, shutil, subprocess, sys

F = 'src/fball.jl'
L = 'src/load.jl'
FAULTS = [
    ('F1 inside: argument order reversed', F,
     'ccall(p_adf_fball_contains[], Cint, (P, P), _ptr(x), _ptr(y))',
     'ccall(p_adf_fball_contains[], Cint, (P, P), _ptr(y), _ptr(x))'),
    ('F2 mul_rat: radius not scaled (q a + N Zhat)', F,
     '    GC.@preserve z x q ccall(p_adf_fball_mul_rat[], Cvoid, (P, P, P), _ptr(z), px, pq)\n'
     '    return AdfResult{FBall}(OK, z)',
     '    _release!(z)\n    return AdfResult{FBall}(OK, FBall(center(x) * q, radius(x)))'),
    ('F3 sub implemented as add', F, '(:try_sub, :p_adf_fball_sub)', '(:try_sub, :p_adf_fball_add)'),
    ('F4 FBall(c, N): radius sign dropped', F,
     'function try_fball(c::Rat, N::Rat)\n',
     'function try_fball(c::Rat, N::Rat)\n    N = Rat(abs(_rational(N)))\n'),
    ('F5 compare_points cast to a status', F,
     '    to_point_comparison(GC.@preserve', '    to_status(GC.@preserve'),
    ('F6 equal_set by identical', F,
     'ccall(p_adf_fball_equal_set[], Cint', 'ccall(p_adf_fball_identical[], Cint'),
    ('F7 GC.@preserve omitted (ball add/sub/mul)', F,
     '        GC.@preserve z x y ccall($cfun[], Cvoid, (P, P, P), _ptr(z), px, py)',
     '        ccall($cfun[], Cvoid, (P, P, P), _ptr(z), px, py)'),
    ('F8 adf_str_free skipped in the printer', F,
     '            ccall(p_adf_str_free[], Cvoid, (Ptr{UInt8},), s)', '            nothing'),
    ('F9 failing parse/try: fresh output not cleared', F,
     '        _release!(z)\n        return AdfResult{FBall}(to_status',
     '        return AdfResult{FBall}(to_status'),
    ('F10 fball layout row not gated', L,
     '    (suffix = :fball, size = 48, align = 8, source = "docs/api-m1.md:36,45"),\n', ''),
    ('F11 q - x computed as x - q', F, '        return $name(FBall(q), x)', '        return $name(x, FBall(q))'),
    ('F12 div_rat status ignored', F, '    return _result(z, raw, :div)', '    return AdfResult{FBall}(OK, z)'),
    ('F13 == on balls as equal_set', F,
     'Base.:(==)(x::FBall, y::FBall) = throw(ArgumentError(NO_BALL_EQUALITY))',
     'Base.:(==)(x::FBall, y::FBall) = equal_set(x, y)'),
    ('F14 isequal by equal_set (not identity)', F,
     'Base.isequal(x::FBall, y::FBall) = x === y', 'Base.isequal(x::FBall, y::FBall) = equal_set(x, y)'),
]

root = os.getcwd()
scratch = os.path.join(root, 'lanes/j-slice2/scratch')
ids = set(sys.argv[1:])
out = open('lanes/j-slice2/faults.log', 'a')
for name, rel, old, new in FAULTS:
    if ids and name.split()[0] not in ids:
        continue
    if os.path.exists(scratch):
        shutil.rmtree(scratch)
    pkg = os.path.join(scratch, 'Adelefeld')
    shutil.copytree('julia/Adelefeld', pkg, ignore=shutil.ignore_patterns('Manifest.toml'))
    path = os.path.join(pkg, rel)
    src = open(path).read()
    assert src.count(old) == 1, name
    open(path, 'w').write(src.replace(old, new))
    env = dict(os.environ, PKGDIR=pkg, ADELEFELD_LIB=os.path.join(root, 'build/libadelefeld.so'))
    r = subprocess.run(['sh', 'lanes/j-slice2/gate.sh'], capture_output=True, text=True, env=env)
    text = r.stdout + r.stderr
    open('lanes/j-slice2/fault-%s.log' % name.split()[0], 'w').write(text)
    m = re.search(r'Adelefeld step J-a\.2 *\|([ 0-9]+)', text)
    summ = ' '.join(m.group(1).split()) if m else 'no J-a.2 summary'
    if 'Testing Adelefeld tests passed' in text:
        res = 'NOT CAUGHT: all tests passed (J-a.2 row: %s)' % summ
    else:
        res = 'caught: exit %d; J-a.2 row (pass fail error total): %s' % (r.returncode, summ)
    shutil.rmtree(scratch)
    print(name, '->', res, file=out, flush=True)
out.close()
