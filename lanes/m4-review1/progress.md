# m4-review1 progress notes

- Signs derived (conventions 6.1:840-847, tate-poonen notes.txt:693-700): psi_inf(x)=E(-x), psi_p(x)=E(fp_p(x)),
  F_c f(y)=int f(x) conj(psi(xy)) dx. Real kernel conj(E(-xy))=E(+xy). Finite: f on j/D+M Zhat (vol 1/M),
  y=k/M: conj(psi_f(jk/(DM)))=E(-jk/L), weight 1/M. Matches api-4.md:166.
- Built plain and SAN+INV archives (exit 0).
- Read src/ffun.c, rfun.c, tensor.c, poisson.c fully before harnesses.
- Hunt 1: 5005 ffun (D,M<=12), 7 runs, ~13.2M cell checks (fourier center+member, F^2, Parseval, add, mul, refine,
  translate, reflect, conj, dilate_rat, dilate_idele, ffnorm, covariance): 0 failures; 164 undecided (exact outputs
  within the 2^-390 numeric error of my reference). Radius metric in hunt1 mixes mag 30-bit rounding; redo.
- Hunt 2: 3000 rfun, 4 runs: 0 failures (closure exact/numeric params, 100 evals each, fourier at 50 y via C eval,
  F^2 at 10 x, integral, norm2 vs own closed form). maxrel_eval ~ 1300 (rad <= 2^(-p+11) max(1,|v|)).
- Hunt 4: 300 tensors, 0 failures; all OK; selfcheck left=right of own model to 1e-71 (signs consistent).
- Hunt 3: 5000 cases (2353+ global evals, ~4400 local, 5000 teval, 5000 sball, 800 bound fns x 1000 pts): 0 fails.
  Harness bug fixed: LOCALFAIL printed two lines.
- Hunt 5: 3000 texts (own printer), 3000 dumps (identity), 20000 mutated dumps: 59 cases where a refused (PARSE)
  ffun dump made 2 fmpz_set_str calls: dp_w_other (src/dump.c:1129-1134) converts the D, M tokens of >15 hex
  digits with fmpz_set_str during the grammar stage (token-validated bytes; text not yet validated). MINOR.
- tests/test_ffun_dump.c, test_rfun_dump.c are 3-line wrappers including lanes/f4-slice7/dump_test.h. MINOR.
- Special: shifted Gaussian Im>0 certified; root branch 4 quadrants ok (quad); theta ok; witness ND U;
  bits -1, 2^21+1 DOMAIN U; prec cap+1 LIMIT U; tiny Re(A) 2^-20, 2^-30, 1e-8 LIMIT < 1 s;
  bits=2^21 (allowed) runs > 160 s (bits 128000: 13 s; x4 bits -> x20 time). MINOR.
