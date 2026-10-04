# lanes/drv-ball: the printer of the driver against the printers of the library (step 3)

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

| fixture | command | prec | digits | driver text | library text |
|---|---|---|---|---|---|
| `drv-ball-ops` | `project 4 with 2` | 8 | 20 | `2: 4` | `{p=2: 4}` |
| `drv-ball-ops` | `project {p=2: 4} wi...` | 8 | 20 | `2: 4` | `{p=2: 4}` |
| `drv-ball-ops` | `exp_at 4 with 2` | 8 | 20 | `2: 77 + O(2^8)` | `{p=2: 77 + O(2^8)}` |
| `drv-ball-ops` | `exp_at {p=2: 4} with 2` | 8 | 20 | `2: 77 + O(2^8)` | `{p=2: 77 + O(2^8)}` |
| `drv-ball-ops` | `project {p=2: 4; p=...` | 8 | 20 | `2: 4; 5: 1` | `{p=2: 4; p=5: 1}` |
| `drv-ball-ops` | `project {p=2: 4; p=...` | 8 | 20 | `5: 1` | `{p=5: 1}` |
| `drv-ball-ops` | `project {p=2: 4 + O...` | 8 | 20 | `2: 4 + O(2^3)` | `{p=2: 4 + O(2^3)}` |
| `drv-ball-ops` | `project 0 with real 2` | 8 | 20 | `real: 0; 2: 0` | `{inf: 0; p=2: 0}` |
| `drv-ball-ops` | `project {inf: 0; p=...` | 8 | 20 | `real: 0; 2: 0` | `{inf: 0; p=2: 0}` |
| `f-at-prime` | `exp_at 5 with 5` | 8 | 20 | `5: 349831 + O(5^8)` | `{p=5: 349831 + O(5^8)}` |
| `f-at-prime` | `log_at 6 with 5` | 8 | 20 | `5: 329930 + O(5^8)` | `{p=5: 329930 + O(5^8)}` |
| `f-at-prime` | `log_at -1 with 2` | 8 | 20 | `2: 0` | `{p=2: 0}` |
| `f-at-prime` | `exp_at 0 with 5` | 8 | 20 | `5: 1` | `{p=5: 1}` |
| `f-at-prime` | `exp_at 4 with 2` | 8 | 20 | `2: 77 + O(2^8)` | `{p=2: 77 + O(2^8)}` |
| `f-at-prime` | `log_at 3 with 2` | 8 | 20 | `2: 244 + O(2^8)` | `{p=2: 244 + O(2^8)}` |
| `f-at-prime` | `log_at 10 with 3` | 8 | 20 | `3: 3492 + O(3^8)` | `{p=3: 3492 + O(3^8)}` |
| `f-at-prime` | `exp_at 20/3 with 5` | 8 | 20 | `5: 183266 + O(5^8)` | `{p=5: 183266 + O(5^8)}` |
| `f-at-prime` | `log_at 1 with 7` | 8 | 20 | `7: 0` | `{p=7: 0}` |
| `f-at-prime` | `log_at 1 with 18446...` | 8 | 20 | `1844674407370955155...` | `{p=1844674407370955...` |
| `f-at-prime` | `exp_at 5 with 5` | 3 | 20 | `5: 81 + O(5^3)` | `{p=5: 81 + O(5^3)}` |
| `f-at-prime` | `exp_at 5 with 5` | 1 | 20 | `5: 1 + O(5^1)` | `{p=5: 1 + O(5^1)}` |
| `f-at-prime` | `exp_at 5 with 5` | 20 | 20 | `5: 55100931209206 +...` | `{p=5: 5510093120920...` |
| `f-at-prime` | `exp_at (* ; 5 mod 2...` | 8 | 20 | `5: 6 + O(5^2)` | `{p=5: 6 + O(5^2)}` |
| `f-at-prime` | `log_at (* ; 6 mod 2...` | 8 | 20 | `5: 5 + O(5^2)` | `{p=5: 5 + O(5^2)}` |
| `f-at-prime` | `exp_at (0 ; 5) with 5` | 8 | 20 | `5: 349831 + O(5^8)` | `{p=5: 349831 + O(5^8)}` |
| `f-at-prime` | `exp_at (0 ; 5) with...` | 8 | 20 | `real: 1` | `{inf: 1}` |
| `f-at-prime` | `exp_at 1 with real` | 64 | 6 | `real: 2.71828 +/- 1...` | `{inf: 2.71828 +/- 2...` |
| `f-at-prime` | `log_at 2 with real` | 64 | 6 | `real: 0.693147 +/- ...` | `{inf: 0.693147 +/- ...` |
| `f-at-prime` | `exp_at (1 ; 5 mod 2...` | 64 | 6 | `real: 2.71828 +/- 1...` | `{inf: 2.71828 +/- 2...` |
| `f-cross` | `project 5 with 5` | 2 | 20 | `5: 5` | `{p=5: 5}` |
| `f-cross` | `exp_at 5 with 5` | 2 | 20 | `5: 6 + O(5^2)` | `{p=5: 6 + O(5^2)}` |
| `f-cross` | `log_at 6 with 5` | 2 | 20 | `5: 5 + O(5^2)` | `{p=5: 5 + O(5^2)}` |
| `f-cross` | `project (1 ; 1) wit...` | 2 | 20 | `real: 1; 2: 1` | `{inf: 1; p=2: 1}` |
| `f-places-1000` | `project 1/3 with re...` | 64 | 6 | `real: 0.333333 +/- ...` | `{inf: 0.333333 +/- ...` |
| `f-places-1000` | `project 1/3 with 79...` | 64 | 6 | `real: 0.333333 +/- ...` | `{inf: 0.333333 +/- ...` |
| `f-project` | `project 2/3 with 2 ...` | 64 | 6 | `real: 0.666667 +/- ...` | `{inf: 0.666667 +/- ...` |
| `f-project` | `project 1/3 with real` | 64 | 6 | `real: 0.333333 +/- ...` | `{inf: 0.333333 +/- ...` |
| `f-project` | `project 0 with 2 real` | 64 | 6 | `real: 0; 2: 0` | `{inf: 0; p=2: 0}` |
| `f-project` | `project -7/4 with 7 2` | 64 | 6 | `2: -7/4; 7: -7/4` | `{p=2: -7/4; p=7: -7/4}` |
| `f-project` | `project 5 with 5 3 2` | 64 | 6 | `2: 5; 3: 5; 5: 5` | `{p=2: 5; p=3: 5; p=...` |
| `f-project` | `project 1/5 with 5` | 64 | 6 | `5: 1/5` | `{p=5: 1/5}` |
| `f-project` | `project 5 with real...` | 64 | 6 | `real: 5; 2: 5; 7: 5` | `{inf: 5; p=2: 5; p=...` |
| `f-project` | `project (0.5 ; 7 mo...` | 64 | 6 | `real: 0.5; 2: 1 + O...` | `{inf: 0.5; p=2: 1 +...` |
| `f-project` | `project (* ; 1/5 mo...` | 64 | 6 | `3: 0 + O(3^0); 5: 1...` | `{p=3: 0 + O(3^0); p...` |
| `f-project` | `project 1/3 with 18...` | 64 | 6 | `1844674407370955155...` | `{p=1844674407370955...` |
| `gfunc-log-v...` | `Log_at (1 ; 4 * [1 ...` | 5 | 20 | `3: 3 + O(3^2)` | `{p=3: 3 + O(3^2)}` |
| `gfunc-log-v...` | `Log_at (1 ; 4 * [1 ...` | 5 | 20 | `5: 0 + O(5^1)` | `{p=5: 0 + O(5^1)}` |
| `gfunc-log-v...` | `Log_at (1 ; 4 * [1 ...` | 5 | 20 | `2: 0 + O(2^2)` | `{p=2: 0 + O(2^2)}` |
| `gfunc-log-v...` | `Log_at (1 ; 1 * [1 ...` | 5 | 20 | `2: 0 + O(2^2)` | `{p=2: 0 + O(2^2)}` |
| `gfunc-log-v...` | `Log_at (1 ; 3 * [-1...` | 5 | 20 | `3: 0` | `{p=3: 0}` |
| `gfunc-log-v...` | `Log_at (1 ; 1/3 * [...` | 5 | 20 | `3: 0 + O(3^2)` | `{p=3: 0 + O(3^2)}` |
| `gfunc-log-v...` | `Log_at (1 ; 4 * [1 ...` | 5 | 20 | `real: 0` | `{inf: 0}` |
| `gfunc-log-v...` | `log_abs_at (-1 ; 4 ...` | 5 | 20 | `real: 0` | `{inf: 0}` |
| `pow-values` | `powunit_at 6 with 5...` | 10 | 20 | `5: 6520516 + O(5^10)` | `{p=5: 6520516 + O(5...` |
| `pow-values` | `powunit_at 6 with 5...` | 10 | 20 | `5: 36 + O(5^3)` | `{p=5: 36 + O(5^3)}` |
| `pow-values` | `powunit_at (* ; 6 m...` | 10 | 20 | `5: 11 + O(5^2)` | `{p=5: 11 + O(5^2)}` |
| `pow-values` | `powunit_at 1 with 5...` | 10 | 20 | `5: 1` | `{p=5: 1}` |
| `pow-values` | `powunit_at 7 with 3...` | 10 | 20 | `3: 1` | `{p=3: 1}` |
| `pow-values` | `powunit_at -1 with ...` | 10 | 20 | `2: -1` | `{p=2: -1}` |
| `pow-values` | `powunit_at -1 with ...` | 10 | 20 | `2: 1 + O(2^1)` | `{p=2: 1 + O(2^1)}` |
| `pow-values` | `powunit_at 3 with 2...` | 10 | 20 | `2: 379 + O(2^10)` | `{p=2: 379 + O(2^10)}` |
| `pow-values` | `powunit_at (* ; 3 m...` | 10 | 20 | `2: 3 + O(2^2)` | `{p=2: 3 + O(2^2)}` |
| `pow-values` | `powunit_at (* ; 3 m...` | 10 | 20 | `2: 1 + O(2^1)` | `{p=2: 1 + O(2^1)}` |
| `pow-values` | `powunit_at (* ; 1 m...` | 10 | 20 | `2: 1 + O(2^1)` | `{p=2: 1 + O(2^1)}` |
| `pow-values` | `powunit_at (* ; 1 m...` | 10 | 20 | `2: 1 + O(2^3)` | `{p=2: 1 + O(2^3)}` |
| `trig-prime` | `sin_at 5 with 5` | 20 | 20 | `5: 91977224184255 +...` | `{p=5: 9197722418425...` |
| `trig-prime` | `sin_at 4 with 2` | 8 | 20 | `2: 36 + O(2^8)` | `{p=2: 36 + O(2^8)}` |
| `trig-prime` | `sin_at 3 with 3` | 5 | 20 | `3: 201 + O(3^5)` | `{p=3: 201 + O(3^5)}` |
| `trig-prime` | `sin_at 0 with 5` | 8 | 20 | `5: 0` | `{p=5: 0}` |
| `trig-prime` | `sin_at (* ; 0 mod 4...` | 8 | 20 | `2: 0 + O(2^2)` | `{p=2: 0 + O(2^2)}` |
| `trig-prime` | `sin_at (* ; 0 mod 1...` | 8 | 20 | `5: 0 + O(5^3)` | `{p=5: 0 + O(5^3)}` |
| `trig-prime` | `sin_at (* ; 0 mod 1...` | 2 | 20 | `5: 0 + O(5^2)` | `{p=5: 0 + O(5^2)}` |
| `trig-prime` | `sin_at (* ; 5 mod 1...` | 8 | 20 | `5: 5 + O(5^3)` | `{p=5: 5 + O(5^3)}` |
| `trig-prime` | `sin_at (* ; 3 mod 9...` | 1 | 20 | `3: 0 + O(3^1)` | `{p=3: 0 + O(3^1)}` |
| `trig-prime` | `sin_at -7 with 7` | 8 | 20 | `7: 904827 + O(7^8)` | `{p=7: 904827 + O(7^8)}` |
| `trig-prime` | `sin_at 13 with 13` | 8 | 20 | `13: 675072203 + O(1...` | `{p=13: 675072203 + ...` |
| `trig-prime` | `cos_at 5 with 5` | 20 | 20 | `5: 15188663525926 +...` | `{p=5: 1518866352592...` |
| `trig-prime` | `cos_at 4 with 2` | 8 | 20 | `2: 89 + O(2^8)` | `{p=2: 89 + O(2^8)}` |
| `trig-prime` | `cos_at 3 with 3` | 5 | 20 | `3: 172 + O(3^5)` | `{p=3: 172 + O(3^5)}` |
| `trig-prime` | `cos_at 0 with 5` | 8 | 20 | `5: 1` | `{p=5: 1}` |
| `trig-prime` | `cos_at (* ; 0 mod 4...` | 8 | 20 | `2: 1 + O(2^3)` | `{p=2: 1 + O(2^3)}` |
| `trig-prime` | `cos_at (* ; 0 mod 1...` | 8 | 20 | `5: 1 + O(5^6)` | `{p=5: 1 + O(5^6)}` |
| `trig-prime` | `cos_at (* ; 0 mod 1...` | 2 | 20 | `5: 1 + O(5^2)` | `{p=5: 1 + O(5^2)}` |
| `trig-prime` | `cos_at (* ; 5 mod 1...` | 8 | 20 | `5: 51 + O(5^3)` | `{p=5: 51 + O(5^3)}` |
| `trig-prime` | `cos_at (* ; 3 mod 9...` | 1 | 20 | `3: 1 + O(3^1)` | `{p=3: 1 + O(3^1)}` |
| `trig-prime` | `cos_at -7 with 7` | 8 | 20 | `7: 289297 + O(7^8)` | `{p=7: 289297 + O(7^8)}` |
| `trig-prime` | `cos_at 13 with 13` | 8 | 20 | `13: 21520630 + O(13^8)` | `{p=13: 21520630 + O...` |
| `trig-prime` | `sinh_at 5 with 5` | 20 | 20 | `5: 28956828517630 +...` | `{p=5: 2895682851763...` |
| `trig-prime` | `sinh_at 4 with 2` | 8 | 20 | `2: 228 + O(2^8)` | `{p=2: 228 + O(2^8)}` |
| `trig-prime` | `sinh_at 3 with 3` | 5 | 20 | `3: 210 + O(3^5)` | `{p=3: 210 + O(3^5)}` |
| `trig-prime` | `sinh_at 0 with 5` | 8 | 20 | `5: 0` | `{p=5: 0}` |
| `trig-prime` | `sinh_at (* ; 0 mod ...` | 8 | 20 | `2: 0 + O(2^2)` | `{p=2: 0 + O(2^2)}` |
| `trig-prime` | `sinh_at (* ; 0 mod ...` | 8 | 20 | `5: 0 + O(5^3)` | `{p=5: 0 + O(5^3)}` |
| `trig-prime` | `sinh_at (* ; 0 mod ...` | 2 | 20 | `5: 0 + O(5^2)` | `{p=5: 0 + O(5^2)}` |
| `trig-prime` | `sinh_at (* ; 5 mod ...` | 8 | 20 | `5: 5 + O(5^3)` | `{p=5: 5 + O(5^3)}` |
| `trig-prime` | `sinh_at (* ; 3 mod ...` | 1 | 20 | `3: 0 + O(3^1)` | `{p=3: 0 + O(3^1)}` |
| `trig-prime` | `sinh_at -7 with 7` | 8 | 20 | `7: 5532240 + O(7^8)` | `{p=7: 5532240 + O(7...` |
| `trig-prime` | `sinh_at 13 with 13` | 8 | 20 | `13: 779653797 + O(1...` | `{p=13: 779653797 + ...` |
| `trig-prime` | `cosh_at 5 with 5` | 20 | 20 | `5: 26144102691576 +...` | `{p=5: 2614410269157...` |
| `trig-prime` | `cosh_at 4 with 2` | 8 | 20 | `2: 105 + O(2^8)` | `{p=2: 105 + O(2^8)}` |
| `trig-prime` | `cosh_at 3 with 3` | 5 | 20 | `3: 19 + O(3^5)` | `{p=3: 19 + O(3^5)}` |
| `trig-prime` | `cosh_at 0 with 5` | 8 | 20 | `5: 1` | `{p=5: 1}` |
| `trig-prime` | `cosh_at (* ; 0 mod ...` | 8 | 20 | `2: 1 + O(2^3)` | `{p=2: 1 + O(2^3)}` |
| `trig-prime` | `cosh_at (* ; 0 mod ...` | 8 | 20 | `5: 1 + O(5^6)` | `{p=5: 1 + O(5^6)}` |
| `trig-prime` | `cosh_at (* ; 0 mod ...` | 2 | 20 | `5: 1 + O(5^2)` | `{p=5: 1 + O(5^2)}` |
| `trig-prime` | `cosh_at (* ; 5 mod ...` | 8 | 20 | `5: 76 + O(5^3)` | `{p=5: 76 + O(5^3)}` |
| `trig-prime` | `cosh_at (* ; 3 mod ...` | 1 | 20 | `3: 1 + O(3^1)` | `{p=3: 1 + O(3^1)}` |
| `trig-prime` | `cosh_at -7 with 7` | 8 | 20 | `7: 3348220 + O(7^8)` | `{p=7: 3348220 + O(7...` |
| `trig-prime` | `cosh_at 13 with 13` | 8 | 20 | `13: 726234913 + O(1...` | `{p=13: 726234913 + ...` |
| `trig-real` | `sin_at 0 with real` | 64 | 20 | `real: 0` | `{inf: 0}` |
| `trig-real` | `sin_at (0 ; 5) with...` | 64 | 20 | `real: 0` | `{inf: 0}` |
| `trig-real` | `cos_at 0 with real` | 64 | 20 | `real: 1` | `{inf: 1}` |
| `trig-real` | `cos_at (0 ; 5) with...` | 64 | 20 | `real: 1` | `{inf: 1}` |
| `trig-real` | `sinh_at 0 with real` | 64 | 20 | `real: 0` | `{inf: 0}` |
| `trig-real` | `sinh_at (0 ; 5) wit...` | 64 | 20 | `real: 0` | `{inf: 0}` |
| `trig-real` | `cosh_at 0 with real` | 64 | 20 | `real: 1` | `{inf: 1}` |
| `trig-real` | `cosh_at (0 ; 5) wit...` | 64 | 20 | `real: 1` | `{inf: 1}` |
| `pow-values` | `powrat_at 9 with 2 ...` | 20 | 20 | `2 [-1]: 27` | `[p=2: 27]` |
| `pow-values` | `powrat_at 9 with 2 ...` | 20 | 20 | `2 [+1]: -27` | `[p=2: -27]` |
| `root-values` | `roots_at 9 with 2 w...` | 20 | 20 | `2 [+1]: -3; 2 [-1]: 3` | `[p=2: -3]; [p=2: 3]` |
| `root-values` | `root_at 9 with 2 wi...` | 20 | 20 | `2 [-1]: 3` | `[p=2: 3]` |
| `root-values` | `roots_at (* ; 1 mod...` | 20 | 20 | `2 [+1]: 1 + O(2^3);...` | `[p=2: 1 + O(2^3)]; ...` |

122 lines, of which 0 byte-identical and 122 different.
Of the 122 lines, 115 have byte-identical components inside the two texts.

Fixtures not in the table, because step 1 of this lane changed their expected output
and they were not corrected here (the .out files are read-only for the lane):
`01_spec_4_1`, `06_pairs`, `07_status`,
`12_status_order`, `13_dump`, `f-places-hostile`.
No line of those fixtures prints a partial ball or a local ball through
`adf_drv_put_sball` or `adf_drv_lball_text`: they print `error: UNSUPPORTED`,
`error: DOMAIN` or, after step 1, a value through the printer of the library.
