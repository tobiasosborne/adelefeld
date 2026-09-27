# Report of lane m0-amend (SPEC, PLAN, PERF to version 1.1)

Date: 2026-09-28. Author: Claude opus subagent. Saved to disk by the orchestrator from the agent's final
message, verbatim below.

m0-amend: all of the brief is applied, plus the later message (F7–F10, `adf1 Q` dump header, `inf` label). The harness refused `lanes/m0-amend/report.md`, so this is the report. No git state was changed.
**SPEC 1.1**
- Change log added; new label [quoted]. Section 1: C library, Python only for checks, Julia later (TJO). Section 3: rows `A_K`/`A_F` corrected; seams R1–R9 adopted as a list.
- 4.1: local backend rewritten per policies P24, P25, S26; raw local data (CV-55, proposed). 4.4: scaled-policy rules now cited as proved; tight scaled product is a separate operation; the cap never touches an exact value.
- 5: exact units with modulus 0; power `c^k U(N)` is the default, `M_k` a separate operation; division by an idele uses the P19 ball; stored `t^s chi(u')` vs `conj(chi)` of the Tate integral (F10, analysis P11).
- 6: `k+1` pieces; closed balls may exceed `[0,1]`, midpoint invariant; transform-convention note. 8: continuation and functional equation proved. 9.2: closed interval. 9.3: functions.md cited by number.
- 9.3.7: a source column; Hilbert unit part `r'`; pole status; finest-modulus centre `r`, not `c^e`; `_exp_u`/`_exp_uinv`, old names only in a source-pending remark.
- 10: dump loader validates first; Julia item. 15: 15.1 TJO, 15.2 D1–D11.
- Decision ids are written M0-D1..M0-D12, because D1–D4 are already review-1 finding ids in SPEC; `conventions.md` still says D1..D12.
- Labels: all 7 [unverified]/[standard] statements now [quoted] (16 places), each cited line read in `refs/src/`. Kept: [unverified] on "Tate's thesis, section 2.2" (thesis pending). New source-pending remark: the names "arithmetic"/"geometric".
**PLAN 1.1**
- Milestone 0 status per work package with commits. Section 4: exact-unit coset, scaled `exact` field, `adf_place_t`, raw local backend (F7), residues `1..N` (F8).
- Section 5 matches conventions §§9–10. Rows 1.4, 1.6, 1.7, 1.8, 1F.9, 2.1, 2.3, 2.4, 3.1 adjusted; new row 1.9, the Julia-friendly interface check.
- Section 8: the four proof reviews total 103 statements, 86 valid, 15 minor, 2 invalid. Two new risks.
**PERF 1.1**
- P1/P2: figures are the measured-loop column of the register form. Zen 2 `add r64,r64`: measured 0.25 cycles/instruction (4/cycle); the memory form measures 0.50. `adc`: register form 0.35 measured, 0.25 documented; memory form 0.50.
- Zen 2 `imul`, loads and stores are now quoted instead of from memory. New profile 1b and section 3b; section 4 conversion; section 7 now asks every cycle figure to state its clock source.
**Intel profile**
- i7-1365U, model 186 = RAPTORLAKE_P. The harness pins to cpu 2, a performance core (P-cores are cpus 0–3, E-cores 4–11).
- uops.info has no Raptor Lake data. I used its Alder Lake-P (Golden Cove) figures, the closest it has, and say so in PERF.
- Clock during the run: 3.70–3.80 GHz, from a dependent-add probe (3.785 GHz before and after) and `scaling_cur_freq`.
- Floors and provisional ratios (run `2026-09-27T221654Z`, `quiet_machine: no`), as floor (a) plain / (b) compiled kernel → ratio (a); (b):
  - add chain: 1 / 3 cycles → x3.0–3.1; x1.00–1.03
  - mul chain: 4 / 18 → x4.6–4.7; x1.02–1.05
  - mulmod2 chain: 4 → x5.5–5.7
  - add batch: 0.2 / 0.8 → x9.9–10.2; x2.5
  - mul batch: 1 / 3.5 → x5.4–5.6; x1.55–1.59
  - mulmod2 batch: 1 → x10.8–11.1
- 4096-bit I/O floors in this profile: 10.7, 16, 16, 10.7 cycles.
- The mul-chain floor first came out at 19 cycles, above the measured 18.4–18.9. `add r64,imm8` is eliminated on Alder Lake-P (latency 0), so the floor is 18; recorded in PERF 3b.
**Not applied (files not mine)**
- `docs/sources.md` and `refs/README.md` need a row for `uops-intel`. Those pages also settle pending item 4 (Zen 2 register forms).
- `ideles.md` Def 8 vs CV-16 (F8) and `catalogue.md` P15 names (F9) are unchanged.
- The harness records no core clock; the probe is `lanes/m0-amend/clockprobe.c`.
- New files: `refs/fetch_intel.sh`, `refs/manifest-intel.sha256` (20 pages, all hashes OK), `bench/results/2026-09-27T221654Z_word.txt`, `lanes/m0-amend/{clockprobe.c,run.log,freq_cpu2.txt}`.
