"""Tests of the Bench slice: python3 -m unittest discover -s ux/bench -p 'test_*.py' (from the repository root).

The end-to-end tests run build/adf (built by `make -j2 && make -C tools/adf`); they are skipped without it.
"""
import json
import os
import re
import stat
import sys
import tempfile
import time
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(HERE))

import sweep  # noqa: E402
import render  # noqa: E402

DRIVER = ROOT / 'build' / 'adf'


class SieveTest(unittest.TestCase):
    def test_small_bounds(self):
        self.assertEqual(sweep.primes_below(0), [])
        self.assertEqual(sweep.primes_below(2), [])
        self.assertEqual(sweep.primes_below(3), [2])
        self.assertEqual(sweep.primes_below(30), [2, 3, 5, 7, 11, 13, 17, 19, 23, 29])

    def test_counts(self):
        self.assertEqual(len(sweep.primes_below(100)), 25)
        self.assertEqual(len(sweep.primes_below(1000)), 168)
        self.assertEqual(sweep.primes_below(1000)[-1], 997)

    def test_agrees_with_trial_division(self):
        trial = [n for n in range(2, 2000) if all(n % d for d in range(2, int(n ** 0.5) + 1))]
        self.assertEqual(sweep.primes_below(2000), trial)

    def test_places(self):
        self.assertEqual(sweep.expand_places(['R', 'primes below 12']), ['R', 2, 3, 5, 7, 11])
        self.assertEqual(sweep.expand_places([7, 'R', 2, 7]), ['R', 2, 7])
        with self.assertRaises(ValueError):
            sweep.expand_places([9])
        with self.assertRaises(ValueError):
            sweep.expand_places(['primes under 12'])


class StatusClassTest(unittest.TestCase):
    def test_every_name_of_status_h(self):
        text = (ROOT / 'include' / 'adelefeld' / 'status.h').read_text()
        names = set(re.findall(r'case ADF_[A-Z_]+: return "([A-Z_]+)";', text))
        self.assertEqual(len(names), 11)
        self.assertEqual(set(sweep.STATUS_CLASS), names)
        self.assertEqual(sweep.STATUS_CLASS['OK'], 'ok')
        self.assertEqual(sweep.STATUS_CLASS['LIMIT'], 'budget')
        for n in ('DOMAIN', 'NO_SOLUTION', 'NOT_UNIT'):
            self.assertEqual(sweep.STATUS_CLASS[n], 'no')
        for n in ('NOT_DETERMINED', 'UNIT_NOT_CERTIFIED', 'NEEDS_SPLIT', 'NOT_UNIQUE'):
            self.assertEqual(sweep.STATUS_CLASS[n], 'open')
        for n in ('PARSE', 'UNSUPPORTED'):
            self.assertEqual(sweep.STATUS_CLASS[n], 'plain')


class ParserTest(unittest.TestCase):
    def test_roots_at_line_with_count(self):
        r = sweep.parse_output('roots 2', '7 [3]: 1802916 + O(7^8); 7 [4]: 3961885 + O(7^8)')
        self.assertEqual((r['status'], r['class'], r['count']), ('OK', 'ok', 2))
        r = sweep.parse_output('roots 2', '2 [+1]: -3; 2 [-1]: 3')
        self.assertEqual(r['count'], 2)

    def test_polynomial_roots_line(self):
        r = sweep.parse_output('roots', '23 mod 2^8; 233 mod 2^8')
        self.assertEqual((r['status'], r['count']), ('OK', 2))
        r = sweep.parse_output('roots', 'none')
        self.assertEqual((r['status'], r['class'], r['count']), ('OK', 'ok', 0))

    def test_error_line(self):
        r = sweep.parse_output('roots 2', 'error: DOMAIN')
        self.assertEqual((r['status'], r['class'], r['count']), ('DOMAIN', 'no', None))
        r = sweep.parse_output('exp', 'error: NOT_DETERMINED')
        self.assertEqual((r['status'], r['class']), ('NOT_DETERMINED', 'open'))
        r = sweep.parse_output('roots', 'error: LIMIT')
        self.assertEqual(r['class'], 'budget')
        with self.assertRaises(ValueError):
            sweep.parse_output('roots', 'error: SOMETHING_ELSE')

    def test_real_roots_line(self):
        line = ('-14.866068747318505523 +/- 3.9e-19; -4.1231056256176605498 +/- 6.9e-20; '
                '-3.6055512754639892931 +/- 3.4e-20; 3.6055512754639892931 +/- 3.4e-20; '
                '4.1231056256176605498 +/- 6.9e-20; 14.866068747318505523 +/- 3.9e-19')
        r = sweep.parse_output('roots', line)
        self.assertEqual((r['status'], r['count']), ('OK', 6))
        r = sweep.parse_output('realroots', '-1.4142135623730950488 +/- 1.7e-21; 1.4142135623730950488 +/- 1.7e-21')
        self.assertEqual(r['count'], 2)
        self.assertEqual(sweep.parse_output('realroots', '0')['count'], 1)
        with self.assertRaises(ValueError):
            sweep.parse_output('realroots', '1.5 +/-')

    def test_value_line(self):
        r = sweep.parse_output('exp', '5: 349831 + O(5^8)')
        self.assertEqual((r['status'], r['count'], r['value']), ('OK', None, '349831 + O(5^8)'))
        r = sweep.parse_output('powrat 3/2 3', '5 [3]: 27')
        self.assertEqual(r['value'], '27')


class CommandTest(unittest.TestCase):
    def test_roots_n(self):
        self.assertEqual(sweep.command_for('roots 2', 7, {'value': '2'})[0], 'roots_at 2 with 7 with 2')
        self.assertEqual(sweep.command_for('roots 2', 'R', {'value': '2'})[0], 'realroots -2 0 1')
        self.assertEqual(sweep.command_for('roots 3', 'R', {'value': '-5/4'})[0], 'realroots 5 0 0 4')
        line, reason = sweep.command_for('roots 2', 'R', {'value': '(* ; 9 mod 15625)'})
        self.assertIsNone(line)
        self.assertTrue(reason)

    def test_polynomial(self):
        poly = {'poly': '-2 0 1'}
        self.assertEqual(sweep.command_for('roots', 5, poly, prec=6)[0], 'roots -2 0 1 with 5 with 6')
        self.assertEqual(sweep.command_for('roots', 'R', poly)[0], 'realroots -2 0 1')
        self.assertIsNone(sweep.command_for('realroots', 5, poly)[0])

    def test_value_operations(self):
        x = {'value': '5'}
        self.assertEqual(sweep.command_for('exp', 5, x)[0], 'exp_at 5 with 5')
        self.assertEqual(sweep.command_for('log', 'R', x)[0], 'log_at 5 with real')
        self.assertEqual(sweep.command_for('project', 3, x)[0], 'project 5 with 3')
        self.assertEqual(sweep.command_for('powrat 3/2 3', 5, x)[0], 'powrat_at 5 with 5 with 3/2 with 3')
        self.assertIsNone(sweep.command_for('powrat 3/2 3', 'R', x)[0])
        with self.assertRaises(ValueError):
            sweep.command_for('sqrt', 5, x)


def answers_for(e, rows):
    """Fake outputs for every cell of the rows: `none` at R, `error: DOMAIN` at a prime."""
    return {(r, j): ('none' if r == 'R' else 'error: DOMAIN') for r in rows for j in range(len(e['inputs']))}


def small_experiment(**over):
    e = {'name': 'tiny', 'question': 'Is n a square at the place?', 'operation': 'roots 2',
         'inputs': [{'value': '2'}, {'value': '-1'}, {'value': '3'}],
         'places': ['R', 2, 7], 'precision': {'R': 64, 'p': 4},
         'budget': {'max_cells': 9, 'timeout': 20}}
    e.update(over)
    return sweep.validate(e)


class BudgetTest(unittest.TestCase):
    def test_plan_cut_by_cells(self):
        e = small_experiment(places=['R', 2, 3, 5], budget={'max_cells': 7, 'timeout': 20})
        cells = sweep.plan(e)
        self.assertEqual(len(cells), 12)
        run = [c for c in cells if c['run']]
        below = [c for c in cells if not c['run']]
        self.assertEqual({c['row'] for c in run}, {'R', 2})
        self.assertEqual({c['row'] for c in below}, {3, 5})
        self.assertTrue(all('max_cells' in c['reason'] for c in below))

    def test_stop_line(self):
        e = small_experiment(places=['R', 2, 3, 5], budget={'max_cells': 7, 'timeout': 20})
        records = sweep.records_from_outputs(e, answers_for(e, ['R', 2]))
        self.assertEqual(sweep.stop_line(records), {'row': 3, 'below': 2, 'swept': ['R', 2]})
        self.assertTrue(all(r['status'] == 'below the stop line' for r in records if r['row'] in (3, 5)))

    def test_no_cut(self):
        e = small_experiment()
        self.assertTrue(all(c['run'] for c in sweep.plan(e)))

    def test_timeout_places_stop_line(self):
        # A fake driver that answers row R (three lines) and one cell of row 2, then hangs.
        with tempfile.TemporaryDirectory() as d:
            fake = Path(d) / 'fake_adf'
            fake.write_text('#!/bin/sh\ncat > /dev/null\necho none\necho none\necho none\n'
                            'echo "error: DOMAIN"\nsleep 30\n')
            fake.chmod(fake.stat().st_mode | stat.S_IXUSR)
            e = small_experiment(budget={'max_cells': 9, 'timeout': 1})
            t0 = time.perf_counter()
            records, run = sweep.sweep(e, fake)
            elapsed = time.perf_counter() - t0
        self.assertLess(elapsed, 5)                      # the hanging process group is killed at the deadline
        self.assertTrue(run['timed_out'])
        self.assertEqual(run['answered'], 4)
        self.assertEqual([(r['status'], r['count']) for r in records[:3]], [('OK', 0)] * 3)
        self.assertEqual(sweep.stop_line(records)['row'], 2)
        cut = [r for r in records if r['row'] in (2, 7)]
        self.assertEqual(len(cut), 6)
        self.assertTrue(all(r['status'] == 'below the stop line' and 'timeout' in r['reason'] for r in cut))
        self.assertEqual(cut[0]['output'], 'error: DOMAIN')    # an answer of a cut row is kept in its record


@unittest.skipUnless(DRIVER.exists(), 'build/adf not built')
class DriverTest(unittest.TestCase):
    def test_three_by_three(self):
        e = small_experiment()
        records, run = sweep.sweep(e, DRIVER)
        self.assertFalse(run['timed_out'])
        self.assertEqual(len(records), 9)
        cell = {(r['row'], r['column']): r for r in records}
        self.assertEqual(cell[('R', 0)]['count'], 2)            # 2 > 0
        self.assertEqual(cell[('R', 1)]['count'], 0)            # x^2 + 1 has no real root: OK, `none`
        self.assertEqual(cell[(2, 0)]['status'], 'DOMAIN')      # v_2(2) odd
        self.assertEqual(cell[(7, 0)]['count'], 2)              # 2 = 3^2 mod 7
        self.assertEqual(cell[(7, 1)]['status'], 'DOMAIN')      # 7 = 3 mod 4
        self.assertEqual(cell[(7, 0)]['line'], 'roots_at 2 with 7 with 2')
        self.assertEqual(sweep.stop_line(records), {'row': None, 'below': 0, 'swept': ['R', 2, 7]})


class RenderTest(unittest.TestCase):
    def records(self):
        e = small_experiment()
        fake = [
            ('R', 0, '-1.41 +/- 0.01; 1.41 +/- 0.01'), ('R', 1, 'none'), ('R', 2, '-1.73 +/- 0.01; 1.73 +/- 0.01'),
            (2, 0, 'error: DOMAIN'), (2, 1, 'error: DOMAIN'), (2, 2, 'error: DOMAIN'),
            (7, 0, '7 [3]: 1802916 + O(7^8); 7 [4]: 3961885 + O(7^8)'), (7, 1, 'error: DOMAIN'),
            (7, 2, 'error: DOMAIN')]
        return e, sweep.records_from_outputs(e, {(r, c): o for r, c, o in fake})

    def test_every_row_and_column(self):
        e, recs = self.records()
        page = render.render(e, recs, [])
        for label in ('2', '-1', '3'):
            self.assertIn('<span class="in">%s</span>' % label, page)
        for row in ('R', '2', '7'):
            self.assertIn('<th scope="row">%s' % row, page)
        self.assertEqual(page.count('class="w '), 9 + 7)   # nine wells and the seven wells of the legend
        self.assertEqual(page.count('class="w ok zero"'), 1 + 1)   # `none` at R for -1, and its legend entry
        self.assertIn('p &gt;= 11: not swept', page)
        self.assertIn('sweeper: 9 calls, 9 done, 0 below the stop line', page)

    def test_stop_line_and_count(self):
        e = small_experiment(places=['R', 2, 3, 5], budget={'max_cells': 7, 'timeout': 20})
        recs = sweep.records_from_outputs(e, answers_for(e, ['R', 2]))
        page = render.render(e, recs, [])
        self.assertNotIn('<th scope="row">3', page)
        self.assertIn('<th scope="row">2', page)
        self.assertIn('2 places of the experiment not run', page)
        self.assertIn('sweeper: 12 calls, 6 done, 6 below the stop line', page)

    def test_finding_rule_checked(self):
        e, recs = self.records()
        good = {'id': 'F1', 'state': 'pattern', 'column': '2', 'text': 'two roots at 7',
                'rule': {'places': 'odd primes', 'when': {'mod': 8, 'residues': [1, 7]},
                         'then': {'count': 2}, 'else': {'status': 'DOMAIN'}}}
        bad = dict(good, id='F2', rule={'places': 'all', 'then': {'count_min': 1}})
        n, exc = render.check_rule(good, e, recs)
        self.assertEqual((n, exc), (1, []))
        n, exc = render.check_rule(bad, e, recs)
        self.assertEqual(n, 3)
        self.assertEqual(exc, [2])
        page = render.render(e, recs, [good, bad])
        self.assertIn('checked by the renderer on 1 place: no exception', page)
        self.assertIn('the records contradict this finding at 2', page)

    def test_rule_with_places_and_without_else(self):
        e, recs = self.records()
        f = {'id': 'F4', 'state': 'pattern', 'column': '2', 'text': 't',
             'rule': {'places': 'primes', 'when': {'places': [7]}, 'then': {'count': 2},
                      'else': {'status': 'DOMAIN'}}}
        self.assertEqual(render.check_rule(f, e, recs), (2, []))
        f['rule'] = {'places': 'all', 'when': {'places': [7]}, 'then': {'count': 2}}
        self.assertEqual(render.check_rule(f, e, recs), (3, []))       # no `else`: nothing asked outside `when`
        f['rule'] = {'places': 'all', 'when': {'places': [2]}, 'then': {'count': 2}}
        self.assertEqual(render.check_rule(f, e, recs), (3, [2]))

    def test_count_min_is_inclusive(self):
        e, recs = self.records()
        f = {'id': 'F5', 'state': 'pattern', 'column': '2', 'text': 't',
             'rule': {'places': 'all', 'then': {'count_min': 2}}}
        self.assertEqual(render.check_rule(f, e, recs), (3, [2]))

    def test_source_lines_must_exist(self):
        render.check_source({'file': 'include/adelefeld/status.h', 'lines': '45-57'})
        with self.assertRaises(ValueError):
            render.check_source({'file': 'include/adelefeld/status.h', 'lines': '45-99999'})
        with self.assertRaises(ValueError):
            render.check_source({'file': 'include/adelefeld/status.h', 'lines': '0'})

    def test_states_have_different_shapes(self):
        css = render.CSS
        shapes = {s: re.search(r'\.st-%s \{([^}]*)\}' % s, css).group(1) for s in render.STATES}
        self.assertEqual(len(set(shapes.values())), len(render.STATES))
        self.assertEqual(set(render.STATES), {'pattern', 'sourced', 'refuted', 'open'})

    def test_sourced_needs_a_file_on_disk(self):
        e, recs = self.records()
        f = {'id': 'F3', 'state': 'sourced', 'text': 't',
             'sources': [{'file': 'refs/none/missing.txt', 'lines': '1-2'}]}
        with self.assertRaises(ValueError):
            render.render(e, recs, [f])


def polymul(a, b):
    out = [0] * (len(a) + len(b) - 1)
    for i, x in enumerate(a):
        for j, y in enumerate(b):
            out[i + j] += x * y
    return out


class FixtureTest(unittest.TestCase):
    NAMES = ('squares', 'local_global', 'exp_edge')

    def test_product_polynomial(self):
        e = sweep.load_experiment(HERE / 'experiments' / 'local_global.json')
        coeffs = [[int(c) for c in inp['poly'].split()] for inp in e['inputs']]
        self.assertEqual(coeffs[0], polymul(polymul(coeffs[1], coeffs[2]), coeffs[3]))

    def test_committed_findings_hold(self):
        for name in self.NAMES:
            e = sweep.load_experiment(HERE / 'experiments' / (name + '.json'))
            recs = render.read_jsonl(HERE / 'out' / (name + '.records.jsonl'))
            findings = json.loads((HERE / 'experiments' / (name + '.findings.json')).read_text())
            for f in findings:
                result = render.check_rule(f, e, recs)
                self.assertIsNotNone(result, f['id'])
                self.assertEqual(result[1], [], '%s %s' % (name, f['id']))
            page = render.render(e, recs, findings)
            self.assertNotIn('contradict', page)
            self.assertEqual((HERE / 'out' / (name + '.html')).read_text(), page)

    def test_scenario_a_wall(self):
        recs = render.read_jsonl(HERE / 'out' / 'squares.records.jsonl')
        self.assertEqual(len(recs), 104)
        two = [r['row'] for r in recs if r['column'] == 0 and r['count'] == 2 and r['row'] != 'R']
        self.assertEqual(two, [p for p in sweep.primes_below(100) if p % 8 in (1, 7)])

    def test_scenario_b_wall(self):
        recs = render.read_jsonl(HERE / 'out' / 'local_global.records.jsonl')
        wells = [r for r in recs if r['kind'] == 'well' and r['column'] == 0]
        self.assertEqual([r['row'] for r in wells], ['R'] + sweep.primes_below(50))
        self.assertTrue(all(r['status'] == 'OK' and r['count'] >= 1 for r in wells))
        checks = {r['id']: r['output'] for r in recs if r['kind'] == 'check'}
        self.assertEqual(checks, {'q13': 'error: DOMAIN', 'q17': 'error: DOMAIN', 'q221': 'error: DOMAIN'})


DRIVE = ROOT / 'lanes' / 'ux-drive-bench'      # the experiment files of the test drive (read only)


def quiet(fn, *args):
    """Call a main() without its printed lines."""
    import contextlib
    import io
    with contextlib.redirect_stdout(io.StringIO()):
        return fn(*args)


def well_text(page, wid):
    """The visible text of the table cell that holds the well `wid`: tags and title attributes removed."""
    m = re.search(r'<td>(<span class="w[^"]*" id="%s".*?)</td>' % re.escape(wid), page, re.S)
    if not m:
        return None
    return re.sub(r'<[^>]*>', ' ', re.sub(r' title="[^"]*"', '', m.group(1))).split()


def card(page, fid):
    """The html of the finding card whose id line starts with `fid`."""
    m = re.search(r'<div class="find"[^>]*>\s*<span class="id">%s ·.*?</div>' % re.escape(fid), page, re.S)
    return m.group(0) if m else ''


class SymbolTest(unittest.TestCase):
    """Repair item 1: the symbol commands of tools/adf/README.md as sweep operations."""

    def test_lines(self):
        x = {'value': '2'}
        self.assertEqual(sweep.command_for('legendre', 7, x), ('legendre 2 with 7', None))
        self.assertEqual(sweep.command_for('jacobi', 7, x)[0], 'jacobi 2 with 7')
        self.assertEqual(sweep.command_for('kronecker', 2, {'value': '3'})[0], 'kronecker 3 with 2')
        for op in ('legendre', 'jacobi', 'kronecker'):
            line, reason = sweep.command_for(op, 'R', x)
            self.assertIsNone(line)
            self.assertIn('tools/adf/README.md', reason)
        self.assertEqual(sweep.command_for('hilbert 3', 'R', {'value': '3'})[0], 'hilbert_at 3 with 3 with real')
        self.assertEqual(sweep.command_for('hilbert -1/2', 2, {'value': '3'})[0], 'hilbert_at 3 with -1/2 with 2')

    def test_validate(self):
        for op in ('legendre', 'jacobi', 'kronecker', 'hilbert 3', 'hilbert -1/2'):
            sweep.validate(dict(name='s', question='q', operation=op, inputs=[{'value': '2'}], places=[3],
                                precision={'p': 8}, budget={'max_cells': 1, 'timeout': 5}))
        for op in ('hilbert', 'hilbert x', 'legendre 2'):
            with self.assertRaises(ValueError):
                sweep.validate(dict(name='s', question='q', operation=op, inputs=[{'value': '2'}], places=[3],
                                    precision={'p': 8}, budget={'max_cells': 1, 'timeout': 5}))

    def test_parse(self):
        for line in ('1', '-1', '0'):
            r = sweep.parse_output('legendre', line)
            self.assertEqual((r['status'], r['class'], r['count'], r['value']), ('OK', 'ok', None, line))
        self.assertEqual(sweep.parse_output('hilbert 3', '-1')['value'], '-1')
        self.assertEqual(sweep.parse_output('kronecker', 'error: NOT_DETERMINED')['class'], 'open')
        with self.assertRaises(ValueError):
            sweep.parse_output('legendre', '2')

    def test_value_well(self):
        e = sweep.validate(dict(name='s', question='q', operation='legendre', inputs=[{'value': '2'}],
                                places=[7], precision={'p': 8}, budget={'max_cells': 1, 'timeout': 5}))
        page = render.render(e, sweep.records_from_outputs(e, {(7, 0): '1'}), [])
        self.assertEqual(well_text(page, 'w-7-0'), ['1'])
        self.assertIn('class="w ok val" id="w-7-0"', page)

    def test_readme_cites_the_symbol_commands(self):
        readme = (HERE / 'README.md').read_text()
        self.assertNotIn('Tier A', readme)
        adf = (ROOT / 'tools' / 'adf' / 'README.md').read_text().splitlines()
        cites = re.findall(r'tools/adf/README\.md:(\d+)-(\d+)', readme)
        cited = '\n'.join('\n'.join(adf[int(a) - 1:int(b)]) for a, b in cites)
        for name in ('legendre X with P', 'jacobi X with B', 'kronecker X with B',
                     'hilbert_at X with Y with PLACE'):
            self.assertIn(name, cited)
            self.assertIn(name.split()[0], readme)

    def test_symbols_wall_agrees_with_squares_wall(self):
        """Place by place: Legendre 1 where x^2 - n has two roots, -1 where DOMAIN at an odd p not dividing n,
        0 where p divides n (and the squares wall says DOMAIN: v_p(n) = 1 is odd). At 2 the Legendre symbol is
        not defined (the driver says DOMAIN), and the squares wall says DOMAIN for 2, -1, 3, 5 for another
        reason (2 has odd valuation; -1, 3, 5 are not 1 mod 8)."""
        e = sweep.load_experiment(HERE / 'experiments' / 'symbols.json')
        self.assertEqual(e['operation'], 'legendre')
        self.assertEqual(e['labels'], ['2', '-1', '3', '5'])
        self.assertEqual(e['rows'], sweep.primes_below(100))
        sym = {(r['row'], r['input']): r for r in render.read_jsonl(HERE / 'out' / 'symbols.records.jsonl')}
        sq = {(r['row'], r['input']): r for r in render.read_jsonl(HERE / 'out' / 'squares.records.jsonl')
              if r['kind'] == 'well' and r['row'] != 'R'}
        self.assertEqual(set(sym), set(sq))
        self.assertEqual(len(sym), 100)
        kinds = {'1': 0, '-1': 0, '0': 0}
        for (p, n), s in sym.items():
            q = sq[(p, n)]
            if p == 2:
                self.assertEqual((s['status'], q['status']), ('DOMAIN', 'DOMAIN'))
                continue
            self.assertEqual(s['status'], 'OK')
            kinds[s['value']] += 1
            if int(n) % p == 0:
                self.assertEqual((s['value'], q['status']), ('0', 'DOMAIN'))
            elif q['status'] == 'OK':
                self.assertEqual((s['value'], q['count']), ('1', 2))
            else:
                self.assertEqual((s['value'], q['status']), ('-1', 'DOMAIN'))
        self.assertEqual(sum(kinds.values()), 96)
        self.assertEqual(kinds['0'], 2)                          # 3 at 3, 5 at 5

    def test_hilbert_product_formula(self):
        """The product of (a, b)_v over R and every prime is 1; the rows hold R and every prime dividing 2ab."""
        e = sweep.load_experiment(HERE / 'experiments' / 'hilbert.json')
        b = int(e['operation'].split()[1])
        recs = [r for r in render.read_jsonl(HERE / 'out' / 'hilbert.records.jsonl') if r['kind'] == 'well']
        for j, inp in enumerate(e['inputs']):
            a = int(inp['value'])
            col = [r for r in recs if r['column'] == j]
            self.assertTrue(all(r['status'] == 'OK' for r in col))
            bad = [p for p in sweep.primes_below(10 ** 4) if (2 * a * b) % p == 0]
            self.assertTrue(set(bad) <= {r['row'] for r in col})
            prod = 1
            for r in col:
                prod *= int(r['value'])
            self.assertEqual(prod, 1, 'column %d' % a)


class ProvenanceTest(unittest.TestCase):
    """Repair item 2 (D1): the footer names the files that were read."""

    def test_footer_without_findings_file(self):
        e, recs = RenderTest().records()
        page = render.render(e, recs, [], records_path='x/tiny.records.jsonl', findings_path=None)
        foot = re.search(r'<p class="foot">(.*?)</p>', page, re.S).group(1)
        self.assertIn('x/tiny.records.jsonl', foot)
        self.assertIn('no findings file', foot)

    @unittest.skipUnless(DRIVER.exists() and (DRIVE / 'brief_a.json').exists(), 'driver or drive files missing')
    def test_custom_out_footer_paths_exist(self):
        with tempfile.TemporaryDirectory() as d:
            quiet(sweep.main, [str(DRIVE / 'brief_a.json'), '--driver', str(DRIVER), '--out', d])
            quiet(render.main, [str(DRIVE / 'brief_a.json'), '--out', d])
            page = (Path(d) / 'brief_a.html').read_text()
            foot = re.search(r'<p class="foot">(.*?)</p>', page, re.S).group(1)
            named = re.findall(r'<code>([^<]+)</code>', foot)
            self.assertEqual(len(named), 2)
            for path in named:
                full = Path(path) if os.path.isabs(path) else ROOT / path
                self.assertTrue(full.is_file(), path)
            self.assertEqual(Path(named[0]).resolve(), (Path(d) / 'brief_a.records.jsonl').resolve())
            self.assertEqual((ROOT / named[1]).resolve(), (DRIVE / 'brief_a.findings.json').resolve())


class PrecisionTest(unittest.TestCase):
    """Repair item 3: the requested precision is labelled, a smaller printed O(p^k) is shown in the well."""

    def test_well_shows_smaller_precision(self):
        e = small_experiment(operation='roots 2', inputs=[{'value': '(* ; 9 mod 15625)'}], places=[5, 7],
                             precision={'p': 8}, budget={'max_cells': 2, 'timeout': 5})
        recs = sweep.records_from_outputs(e, {(5, 0): '5 [2]: 15622 + O(5^6); 5 [3]: 3 + O(5^6)',
                                              (7, 0): '7 [3]: 1802916 + O(7^8); 7 [4]: 3961885 + O(7^8)'})
        page = render.render(e, recs, [])
        self.assertEqual(well_text(page, 'w-5-0'), ['2', 'O(5^6)'])
        self.assertEqual(well_text(page, 'w-7-0'), ['2'])          # O(7^8) is the requested 8: no label
        self.assertIn('<span class="m8">requested prec 8</span>', page)

    def test_smallest_printed_precision(self):
        e = small_experiment(operation='roots 2', inputs=[{'value': '9'}], places=[5], precision={'p': 8},
                             budget={'max_cells': 1, 'timeout': 5})
        recs = sweep.records_from_outputs(e, {(5, 0): '5 [2]: 15622 + O(5^6); 5 [3]: 3 + O(5^8)'})
        self.assertEqual(well_text(render.render(e, recs, []), 'w-5-0'), ['2', 'O(5^6)'])

    @unittest.skipUnless(DRIVER.exists() and (DRIVE / 'finite_ball.json').exists(),
                         'driver or drive files missing')
    def test_finite_ball(self):
        with tempfile.TemporaryDirectory() as d:
            quiet(sweep.main, [str(DRIVE / 'finite_ball.json'), '--driver', str(DRIVER), '--out', d])
            quiet(render.main, [str(DRIVE / 'finite_ball.json'), '--out', d])
            page = (Path(d) / 'finite_ball.html').read_text()
        self.assertEqual(well_text(page, 'w-5-0'), ['2', 'O(5^6)'])
        self.assertEqual(page.count('<span class="m8">requested prec 8</span>'), 5)


class RefutedTest(unittest.TestCase):
    """Repair item 4 (D2): a finding contradicted by the records is drawn refuted, whatever its file says."""

    def test_pattern_contradicted_is_drawn_refuted(self):
        e, recs = RenderTest().records()
        f = {'id': 'F2', 'state': 'pattern', 'column': '2', 'text': 't',
             'rule': {'places': 'all', 'then': {'count_min': 1}}}
        c = card(render.render(e, recs, [f]), 'F2')
        self.assertIn('badge st-refuted', c)
        self.assertNotIn('st-pattern', c)
        self.assertIn('written as pattern, refuted by the records at 2', c)
        self.assertEqual(f['state'], 'pattern')

    def test_refuted_covers_nothing_below_the_stop_line(self):
        e, recs = RenderTest().records()
        f = {'id': 'F6', 'state': 'pattern', 'column': '2', 'text': 't', 'argument': 'an argument',
             'rule': {'places': 'all', 'then': {'count_min': 1}}}
        page = render.render(e, recs, [f])
        self.assertNotIn('class="argband"', page)
        self.assertIn('Below the stop line: nothing: the records refute it.', card(page, 'F6'))
        f['rule'] = {'places': [7], 'then': {'count': 2}}
        self.assertIn('class="argband"', render.render(e, recs, [f]))

    @unittest.skipUnless(DRIVER.exists() and (DRIVE / 'attack.json').exists(), 'driver or drive files missing')
    def test_attack(self):
        fpath = DRIVE / 'attack.findings.json'
        before = fpath.read_text()
        with tempfile.TemporaryDirectory() as d:
            quiet(sweep.main, [str(DRIVE / 'attack.json'), '--driver', str(DRIVER), '--out', d])
            quiet(render.main, [str(DRIVE / 'attack.json'), '--out', d])
            page = (Path(d) / 'attack.html').read_text()
        c = card(page, 'FALSE')
        self.assertIn('badge st-refuted', c)
        self.assertIn('refuted: a well contradicts it', c)
        self.assertNotIn('st-pattern', c)
        self.assertNotIn('seen in the swept rows', c)
        self.assertIn('written as pattern, refuted by the records at 3, 5, 11, 13, 19, 29, 37, 43.', c)
        cells = re.search(r'data-cells="([^"]*)"', c).group(1).split()
        self.assertEqual(cells, ['w-%d-1' % p for p in (3, 5, 11, 13, 19, 29, 37, 43)])
        self.assertEqual(fpath.read_text(), before)
        self.assertEqual(json.loads(before)[0]['state'], 'pattern')


class LifetimeTest(unittest.TestCase):
    """Repair item 5 (D3): process_time_s is the lifetime of the driver process; last_answer_s the last line."""

    @unittest.skipUnless((DRIVE / 'timeout_real.json').exists(), 'drive files missing')
    def test_timeout_real_recipe(self):
        e = sweep.load_experiment(DRIVE / 'timeout_real.json')
        self.assertEqual(e['budget']['timeout'], 4)
        with tempfile.TemporaryDirectory() as d:
            fake = Path(d) / 'fake_adf'
            fake.write_text('#!/bin/sh\ncat > /dev/null\necho "error: DOMAIN"\necho "error: DOMAIN"\nsleep 30\n')
            fake.chmod(fake.stat().st_mode | stat.S_IXUSR)
            records, run = sweep.sweep(e, fake)
        self.assertTrue(run['timed_out'])
        self.assertEqual(run['answered'], 2)
        self.assertLess(run['last_answer_s'], 1.0)
        self.assertGreaterEqual(run['process_time_s'], 4.0)
        self.assertLess(run['process_time_s'], 5.0)
        readme = (HERE / 'README.md').read_text()
        self.assertIn('last_answer_s', readme)

if __name__ == '__main__':
    unittest.main()
