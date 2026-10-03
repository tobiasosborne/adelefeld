# Note of the orchestrator on lane t-slice2 (2026-10-04)

The lane wrote `src/text_local.c` with `assemble_text_local.py`: 1008 lines copied from `src/text.c` (which was
read-only for it) followed by its own 847 lines (`text_local_tail.c`). The orchestrator removed the copy: the
tail is now the last section of `src/text.c` (one added line, `#include <stdlib.h>` for `qsort`), and
`src/text_local.c`, the assembly script and the tail file are deleted. The vectors moved to
`tests/ref/vectors/t-slice2/`. After the change: `test_text_local` 10 tests, 68868 checks, 0 failed (the lane's
count); `test_text_idele`, `_adele`, `_fball`, `_rat`, `_classify`, `_limits`, `_r3r9` pass unchanged.
`bite.py` of the lane regenerated `src/text_local.c` for each fault and does not run on the tree as merged; its
seven results in the report were obtained on the lane's layout.
