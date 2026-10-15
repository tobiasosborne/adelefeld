import copy
import json
import pathlib
import re
import subprocess

lane = pathlib.Path(__file__).resolve().parent
root = lane.parent.parent
extra = lane / 'extra-ledger'
(extra / 'claims').mkdir(parents=True, exist_ok=True)

def command(args):
    print('COMMAND:', ' '.join(map(str, args)))
    r = subprocess.run(['timeout', '30', *map(str, args)], text=True, capture_output=True)
    print(r.stdout, r.stderr, 'exit=', r.returncode)
    return r

source = json.loads((root / 'ux/ledger/claims/b6-two-square-family.json').read_text())
branch = copy.deepcopy(source)
branch.update(id='drive-branch', statement='The square root of -1 at 5 is 2 mod 5.',
              script=['prec 1', 'roots_at -1 with 5 with 2'],
              expect=['5 [2]: 2 + O(5^1); 5 [3]: 3 + O(5^1)'], places=[5],
              tail='nothing said', argument='', question='Drive extras')
citation = copy.deepcopy(source)
citation.update(id='drive-citation', argument='No citation supplied.', state='ratified', question='Drive extras')
footprint = copy.deepcopy(source)
footprint.update(id='drive-footprint', places=source['places'] + [101], question='Drive extras')
for c in (branch, citation, footprint):
    (extra / 'claims' / (c['id']+'.json')).write_text(json.dumps(c, indent=2)+'\n')
    print('CLAIM', c['id'], 'places=', c['places'], 'state=', c['state'], 'script lines=', len(c['script']))
    command(['python3', '-B', root / 'ux/ledger/seal.py', '--ledger', extra, '--only', c['id']])
command(['python3', '-B', root / 'ux/ledger/render.py', '--ledger', extra])
page = (extra / 'ledger.html').read_text()
for c in (branch,citation,footprint):
    seal = json.loads((extra/'seals'/(c['id']+'.json')).read_text())
    card = re.search(r'<article[^>]*id="claim-'+c['id']+r'"[\s\S]*?</article>',page)[0]
    print(c['id'], 'verdict=',seal['verdict'],'actual=',seal['actual'])
    if c is branch:
        print('PROMISE RESULT:', 'FAILED: ambiguous singular branch claim accepted' if seal['verdict']=='sealed'
              else 'claim not sealed')
    if c is citation:
        print('PROMISE RESULT:', 'FAILED: missing-source claim displayed RATIFIED' if '>RATIFIED<' in card
              else 'claim not ratified')
    if c is footprint:
        print('PROMISE RESULT:', 'FAILED: uncomputed 101 displayed filled/computed' if '<span class="c">101</span>' in card
              else '101 not displayed computed')
print('Three extra promises tried. Each result is printed above; no slice files changed.')
