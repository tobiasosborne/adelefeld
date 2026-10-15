"""Run and log a bounded user-drive command; keep stdout and stderr verbatim."""
import datetime
import json
import pathlib
import shlex
import subprocess
import sys
import textwrap
import time

lane = pathlib.Path(__file__).resolve().parent
key, expected, seconds, *command = sys.argv[1:]
start = subprocess.check_output(['timeout', '5', 'date', '-Is'], text=True).strip()
t0 = time.monotonic()
with (lane / (key + '.txt')).open('w') as out:
    result = subprocess.run(['timeout', seconds, *command], stdout=out, stderr=subprocess.STDOUT)
duration = round(time.monotonic() - t0, 3)
entry = dict(id=key, time=start, command=['timeout', seconds, *command],
             expected=expected, exit=result.returncode, seconds=duration, output=key + '.txt')
with (lane / 'checks.jsonl').open('a') as log:
    log.write(json.dumps(entry) + '\n')
with (lane / 'drive.md').open('a') as log:
    log.write(f'\n### {key}: {start}\n\n')
    log.write('Expected: ' + expected + '\n\nCommand:\n\n```sh\n')
    log.write(' \\\n  '.join(textwrap.wrap(shlex.join(entry['command']), 110,
                                       break_long_words=False, break_on_hyphens=False)))
    log.write(f'\n```\n\nExit {result.returncode}; {duration} seconds. Output: `{key}.txt`.\n')
print(json.dumps(entry))
print((lane / (key + '.txt')).read_text()[:12000])
