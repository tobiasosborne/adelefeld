"""Unit tests of the Ledger slice: sealer, importer, renderer, oracle.

Run from the repository root:  timeout 300 python3 -m unittest discover -s ux/ledger -v
The sealer tests run the real driver build/adf (built by `make -j2 && make -C tools/adf`).
"""
import contextlib
import io
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import import_fixtures  # noqa: E402
import ledger  # noqa: E402
import oracle  # noqa: E402
import render  # noqa: E402
import seal  # noqa: E402

ADF = os.path.join(ROOT, 'build', 'adf')


def claim(cid, script, expect, **kw):
    c = {'id': cid, 'statement': 'Test claim %s.' % cid, 'script': script, 'expect': expect,
         'places': [], 'tail': 'nothing said', 'argument': '', 'proposer': 'test', 'state': 'proposed'}
    c.update(kw)
    return c


class LedgerDir:
    """A temporary ledger directory with claims/ and seals/."""

    def __init__(self, claims):
        self.dir = tempfile.mkdtemp(prefix='ledger-test-')
        os.makedirs(os.path.join(self.dir, 'claims'))
        for c in claims:
            self.write_claim(c)

    def write_claim(self, c):
        with open(os.path.join(self.dir, 'claims', c['id'] + '.json'), 'w') as f:
            json.dump(c, f, indent=1)
            f.write('\n')

    def seal_path(self, cid):
        return os.path.join(self.dir, 'seals', cid + '.json')

    def read_seal(self, cid):
        with open(self.seal_path(cid)) as f:
            return json.load(f)

    def seal(self, *extra):
        with contextlib.redirect_stdout(io.StringIO()):
            return seal.main(['--ledger', self.dir, '--adf', ADF] + list(extra))

    def close(self):
        shutil.rmtree(self.dir)


@unittest.skipUnless(os.access(ADF, os.X_OK), 'build/adf is not built')
class TestSealer(unittest.TestCase):

    def setUp(self):
        self.L = LedgerDir([
            claim('t-sealed', ['prec 4', 'cos_at 4 with 2', '# a comment', 'show 7/3'],
                  ['2: 9 + O(2^4)', '7/3'], expect_exit=0),
            claim('t-refuted', ['prec 4', 'cos_at 4 with 2'], ['2: 1 + O(2^4)']),
            claim('t-open', ['prec 8', 'roots_at (* ; 1 mod 4) with 2 with 2'],
                  ['2 [+1]: 1 + O(2^2); 2 [-1]: 3 + O(2^2)'], branch='all'),
            claim('t-proposed', [], []),
        ])

    def tearDown(self):
        self.L.close()

    def test_verdicts(self):
        status = self.L.seal()
        self.assertEqual(status, 1)  # not every claim is sealed
        s = self.L.read_seal('t-sealed')
        self.assertEqual(s['verdict'], 'sealed')
        self.assertEqual(s['actual'], ['2: 9 + O(2^4)', '7/3'])
        self.assertEqual(s['script_line'], [2, 4])
        self.assertEqual(s['exit'], 0)
        self.assertEqual(self.L.read_seal('t-refuted')['verdict'], 'refuted')
        o = self.L.read_seal('t-open')
        self.assertEqual(o['verdict'], 'open')
        self.assertEqual(o['actual'], ['error: NOT_DETERMINED'])
        self.assertFalse(os.path.exists(self.L.seal_path('t-proposed')))

    def test_build_identity(self):
        self.L.seal('--only', 't-sealed')
        b = self.L.read_seal('t-sealed')['build']
        self.assertRegex(b['adf_sha256'], r'^[0-9a-f]{64}$')
        self.assertRegex(b['commit'], r'^[0-9a-f]{40}$')
        self.assertRegex(b['flint_version'], r'^\d+\.\d+\.\d+$')
        self.assertIn('libflint', b['flint_library'])
        s = self.L.read_seal('t-sealed')
        with open(os.path.join(self.L.dir, 'claims', 't-sealed.json'), 'rb') as f:
            self.assertEqual(s['claim_sha256'], ledger.sha256_bytes(f.read()))

    def test_only_and_exit_zero(self):
        self.assertEqual(self.L.seal('--only', 't-sealed'), 0)
        self.assertFalse(os.path.exists(self.L.seal_path('t-refuted')))

    def test_reseal_same_result_keeps_file(self):
        self.L.seal('--only', 't-sealed')
        with open(self.L.seal_path('t-sealed'), 'rb') as f:
            first = f.read()
        self.L.seal('--only', 't-sealed')
        with open(self.L.seal_path('t-sealed'), 'rb') as f:
            self.assertEqual(f.read(), first)
        self.assertEqual(os.listdir(os.path.join(self.L.dir, 'seals', 'history')), [])

    def test_reopen_on_other_build(self):
        self.L.seal('--only', 't-sealed')
        s = self.L.read_seal('t-sealed')
        # an earlier seal by another build that gave another line
        s['build']['adf_sha256'] = 'f' * 64
        s['actual'] = ['2: 1 + O(2^4)', '7/3']
        s['date'] = '2026-01-01T00:00:00Z'
        with open(self.L.seal_path('t-sealed'), 'w') as f:
            json.dump(s, f)
        self.L.seal('--only', 't-sealed')
        new = self.L.read_seal('t-sealed')
        self.assertEqual(new['verdict'], 'open')
        self.assertEqual(new['run_verdict'], 'sealed')
        self.assertEqual(new['reopened']['earlier']['adf_sha256'], 'f' * 64)
        self.assertEqual(new['reopened']['later']['adf_sha256'], new['build']['adf_sha256'])
        hist = os.listdir(os.path.join(self.L.dir, 'seals', 'history'))
        self.assertEqual(len(hist), 1)
        self.assertTrue(hist[0].startswith('t-sealed.'))
        page = render.render(self.L.dir)
        self.assertIn('REOPENED.', page)
        self.assertEqual(page.count('Failing place of a status line: <b>not given</b>'), 0)
        self.assertIn('earlier <code>2: 1 + O(2^4)</code>', page)
        # a second run of the same build keeps the claim open
        self.L.seal('--only', 't-sealed')
        self.assertEqual(self.L.read_seal('t-sealed')['verdict'], 'open')

    def test_other_build_same_lines_keeps_seal(self):
        self.L.seal('--only', 't-sealed')
        s = self.L.read_seal('t-sealed')
        s['build']['adf_sha256'] = 'e' * 64
        with open(self.L.seal_path('t-sealed'), 'w') as f:
            json.dump(s, f)
        with open(self.L.seal_path('t-sealed'), 'rb') as f:
            before = f.read()
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            seal.main(['--ledger', self.L.dir, '--adf', ADF, '--only', 't-sealed'])
        self.assertIn('confirmed by build', buf.getvalue())
        with open(self.L.seal_path('t-sealed'), 'rb') as f:
            self.assertEqual(f.read(), before)
        self.assertEqual(os.listdir(os.path.join(self.L.dir, 'seals', 'history')), [])

    def test_failing_setting_prints_a_line(self):
        self.L.write_claim(claim('t-setting', ['prec 0', 'digits 5', 'show 1/2'], ['error: DOMAIN', '1/2']))
        self.L.seal('--only', 't-setting')
        s = self.L.read_seal('t-setting')
        self.assertEqual(s['verdict'], 'sealed')
        self.assertEqual(s['script_line'], [1, 3])

    def test_changed_claim_is_resealed_not_reopened(self):
        self.L.seal('--only', 't-refuted')
        c = claim('t-refuted', ['prec 4', 'cos_at 4 with 2'], ['2: 9 + O(2^4)'])
        self.L.write_claim(c)
        # before the reseal, the page shows the claim as proposed: its seal is of an earlier text
        state, note = ledger.shown_state(c, self.L.read_seal('t-refuted'))
        self.assertEqual(state, 'proposed')
        self.assertIn('changed after its seal', note)
        self.assertEqual(ledger.shown_state(dict(c, state='ratified'), self.L.read_seal('t-refuted'))[0],
                         'proposed')
        self.L.seal('--only', 't-refuted')
        s = self.L.read_seal('t-refuted')
        self.assertEqual(s['verdict'], 'sealed')
        self.assertIsNone(s['reopened'])
        self.assertEqual(len(os.listdir(os.path.join(self.L.dir, 'seals', 'history'))), 1)

    def test_dry_run_runs_nothing(self):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            status = seal.main(['--ledger', self.L.dir, '--adf', '/nonexistent/adf', '--dry-run'])
        self.assertEqual(status, 0)
        self.assertIn('     cos_at 4 with 2', buf.getvalue())
        self.assertIn('     2: 9 + O(2^4)', buf.getvalue())
        self.assertFalse(os.path.exists(self.L.seal_path('t-sealed')))


class TestCompare(unittest.TestCase):

    def test_status_classes_cover_status_h(self):
        with open(os.path.join(ROOT, 'include', 'adelefeld', 'status.h')) as f:
            names = re.findall(r'case ADF_\w+: return "(\w+)";', f.read())
        self.assertEqual(len(names), 11)
        self.assertEqual(sorted(names), sorted(ledger.STATUS_CLASS))
        self.assertEqual(ledger.STATUS_CLASS['OK'], 'ok')
        self.assertEqual(ledger.STATUS_CLASS['LIMIT'], 'budget')

    def test_compare_rules(self):
        self.assertEqual(ledger.compare(['1'], ['1'], None, 0)[0], 'sealed')
        self.assertEqual(ledger.compare(['1'], ['2'], None, 0)[0], 'refuted')
        self.assertEqual(ledger.compare(['1'], ['error: NOT_UNIQUE'], None, 1)[0], 'open')
        self.assertEqual(ledger.compare(['1'], ['error: LIMIT'], None, 1)[0], 'open')
        self.assertEqual(ledger.compare(['1'], ['error: DOMAIN'], None, 1)[0], 'refuted')
        self.assertEqual(ledger.compare(['1'], ['error: PARSE'], None, 1)[0], 'refuted')
        self.assertEqual(ledger.compare(['1', '2'], ['1'], None, 0)[0], 'refuted')
        self.assertEqual(ledger.compare(['1'], ['1'], 1, 0)[0], 'refuted')
        # one contradicting line outweighs an open one
        self.assertEqual(ledger.compare(['1', '2'], ['error: NOT_DETERMINED', '3'], None, 1)[0], 'refuted')


class TestImporter(unittest.TestCase):

    def test_two_fixtures(self):
        d = os.path.join(ROOT, 'tests', 'driver')
        c = import_fixtures.claim_from_fixture(os.path.join(d, 'i-02-ucoset.cmd'))
        self.assertEqual(c['id'], 'fx-i-02-ucoset')
        self.assertEqual(c['state'], 'proposed')
        with open(os.path.join(d, 'i-02-ucoset.out')) as f:
            self.assertEqual(c['expect'], f.read().split('\n')[:-1])
        with open(os.path.join(d, 'i-02-ucoset.cmd')) as f:
            text = f.read()
        self.assertEqual(c['script'], text.split('\n')[:-1])
        first = [ln for ln in text.split('\n') if ln.startswith('#') and not ln.startswith('#!')][0]
        self.assertEqual(c['statement'], first.lstrip('#').strip())
        m = re.search(r'^#!exit (\d+)$', text, re.M)
        self.assertEqual(c['expect_exit'], int(m.group(1)) if m else 0)
        self.assertTrue(all(p == 'R' or isinstance(p, int) or p.startswith('cofactor ') for p in c['places']))
        self.assertEqual(c['places'][0], 'R')  # the real coordinates of its idele literals (U6)
        self.assertIn(2, c['places'])  # [5 mod 36] occurs in the file
        c2 = import_fixtures.claim_from_fixture(os.path.join(d, 's-roots.cmd'))
        self.assertEqual(c2['id'], 'fx-s-roots')
        self.assertEqual(c2['tail'], 'nothing said')
        self.assertEqual(c2['source'], 'tests/driver/s-roots.cmd')

    def test_footprint_rule(self):
        text = '# comment mod 97 is not an operand\nshow [p=1009: 3]\nshow 1 mod 2002\nshow 1 mod 0\n'
        self.assertEqual(import_fixtures.footprint(text), [2, 7, 11, 13, 'cofactor 1009'])
        self.assertEqual(import_fixtures.footprint('show 5 mod 3000017\n'), ['cofactor 3000017'])


class TestRenderer(unittest.TestCase):

    def test_page_holds_every_id(self):
        L = LedgerDir([claim('r-one', ['show 1'], ['1']), claim('r-two', [], []),
                       claim('r-three', ['show 2'], ['2'], state='ratified')])
        try:
            if os.access(ADF, os.X_OK):
                L.seal()
            page = render.render(L.dir)
            for cid in ('r-one', 'r-two', 'r-three'):
                self.assertIn('id="claim-%s"' % cid, page)
            self.assertEqual(page, render.render(L.dir))  # deterministic
            if os.access(ADF, os.X_OK):
                self.assertIn('data-state="ratified"', page)
        finally:
            L.close()

    def test_digit_difference(self):
        self.assertEqual(render.digit_difference('2: 1 + O(2^4)', '2: 9 + O(2^4)'),
                         ('0001', '1001', 3, 2))


class TestOracle(unittest.TestCase):

    def test_exp_matches_driver_readme(self):
        # tools/adf/README.md:269: `5: 349831 + O(5^8)` for `exp_at 5 with 5` at `prec 8`
        self.assertEqual(oracle.exp_mod(5, 5, 8), 349831)


README = os.path.join(HERE, 'README.md')
FRESH_ENV = ['env', '-i', 'PATH=/usr/local/bin:/usr/bin:/bin', 'HOME=' + os.environ.get('HOME', '/tmp'),
             'bash', '--noprofile', '--norc', '-c']


def readme_command_lines():
    """The command lines of the README: every line of a fenced sh block that is not a comment."""
    with open(README) as f:
        text = f.read()
    lines = []
    for block in re.findall(r'^```sh\n(.*?)^```', text, re.M | re.S):
        lines.extend(ln.strip() for ln in block.split('\n') if ln.strip() and not ln.strip().startswith('#'))
    return text, lines


def mirror_root():
    """A temporary repository root: a copy of ux/ledger, links to tests/, build/, include/, refs/, docs/."""
    d = tempfile.mkdtemp(prefix='ledger-readme-')
    shutil.copytree(HERE, os.path.join(d, 'ux', 'ledger'), ignore=shutil.ignore_patterns('__pycache__'))
    for name in ('tests', 'build', 'include', 'refs', 'docs'):
        os.symlink(os.path.join(ROOT, name), os.path.join(d, name))
    return d


def tree_bytes(d):
    out = {}
    for dirpath, dirnames, filenames in os.walk(d):
        dirnames[:] = [x for x in dirnames if x != '__pycache__']
        for name in filenames:
            p = os.path.join(dirpath, name)
            with open(p, 'rb') as f:
                out[os.path.relpath(p, d)] = f.read()
    return out


class TestReadme(unittest.TestCase):
    """D1 of the drive (lanes/ux-drive-ledger/report.md): every README command line is pasteable."""

    def test_commands_are_fenced(self):
        text, lines = readme_command_lines()
        self.assertGreaterEqual(len(lines), 8)
        # no command is left in an indented block, where an explanation can hide on the same line
        self.assertEqual([ln for ln in text.split('\n') if ln.startswith('    ') and ln.strip()], [])
        self.assertLess(len(text.splitlines()), 80)  # D-UX6: under 80 lines
        for ln in lines:
            r = subprocess.run(['bash', '-n', '-c', ln], capture_output=True, text=True, timeout=30)
            self.assertEqual(r.returncode, 0, '%s: %s' % (ln, r.stderr))

    @unittest.skipUnless(os.access(ADF, os.X_OK), 'build/adf is not built')
    def test_commands_run_literally_in_a_fresh_shell(self):
        _, lines = readme_command_lines()
        kinds = {'build': 0, 'ledger': 0, 'shown': 0}
        d = mirror_root()
        try:
            before = tree_bytes(os.path.join(d, 'ux', 'ledger'))
            for ln in lines:
                if ln.startswith('timeout 600 make'):
                    r = subprocess.run(FRESH_ENV + [ln], cwd=ROOT, capture_output=True, text=True, timeout=600)
                    self.assertEqual(r.returncode, 0, '%s: %s' % (ln, r.stdout[-400:] + r.stderr[-400:]))
                    kinds['build'] += 1
                elif ln.startswith('python3 ux/ledger/') or ln.startswith('timeout 300 python3 ux/ledger/'):
                    r = subprocess.run(FRESH_ENV + [ln], cwd=d, capture_output=True, text=True, timeout=300)
                    # seal.py exits 1 on this ledger (claims refuted on purpose); 2 is a usage fault
                    self.assertIn(r.returncode, (0, 1), '%s: %s' % (ln, r.stderr[-400:]))
                    self.assertNotIn('usage:', r.stderr, ln)
                    kinds['ledger'] += 1
                elif ln.startswith('firefox ') or ' -m unittest ' in ln:
                    kinds['shown'] += 1  # checked by bash -n only: a browser, and this test itself
                else:
                    self.fail('a README command line of no known kind: %s' % ln)
            self.assertEqual(kinds['build'], 1)
            self.assertGreaterEqual(kinds['ledger'], 5)
            # import, seal and render reproduce every file of the ledger byte for byte
            after = tree_bytes(os.path.join(d, 'ux', 'ledger'))
            self.assertEqual(sorted(after), sorted(before))
            self.assertEqual([k for k in before if before[k] != after[k]], [])
        finally:
            shutil.rmtree(d)


PROBE = os.path.join(HERE, 'probes', 'flint-equality.c')


class TestClaimA6(unittest.TestCase):
    """D4 of the drive: FLINT's padic_equal returns 0 for exp(3) and exp(12) at precision 8."""

    def setUp(self):
        with open(os.path.join(HERE, 'claims', 'a6-flint-equal-ignores-precision.json')) as f:
            self.c = json.load(f)

    def test_statement_says_what_is_true(self):
        st = self.c['statement']
        self.assertNotIn('are equal', st)
        for s in ('padic_equal', 'not equal', '958', '5125', '4167 = 9 * 463', 'modulo 9', 'not modulo 27'):
            self.assertIn(s, st)
        self.assertEqual(self.c['script'], [])
        # the numbers of the statement: the oracle's centres, their difference, its 3-adic valuation
        self.assertEqual((oracle.exp_mod(3, 3, 8), oracle.exp_mod(12, 3, 8)), (958, 5125))
        self.assertEqual(5125 - 958, 4167)
        self.assertEqual(4167, 9 * 463)
        self.assertNotEqual(463 % 3, 0)
        self.assertIn('ux/ledger/probes/flint-equality.c', self.c['argument'])
        self.assertIn('refs/src/flint-3.0.1/padic.rst:263-266', self.c['argument'])

    @unittest.skipUnless(shutil.which('cc'), 'no C compiler')
    def test_probe_prints_what_the_argument_quotes(self):
        d = tempfile.mkdtemp(prefix='ledger-probe-')
        try:
            exe = os.path.join(d, 'flint-equality')
            r = subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', PROBE, '-lflint', '-lgmp',
                                '-o', exe], capture_output=True, text=True, timeout=120)
            if r.returncode != 0:
                self.skipTest('the probe does not build here: ' + r.stderr[-200:])
            out = subprocess.run([exe], capture_output=True, text=True, timeout=60).stdout.split('\n')
        finally:
            shutil.rmtree(d)
        quoted = re.findall(r'`([a-z_()0-9]+: [^`]+)`', self.c['argument'])
        self.assertEqual(quoted, ['exp(3): 958', 'exp(12): 5125', 'precision: 8 8', 'padic_equal: 0'])
        for q in quoted:
            self.assertIn(q, out)


PREFIXED = 'continuation S=((1 +/- 0.001) + (0)*i ; 0): error: NOT_DETERMINED'


class TestPrefixedStatus(unittest.TestCase):
    """D3 of the drive: a line `<prefix>: error: STATUS` has the class of STATUS."""

    def test_compare_prefixed(self):
        self.assertEqual(ledger.line_status(PREFIXED), 'NOT_DETERMINED')
        self.assertEqual(ledger.compare(['0'], [PREFIXED], None, 1)[0], 'open')
        self.assertEqual(ledger.compare(['0'], ['continuation S=(1 ; 0): error: LIMIT'], None, 1)[0], 'open')
        self.assertEqual(ledger.compare(['0'], ['continuation S=(1 ; 0): error: DOMAIN'], None, 1)[0], 'refuted')
        self.assertEqual(ledger.compare(['0'], ['continuation S=(1 ; 0): error: PARSE'], None, 1)[0], 'refuted')
        # an expected prefixed DOMAIN agrees with itself; the raw forms keep their classes
        d = 'continuation S=((1) + (0)*i ; 0): error: DOMAIN'
        self.assertEqual(ledger.compare([d], [d], None, 1)[0], 'sealed')
        self.assertEqual(ledger.compare(['0'], ['error: LIMIT'], None, 1)[0], 'open')
        self.assertEqual(ledger.compare(['0'], ['error: PARSE'], None, 1)[0], 'refuted')
        # a value line is not a status line
        self.assertEqual(ledger.line_status('2: 9 + O(2^4)'), 'OK')

    @unittest.skipUnless(os.access(ADF, os.X_OK), 'build/adf is not built')
    def test_drive_prefixed_open(self):
        # lanes/ux-drive-ledger/status-checks.py, the claim drive-prefixed-open
        L = LedgerDir([claim('drive-prefixed-open', [
            'prec 128', 'tate_gamma ((1 +/- 0.001) + (0)*i ; 0) with 3 with ((1) + (0)*i ; 0) '
            'with char(q=1, n=1, s=(0) + (0)*i)'], ['0'], places=[3])])
        try:
            self.assertEqual(L.seal(), 1)
            s = L.read_seal('drive-prefixed-open')
            self.assertEqual(s['actual'], [PREFIXED])
            self.assertEqual(s['verdict'], 'open')
            page = render.render(L.dir)
            c = re.search(r'<article[^>]*id="claim-drive-prefixed-open"[\s\S]*?</article>', page).group(0)
            self.assertIn('class="cls cls-open"', c)
            self.assertNotIn('>REFUTED<', c)
            self.assertIn('OPEN &middot; NOT_DETERMINED', c)
        finally:
            L.close()

    def test_tate_fixture_cards_show_every_status_class(self):
        page = render.render(HERE)
        for cid in ('fx-tate-gamma', 'fx-tate-fe'):
            c = re.search(r'<article[^>]*id="claim-%s"[\s\S]*?</article>' % cid, page).group(0)
            with open(os.path.join(HERE, 'seals', cid + '.json')) as f:
                actual = json.load(f)['actual']
            n_status = sum(1 for a in actual if re.search(r'(^|: )error: [A-Z_]+$', a))
            self.assertGreater(n_status, 0)
            self.assertEqual(len(re.findall(r'<span class="cls cls-', c)), n_status, cid)


def card_of(page, cid):
    return re.search(r'<article[^>]*id="claim-%s"[\s\S]*?</article>' % re.escape(cid), page).group(0)


class TestFootprint(unittest.TestCase):
    """D2 and U6 of the drive: a filled box is a place at which a driver line ran; named places are outlined."""

    def test_operand_places(self):
        fp = import_fixtures.footprint
        self.assertEqual(fp('exp_at 4 with 2\n'), [2])
        self.assertEqual(fp('roots_at -1 with 5 with 2\n'), [5])  # the degree 2 is not a place
        self.assertEqual(fp('root_at 9 with 5 with 2 with 1\n'), [5])
        self.assertEqual(fp('hilbert_at 3 with 3 with 2\n'), [2])
        self.assertEqual(fp('Log_at (-1 ; 4 * [1 mod 9]) with real\n'), ['R'])
        self.assertEqual(fp('project 4 with 2 3 real\n'), ['R', 2, 3])
        self.assertEqual(fp('abs (1 ; 6/35 * [1]) with 7\n'), [7])  # computed at 7 only
        self.assertEqual(fp('roots_at (* ; 1 mod 4) with 2 with 2\n'), [2])
        self.assertEqual(fp('norm (0.17 +/- 1e-27 ; 6/35 * [1])\n'), ['R'])
        self.assertEqual(fp('add (* ; 1/2 mod 1) with (* ; 1/2 mod 1)\n'), [])
        self.assertEqual(fp('exp_at 4 with 1009\n'), ['cofactor 1009'])
        self.assertEqual(fp('roots -2 0 1 with 8 with 5\n'), [])  # 8 is not a prime: no place

    def test_imported_footprints(self):
        d = os.path.join(ROOT, 'tests', 'driver')
        names = [n for n in sorted(os.listdir(d))
                 if n.endswith('.cmd') and os.path.exists(os.path.join(d, n[:-4] + '.out'))]
        claims = [import_fixtures.claim_from_fixture(os.path.join(d, n)) for n in names]
        self.assertEqual(len(claims), 86)
        real = [c for c in claims if any('with real' in x for x in c['script'] if not x.lstrip().startswith('#'))]
        self.assertEqual(len(real), 19)  # the drive's 20 counts i-01-show-type: a comment
        self.assertEqual([c['id'] for c in real if 'R' not in c['places']], [])
        empty = [c['id'] for c in claims if not c['places']]
        self.assertLess(len(empty), 29)  # 29 before this repair (the drive, U6)

    @unittest.skipUnless(os.access(ADF, os.X_OK), 'build/adf is not built')
    def test_drive_footprint(self):
        # lanes/ux-drive-ledger/extras.py: b6 with 101 added to places and no driver line at 101
        with open(os.path.join(HERE, 'claims', 'b6-two-square-family.json')) as f:
            b6 = json.load(f)
        c = dict(b6, id='drive-footprint', places=b6['places'] + [101])
        L = LedgerDir([c])
        try:
            L.seal()
            self.assertEqual(L.read_seal('drive-footprint')['verdict'], 'sealed')
            card = card_of(render.render(L.dir), 'drive-footprint')
            m = re.search(r'<span class="c wide" title="(\d+) primes: ([0-9 ]+)">11 ... (\d+)</span>', card)
            self.assertIsNotNone(m)
            self.assertEqual((m.group(1), m.group(3)), ('21', '97'))
            self.assertNotIn('101', m.group(2).split())
            self.assertIn('<span class="n">101</span>', card)
            for p in (3, 5, 7):
                self.assertIn('<span class="c">%d</span>' % p, card)
            self.assertIn('<span>2</span>', card)  # 2 is neither named nor computed
        finally:
            L.close()


@unittest.skipUnless(os.access(ADF, os.X_OK), 'build/adf is not built')
class TestBranch(unittest.TestCase):
    """U3 of the drive: a claim whose script calls a root command names its branch, by seed or `all`."""

    SEEDLESS = claim('drive-branch', ['prec 1', 'roots_at -1 with 5 with 2'],
                     ['5 [2]: 2 + O(5^1); 5 [3]: 3 + O(5^1)'], places=[5],
                     statement='The square root of -1 at 5 is 2 mod 5.')

    def run_one(self, c):
        L = LedgerDir([c])
        try:
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                status = seal.main(['--ledger', L.dir, '--adf', ADF])
            sealed = os.path.exists(L.seal_path(c['id']))
            page = render.render(L.dir)
            return status, buf.getvalue(), sealed, card_of(page, c['id'])
        finally:
            L.close()

    def test_seedless_is_refused(self):
        status, out, sealed, card = self.run_one(self.SEEDLESS)
        self.assertEqual(status, 1)
        self.assertFalse(sealed)
        self.assertRegex(out, r'drive-branch +proposed +refused: the script calls roots_at')
        self.assertIn('PROPOSED', card)
        self.assertIn('names no branch', card)

    def test_all_branches_and_seeds_seal(self):
        status, out, sealed, card = self.run_one(dict(
            self.SEEDLESS, branch='all', statement='The square roots of -1 at 5 are 2 and 3 mod 5 (all branches).'))
        self.assertEqual((status, sealed), (0, True), out)
        for seed, centre in (('2', '2'), ('3', '3')):
            c = claim('drive-seed-' + seed, ['prec 1', 'root_at -1 with 5 with 2 with ' + seed],
                      ['5 [%s]: %s + O(5^1)' % (seed, centre)], places=[5], branch=seed)
            status, out, sealed, _ = self.run_one(c)
            self.assertEqual((status, sealed), (0, True), out)

    def test_seed_must_match_the_script(self):
        c = claim('drive-seed-mismatch', ['prec 1', 'root_at -1 with 5 with 2 with 3'], ['5 [3]: 3 + O(5^1)'],
                  places=[5], branch='2')
        status, out, sealed, _ = self.run_one(c)
        self.assertEqual((status, sealed), (1, False))
        self.assertIn('the branch 2 is not the seed 3 of script line 2', out)
        c = dict(self.SEEDLESS, id='drive-seed-roots-at', branch='2')
        status, out, sealed, _ = self.run_one(c)
        self.assertEqual((status, sealed), (1, False))
        self.assertIn('roots_at prints every branch', out)

    def test_ledger_claims_with_root_commands_name_a_branch(self):
        need = sorted(c['id'] for c, _ in ledger.load_claims(HERE) if ledger.root_lines(c['script']))
        self.assertEqual(need, ['b5-1-sqrt-1-plus-8z2', 'b5-2-sqrt-5-plus-8z2', 'b5-sqrt-1-plus-4z2',
                                'fx-gfunc-root', 'fx-pow-status', 'fx-pow-values', 'fx-root-status',
                                'fx-root-values'])
        for c, _ in ledger.load_claims(HERE):
            self.assertEqual(ledger.branch_fault(c), '', c['id'])


@unittest.skipUnless(os.access(ADF, os.X_OK), 'build/adf is not built')
class TestRatification(unittest.TestCase):
    """U2 of the drive: an argued tail is ratified only with a source cited by file and line."""

    def run_one(self, c):
        L = LedgerDir([c])
        try:
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                seal.main(['--ledger', L.dir, '--adf', ADF])
            return buf.getvalue(), card_of(render.render(L.dir), c['id'])
        finally:
            L.close()

    def b6(self, **kw):
        with open(os.path.join(HERE, 'claims', 'b6-two-square-family.json')) as f:
            return dict(json.load(f), state='ratified', **kw)

    def test_drive_citation_is_refused(self):
        # lanes/ux-drive-ledger/extras.py: b6 without its citations, set to ratified
        out, card = self.run_one(self.b6(id='drive-citation', argument='No citation supplied.'))
        self.assertRegex(out, r'drive-citation +sealed ')
        self.assertNotIn('>RATIFIED<', card)
        self.assertIn('SEALED', card)
        self.assertIn('ratification refused: no source', card)
        self.assertIn('the ratification is void', card)

    def test_citation_of_a_missing_file_is_refused(self):
        out, card = self.run_one(self.b6(id='drive-citation-2', argument='See refs/src/nowhere/none.txt:1-2.'))
        self.assertIn('ratification refused: no source', card)
        out, card = self.run_one(self.b6(id='drive-citation-3',
                                         argument='See refs/src/conrad-hensel/hensel.txt:900000.'))
        self.assertIn('ratification refused: no source', card)

    def test_cited_source_ratifies(self):
        out, card = self.run_one(self.b6(id='drive-citation-ok'))
        self.assertRegex(out, r'drive-citation-ok +ratified ')
        self.assertIn('RATIFIED', card)
        self.assertNotIn('ratification refused', card)
        # a tail that says nothing about other places needs no source
        out, card = self.run_one(self.b6(id='drive-citation-local', tail='nothing said', argument=''))
        self.assertRegex(out, r'drive-citation-local +ratified ')


if __name__ == '__main__':
    unittest.main()
