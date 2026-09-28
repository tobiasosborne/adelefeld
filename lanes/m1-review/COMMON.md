# Code review of milestone 1: rules for every reviewer (adversarial, refute mode)

You review code that another model family wrote. Your task is not to confirm that the tests pass; they do
(`make check`, 30 programs, also under the sanitizers). Your task is to find **an input that breaks it**:

1. **Enclosure.** An input, admitted by the header, for which the result does not contain the true value or
   the true set. This is the worst defect the library can have. Search for it with your own oracle: exact
   arithmetic in Python (`fractions`, `python-flint`) or an enumeration of members, never the code under
   review and not the project's reference alone.
2. **Contract.** A status, an untouched-output rule, an aliasing rule, a canonical form or an ownership rule
   of the header or of `docs/conventions.md` that the code breaks. A memory error (use the sanitizers and
   `valgrind`, which is on the PATH).
3. **Tests that prove less than they say.** A test whose expected value was taken from the program; a test
   that cannot fail; a claim in a lane report that the code does not bear out. The lane reports are under
   `lanes/<lane>/report.md`; read the one of each file you review, and do not trust it.
4. **Ground truth.** A citation in the code (file and line of a proof, of the conventions, of FLINT's
   documentation under `refs/src/flint-3.0.1/`) that does not say what the code claims it says.

Rules.
- You change no file outside your review directory `docs/reviews/m1/<your name>/`. No git command that
  changes state; no `bd`. Build in your own worktree with `make -j2`; at most 2 cores; no computation above
  about 3 minutes.
- Every finding has: an identifier (`R<k>`), a severity (BLOCKER: wrong result or memory error reachable
  through the public interface with admitted input; MAJOR: contract broken, or a test that hides a defect;
  MINOR: everything else), the file and line, the input, what the header or proof requires with its file and
  line, what the code does, and **a program or script in `docs/reviews/m1/<your name>/checks/` that shows it**
  and that you have run. A finding without a reproducer that you ran is marked `UNCONFIRMED` and says why.
- Say also what you examined and found correct, with the method (how many cases, which oracle), so that the
  reader knows what was covered. Say what you did not examine.
- Plain, sober English; short sentences; numbers, not adjectives; lines at most 116 characters.
- Write `docs/reviews/m1/<your name>/review.md`. First line after the title: the verdict, one of
  `NO BLOCKER FOUND`, `BLOCKER FOUND`, with the counts of findings by severity. If you cannot write the file,
  your final message is the review; in every case your final message contains the complete review.
