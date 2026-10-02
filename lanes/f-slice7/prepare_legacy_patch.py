"""Prepare, but do not apply, the narrow changes to files outside lane ownership."""
from pathlib import Path
import difflib

changes = {}
p = 'tests/test_rfunc.c'
s = Path(p).read_text()
s = s.replace('the others UNSUPPORTED (where = v) (changed by lane f-slice6; before: every function UNSUPPORTED)',
              'sin/cos also have their local domains; the others UNSUPPORTED (where = v)')
s = s.replace('/* f-slice6: exp and log exist at a prime now; for the value 2/3 both are DOMAIN at 5 and at 2 (v_5(2/3) = 0,',
              '/* exp, log, sin and cos at 2/3 are DOMAIN at 5 and at 2 (v_5(2/3) = 0,')
s = s.replace('want_p = (k == 0 || k == 1) ? ADF_DOMAIN : ADF_UNSUPPORTED;',
              'want_p = (k == 0 || k == 1 || k == 3 || k == 4) ? ADF_DOMAIN : ADF_UNSUPPORTED;')
changes[p] = s
p = 'tests/julia/sball.jl'
s = Path(p).read_text().replace('@test st == UNSUPPORTED && primeof(where) == 2   # sin at a prime is a later slice',
                               '@test st == DOMAIN && primeof(where) == 2   # v_2(2/3)=1 is outside 4 Z_2')
changes[p] = s
p = 'tests/julia/f_at.jl'
s = Path(p).read_text().replace('''    # sin at a prime: a valid request, not yet implemented: UNSUPPORTED with where = 5
    st, _, where = at(:adf_sball_sin_at, s, p5, 12)
    @test st == UNSUPPORTED && primeof(where) == 5''', '''    # sin(5/3) at 5, absolute N=12. Lemma 5 bounds the tail after degree 81 above 60.
    st, sine, _ = at(:adf_sball_sin_at, s, p5, 12)
    @test st == OK && nplaces(sine) == 1 && arch(sine) == ARCH_NONE
    stc, cs = getlball(sine, p5)
    partial = sum((-1)^div(k-1, 2) * (big(5)//3)^k / factorial(big(k)) for k in 1:2:81)
    @test stc == OK && !isexact(cs) && prec(cs) == 12 && den(cs) == 1
    @test big(5)^val(cs) * num(cs) == modp(partial, 5, 12)''')
changes[p] = s
p = 'tests/driver/f-places-hostile.cmd'
s = Path(p).read_text().replace('# sin and cos are not commands; the name is not one of the table: PARSE\nsin_at 5 with 5',
                               '# tan is not a command; the name is not one of the table: PARSE\ntan_at 5 with 5')
changes[p] = s
patch = []
for name, new in changes.items():
    old = Path(name).read_text()
    assert new != old, name
    patch.extend(difflib.unified_diff(old.splitlines(True), new.splitlines(True),
                                   fromfile='a/'+name, tofile='b/'+name))
    copy = Path('lanes/f-slice7/build/legacy')/name
    copy.parent.mkdir(parents=True, exist_ok=True)
    copy.write_text(new)
Path('lanes/f-slice7/legacy-tests.patch').write_text(''.join(patch))
print('Prepared 4-file patch; no owned-external file was written. Driver .out is unchanged.')
