#!/usr/bin/env python3
"""Run the unchanged referee harness with an identity replacement as the unmodified control.

Run from build/harness, exactly like faults.py. This adds a named no-op to its table;
no source or assertion in the referee's files is changed.
"""
import runpy
import sys

script = 'lanes/f-review4/faults.py'
sys.argv = [script, '../faults', 'control_unmodified']
module = runpy.run_path(script, run_name='referee_control')
marker = '            log_short(term, p, z, w, K, PK);\n'
module['FAULTS']['control_unmodified'] = (marker, marker)
module['main']()
