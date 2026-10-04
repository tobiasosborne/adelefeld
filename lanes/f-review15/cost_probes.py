from pathlib import Path
import argparse
import json
import subprocess
import time

lane = Path(__file__).resolve().parent
p = argparse.ArgumentParser()
p.add_argument('group', choices=['precision', 'huge', 'tiny', 'huge-exponent'])
p.add_argument('--place', type=int)
args = p.parse_args()
places = [args.place] if args.place is not None else [2, 0]
rows = []
dest = lane / f'cost-{args.group}-{args.place}.json'
for place in places:
    cases = []
    if args.group == 'precision':
        cases = [(prec, 'real', 0, 1) for prec in [64, 4096, 2**16, 2**20]]
    elif args.group == 'huge':
        cases = [(64, axis, 1000, sign) for axis in ['real-imag', 'imag'] for sign in [1, -1]]
    elif args.group == 'tiny':
        cases = [(64, axis, -2**60, sign) for axis in ['real', 'real-imag', 'imag'] for sign in [1, -1]]
    else:
        cases = [(64, axis, 2**60, sign) for axis in ['real-imag', 'imag'] for sign in [1, -1]]
    for prec, axis, exponent, sign in cases:
        cmd = ['timeout', '60', str(lane / 'build/cost'), str(place), str(prec), axis,
               str(exponent), str(sign)]
        start = time.monotonic()
        r = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        row = {'command': ' '.join(cmd), 'exit': r.returncode,
               'seconds': round(time.monotonic() - start, 6), 'output': r.stdout}
        rows.append(row)
        dest.write_text(json.dumps(rows, indent=2) + '\n')
        print(json.dumps(row), flush=True)
