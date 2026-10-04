"""Create only scratch copies. No repository source is modified."""
from pathlib import Path
import hashlib

lane = Path(__file__).resolve().parent
root = lane.parent.parent
src = (root / 'src/localfactor.c').read_text()
trace = src.replace('if (!acb_is_finite(g))\n    {',
                    'if (!acb_is_finite(g))\n    {\n        review_fallback++;')
trace = trace.replace('mag_mul(B, sum, f);',
                      'mag_mul(B, sum, f);\n    mag_set(review_B, B); review_has_B = 1;')
assert trace != src and 'review_fallback++;' in trace and 'review_has_B = 1' in trace
(lane / 'build/localfactor_trace.c').write_text(trace)
(lane / 'source.sha256').write_text(hashlib.sha256(src.encode()).hexdigest() + '  src/localfactor.c\n')
print('scratch trace: 2 instrumentation sites; source SHA256 written')
