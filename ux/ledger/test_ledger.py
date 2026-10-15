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
                  ['2 [+1]: 1 + O(2^2); 2 [-1]: 3 + O(2^2)']),
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
        self.assertTrue(all(isinstance(p, int) or p.startswith('cofactor ') for p in c['places']))
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


if __name__ == '__main__':
    unittest.main()
