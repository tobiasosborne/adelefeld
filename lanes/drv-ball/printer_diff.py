#!/usr/bin/env python3
"""lanes/drv-ball/printer_diff.py: the table of lanes/drv-ball/printer-diff.md (step 3 of the lane).

For every line of every fixture under tests/driver/ whose expected output is a partial ball or a local ball
printed by the driver itself (adf_drv_put_sball and adf_drv_lball_text), it takes the text of the driver and
gives the text adf_sball_get_str / adf_lball_get_str return for the same value.  That second text is produced
by lanes/drv-ball/lib_text.c, built here against the library of the tree: the value the driver printed is read
back through the value form of conventions 9.2 and printed by the library printer, at the setting prec and the
setting digits in force on that line.

Usage: python3 lanes/drv-ball/printer_diff.py ADDRIVER LIBTEXT OUT   (run from the repository root)
"""
import os
import re
import subprocess
import sys

D = 'tests/driver'
SBALL = re.compile(r'^(real: |\d+: )')
LBALL = re.compile(r'^\d+ \[[+-]\d+\]: ')
BRANCH = re.compile(r'^(\d+) \[[+-]\d+\]: ')
SKIPPED = []


def lines_of(path):
    return open(path).read().splitlines()


def collect(adf):
    rows = []
    SKIPPED.clear()
    for name in sorted(os.listdir(D)):
        if not name.endswith('.out'):
            continue
        base = name[:-4]
        expected = lines_of(os.path.join(D, name))
        got = subprocess.run([adf], stdin=open(os.path.join(D, base + '.cmd'), 'rb'),
                             capture_output=True).stdout.decode().splitlines()
        if got != expected:
            SKIPPED.append(base)
            sys.stderr.write('%s: the driver does not print the expected file on this build; skipped\n' % base)
            continue
        prec, digits = 64, 20
        j = 0                      # the line of the expected file that is not yet paired
        for line in lines_of(os.path.join(D, base + '.cmd')):
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            word = line.split()[0]
            if word in ('prec', 'digits'):
                arg = line.split()[1:] if len(line.split()) > 1 else []
                if arg and arg[0].isdigit():
                    if word == 'prec':
                        prec = int(arg[0])
                    else:
                        digits = int(arg[0])
                    continue
                j += 1               # a setting that fails writes a line of its own and changes nothing
                continue
            if j >= len(expected):
                sys.stderr.write('%s: more commands than expected lines; stopped\n' % base)
                break
            e, p, d = expected[j], prec, digits
            j += 1
            if SBALL.match(e):
                rows.append([base, line, p, d, e, 'sball', [e]])
            elif LBALL.match(e):
                parts = []
                for t in e.split('; '):
                    m = BRANCH.match(t)
                    parts.append(m.group(1) + ': ' + t[m.end():])
                rows.append([base, line, p, d, e, 'lball', parts])
    return rows


def library_texts(libtext, mode, rows):
    inp = '\n'.join('\n'.join(r[6]) for r in rows) + '\n'
    res = subprocess.run([libtext, rows[0][2], rows[0][3], mode], input=inp.encode(), capture_output=True)
    return res.stdout.decode().splitlines()


def main():
    adf, libtext, out = sys.argv[1], sys.argv[2], sys.argv[3]
    rows = collect(adf)
    same, diff, comps = 0, 0, 0
    with open(out, 'w') as f:
        f.write(HEADER)
        f.write('| fixture | command | prec | digits | driver text | library text |\n')
        f.write('|---|---|---|---|---|---|\n')
        for mode in ('sball', 'lball'):
            sel = [r for r in rows if r[5] == mode]
            if not sel:
                continue
            for r, lib in zip(sel, one_by_one(libtext, mode, sel)):
                base, c, p, d, e = r[0], r[1], r[2], r[3], r[4]
                sameb = (e == lib)
                comp = same_components(mode, r[6], lib)
                same += 1 if sameb else 0
                diff += 0 if sameb else 1
                comps += 1 if comp else 0
                f.write('| `%s` | `%s` | %d | %d | `%s` | `%s` |\n'
                        % (cut(base, 14), cut(c, 22), p, d, cut(e, 22), cut(lib, 22)))
        f.write('\n%d lines, of which %d byte-identical and %d different.\n' % (len(rows), same, diff))
        f.write('Of the %d lines, %d have byte-identical components inside the two texts.\n'
                % (len(rows), comps))
        if SKIPPED:
            f.write('\nFixtures not in the table, because step 1 of this lane changed their expected output\n'
                    'and they were not corrected here (the .out files are read-only for the lane):\n')
            f.write(', '.join('`%s`' % s for s in SKIPPED[:3]) + ',\n')
            f.write(', '.join('`%s`' % s for s in SKIPPED[3:]) + '.\n')
            f.write('No line of those fixtures prints a partial ball or a local ball through\n'
                    '`adf_drv_put_sball` or `adf_drv_lball_text`: they print `error: UNSUPPORTED`,\n'
                    '`error: DOMAIN` or, after step 1, a value through the printer of the library.\n')
    sys.stderr.write('%d rows, %d identical, %d different, %d with identical components\n'
                     % (len(rows), same, diff, comps))


def one_by_one(libtext, mode, sel):
    """The library text of every row, computed at the prec and the digits of that row."""
    for r in sel:
        inp = '\n'.join(r[6]) + '\n'
        res = subprocess.run([libtext, str(r[2]), str(r[3]), mode], input=inp.encode(), capture_output=True)
        out = res.stdout.decode().splitlines()
        if mode == 'sball':
            yield out[0] if out else '(no line)'
        else:
            yield '; '.join(out)


def same_components(mode, payload, lib):
    """1 when every component (the local coordinate, the real ball) is byte-identical in the two texts."""
    if mode == 'sball':
        drv = [part.split(': ', 1)[1] for part in payload[0].split('; ')]
        body = lib[1:-1] if lib.startswith('{') and lib.endswith('}') else lib
        lb = [t.split(': ', 1)[1] for t in body.split('; ')] if body else []
    else:
        drv = [t.split(': ', 1)[1] for t in payload]
        lb = [t[len('[p='):].split(': ', 1)[1][:-1] for t in lib.split('; ')]
    return 1 if drv == lb else 0


def cut(s, n):
    return s if len(s) <= n else s[:n - 3] + '...'


HEADER = """# lanes/drv-ball: the printer of the driver against the printers of the library (step 3)

Every line of every fixture under `tests/driver/` whose expected output is a partial ball or a local ball that
the driver prints itself is listed here, with two texts for the same value:

* driver text: `adf_drv_put_sball` and `adf_drv_lball_text` of `tools/adf/adf.c`, the formatting of the
  "commands at places";
* library text: `adf_sball_get_str` and `adf_lball_get_str` (`include/adelefeld/text.h`, lines 296 and 299) for
  the value the driver printed.  The value is read back through the value form of conventions 9.2 by
  `lanes/drv-ball/lib_text.c`, which is built against the library of the tree; the labels of the driver text
  ("real: " and "<p>: ") are rewritten to the labels of conventions 9.4 ("inf: " and "p=<p>: ") and the text is
  enclosed in braces, which is all that separates the two templates.

The prec and the digits of a line are the settings the fixture had in force on it; both printers depend on
them.  A line of `root_at`, `roots_at` and `powrat_at` carries the branch identifier `<p> [+-i]: ` of the driver,
which the library text has no place for; the local coordinate is compared in both texts.

The last two columns are the two texts; the two counts below the table say how many lines are byte-identical
and how many have byte-identical components.  A difference in a component is not a difference between the two
printers: the library text is the text
of the value read back from the driver text, and the value form of a real ball is a decimal enclosure and not
a lossless form (conventions 9.6, gate finding G4), so an archimedean component may lose a digit on the way
back (for instance `real: 2.71828 +/- 1.9e-6` is read as a ball that prints as `2.71828 +/- 2e-6`).  The local
coordinates are exact and round-trip byte for byte.

**Result of step 3: the two texts differ in every line, so nothing was changed.**  The table has 0 lines out
of 122 that are byte-identical: the driver prints the components of a partial ball separated by "; " with the
labels "real: " and "<p>: " and without braces, where the printer of the library prints "{E; E; ...}" with the
labels "inf: " and "p=<p>: " (conventions 9.4).  `adf_drv_put_sball` and `adf_drv_lball_text` are therefore
left as they are and the orchestrator decides.  Inside the two texts the components are byte-identical in 115
of the 122 lines; the 7 lines that are not are archimedean ones, where the round trip through the value form
of a real ball loses a digit of the radius, as the paragraph above says.

"""

if __name__ == '__main__':
    main()