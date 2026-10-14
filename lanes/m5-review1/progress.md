# Review progress

Started 2026-10-14 12:59 UTC. Ownership is lanes/m5-review1 and docs/reviews/m5/review-m5.md.
No slice report, implementation, proto oracle, or t5 reference vector was read before these derivations.

## Independent derivations

1. A shell p^k Z_p^x has multiplicative measure 1, since additive scaling by p^k cancels |x|_p.
   For 1_Zp the integral is the sum over k>=0 of alpha^k p^(-ks) times the unit average of eta_0.
   That average is 1 for the trivial character and 0 otherwise, by translation by a nontrivial unit.
2. The prescribed ramified f cancels eta_0 on units, so its integral is 1. On a residue u mod p^a,
   the additive volume is p^-a. At y=v/p^a its transform is
   p^-a sum_u eta_0(u)^-1 E(-uv/p^a). Substitution w=uv gives eta_0(v) times the sum at v=1.
   The sum vanishes for nonunit v: multiplication by a unit trivial modulo a smaller conductor
   leaves the additive phase fixed and multiplies the sum by a nontrivial character value.
   Thus only shell k=-a contributes to the dual integral. Its contribution is
   p^-a alpha^a p^(a(1-s)) G_minus = alpha^a p^(-as) G_minus.
3. For the trivial unit restriction the Fourier transform of 1_Zp is itself, by additive
   orthogonality. The ratio of dual to original shell sums is
   (1-alpha p^-s)/(1-alpha^-1 p^(s-1)). Its denominator poles differ from the integral poles.
   Their simultaneous vanishing would require 1/p=1, so no cancellation occurs.
4. At infinity the two signs contribute 2 integral_0^infinity x^(s+e-1) exp(-pi x^2) dx.
   Substitution u=pi x^2 gives pi^-((s+e)/2) Gamma((s+e)/2).
   Differentiating the positive-kernel Gaussian transform gives F phi_1=i phi_1.
   The dual/original ratio is i^e pi^(s-1/2) Gamma((1-s+e)/2) rgamma((s+e)/2).
   Poles are 1+e+2k, zeros are -e-2k; epsilon=i^e. Integral poles are -e-2k.
5. Periodizing the balanced Gaussian modulo C gives a negative finite sum and a positive real
   transform. The constants multiply to i^e (-1)^e tau/sqrt(C)=tau/(i^e sqrt(C)).
   Pairing positive and negative n cancels the factor 1/2 in the theta Mellin integral.
   Splitting at t=1 gives exponents z=(s+e)/2 and z'=(1-s+e)/2. Constant terms for C=1 are
   1/(s-1)-1/s. Both decaying integrals are entire by a compact-parameter exponential majorant.
   I=C^-z Lambda. For C>1 Gamma poles are removable in Gamma(z)L(s,chi), not global poles.
6. L14 follows from log(1+u/R)<=u/R for r>=0 and monotonicity for r<0. For its second branch,
   the maximum on [R,2r/b] is at clamp(r/b), and the remaining tail has decay at least b/2.
   P15 follows from n^2 t>=n^2+t-1. Taylor coefficients follow by differentiating
   g'=((z-1)/t-b)g; Cauchy bounds on the disk of radius m/2 give the geometric panel remainder.

Ground truth read: refs/src/tate-poonen/notes.txt:53-65, 693-700, 733-740, 890-1027, 1108-1176;
refs/src/flint-3.0.1/acb_dirichlet.rst:129-132, 203-208, 508-579;
refs/src/flint-3.0.1/acb.rst:893-902. Project statements read: analysis P3,P4,L6,L8,P9-P15;
conventions 6.1-6.5; api-5 sections 1-9; SPEC 8, N-D23,N-D24; PLAN milestone 5.

## Checks completed by 13:26 UTC

- One ordinary archive and one SAN=1 INV=1 archive built. Builds remain in lane storage until final cleanup.
- Phase model: 797 descriptors, 54842 comparisons, zero failures.
- Finite local values: 23520 inputs, 70560 OK calls, zero sampled enclosure or stated-width failures.
- Real integration: 24 direct integrals and 4 functional-equation ratios, zero failures.
- Pole/status cases: 222 inputs, 666 calls; 157 DOMAIN, 375 OK, 122 NOT_DETERMINED, 12 LIMIT.
  Failure representations and where were checked. Zero failures.
- Certificates: 100 cases, 1284 panel thresholds, zero failures; recomputed error/radius <= 0.999999999985.
- L14: 96 direct bound probes, 63 first-branch and 33 second-branch, each against numerical integration.
- T3: 4 runs of 20 panels, 80 true Taylor-tail comparisons, zero failures.
- The first global run at (8,5), s=0 disagreed between my finite-difference derivative and FLINT.
  Differentiating the Hurwitz residue sum termwise removed this review-model error; the repeated run passes.
- My first driver run incorrectly invoked argv for global commands; the driver only accepts their script form.
  My next expected-status table incorrectly applied strict dump-loader rejection to imprimitive value text.
  api-3d:70-77 explicitly lowers that text. The table now separates malformed labels from lowering witnesses.
- C=1, s=1/2+2^-40 i, bits=53, prec=2097152 timed out twice at 60 seconds, including a one-input run.
  C=1 at bits=2097152 returns LIMIT at both prec 128 and the precision cap; C=1021 at prec 128 returns LIMIT.
  The combined C=1021/bits-cap/prec-cap input returns NOT_DETERMINED in 0.413 seconds through a phase refusal.

## Final checks and findings

The full matrix is 6534 global inputs and 19602 calls: 14422 OK, 4467 DOMAIN, 713 NOT_DETERMINED.
The maximum accepted diameter/target is 0.9583398933. The 20 odd descriptors receive -4.5 as an
additional point to replace a duplicate -3 in the distinct-centre count; 60 inputs and 120 OK results.
There are 31 distinct primitive descriptors including conductor 1, and 155 independent FE cases.
Random widths: 2000 inputs, 1219 OK within target, 781 NOT_DETERMINED. Width-floor witnesses: 24.

The real grid has 160 inputs and 480 calls. Two calls, the same even-parity function at conductors 1 and 5,
fail the unchanged factor-64 tightness target at -3/4 with both coordinate radii 2^-12.
Diameter 0.00780157227564; sampled coordinate diameter 0.000101160100176; ratio 77.1210414191.
Independent positive-Gamma quadrature and two recurrences reproduce the samples.
This is a tightness-target failure, not an omitted sampled value or an API width guarantee.

All twelve requested faults compile. Eleven are caught by the combined C tests. Copying the production
FE side survives all five C tests; its printed radius differs from the driver fixture.
The extra L14 replay survives continuation/FE alone but is caught by test_tate.
An extra 5a ramified-integral-zero fault is caught by 1026 checks of test_tate_local.

LSan fails under ptrace. ASan/UBSan with leak checks disabled covers 2000 inputs, 6000 calls and the status
families. Valgrind fallback: 200 inputs, 15472 allocations/frees, zero blocks at exit, zero errors.
The first Valgrind attempt read inactive acb storage in my success-byte comparison and timed out at 161 rows.
The harness now compares raw storage only on failure, after initializing its sentinel's whole storage.
The corrected 200-case run completes in 99.039 seconds. The original error was in the review harness.

The one-input, one-CPU cap probe times out at 60.0095 seconds: 51.5547 user CPU seconds and 4.6326 system.
The permitted cap at bits 53 is slow; no API promise requires LIMIT for that input. Record it as a timing
observation, not a wrong-status claim. The five cap/work probes return in 0.013, 3.010, 0.376, 0.373 and 4.331 s.
S12's entire-function claim fails for conductor 1. Its reference description omits (pi/C)^k.
The normalized implementation values and generator's completion factor are correct.
