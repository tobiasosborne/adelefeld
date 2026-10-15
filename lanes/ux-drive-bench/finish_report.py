"""Assemble the final report and normalize Markdown line widths."""
import datetime
import json
import pathlib
import shlex
import subprocess
import textwrap

lane=pathlib.Path('lanes/ux-drive-bench')
checks=[json.loads(line) for line in (lane/'checks.jsonl').read_text().splitlines()]
assert len(checks)==34
end=subprocess.check_output(['timeout','5','date','-Is'],text=True).strip()
start='2026-10-16T00:37:46+11:00'
elapsed=int((datetime.datetime.fromisoformat(end)-datetime.datetime.fromisoformat(start)).total_seconds())

def command_text(command):
    args=['PATH="$PATH"' if s.startswith('PATH=') else s for s in command]
    lines=['']
    for arg in args:
        tok=arg if arg=='PATH="$PATH"' else shlex.quote(arg)
        if len(tok)>108:
            if lines[-1]:
                lines[-1]+=' \\'
                lines.append('')
            pieces=[shlex.quote(arg[i:i+88]) for i in range(0,len(arg),88)]
            for piece in pieces[:-1]:
                lines[-1]+=piece+'\\'
                lines.append('')
            lines[-1]+=pieces[-1]
        elif len(lines[-1])+len(tok)+1>112:
            lines[-1]+=' \\'
            lines.append(tok)
        else:
            lines[-1]+= (' ' if lines[-1] else '')+tok
    result='\n'.join(lines)
    if not any(s.startswith('PATH=') for s in command):
        assert shlex.split(result.replace('\\\n',''))==command
    return result

report=(lane/'report_body.md').read_text()
report=report.replace('## Work and counts',f'Start: {start}. End: {end}. Elapsed: {elapsed} seconds.\n\n## Work and counts')
report+='\n## Check command appendix\n\n'
report+='Run from the repository root. Output filenames are relative to this lane.\n'
report+='The harness around each command is run_check.py; it records date, exit status and wall duration.\n'
report+='Exit 1 is expected for mixed failing driver queries and for the deliberately invalid source.\n'
report+='The mutation runner exits 0 after verifying its deliberate assertion failure.\n'
report+='The first extra_dom exit 1 is the adapter mapping error; the corrected check exits 0.\n'
report+='The Firefox exit -11 is signal 11. No failure is omitted from this inventory.\n\n'
report+='Setup (not in checks.jsonl):\n\n```sh\n'
report+="timeout 180 bash --noprofile --norc -c \\\n  'timeout 600 make -j2 && timeout 600 make -C tools/adf'\n```\n\n"
report+='Exit 0; build stdout/stderr is in setup.txt.\n\n'
report+='CLI discovery: `timeout 10 python3 ux/bench/sweep.py --help` and\n'
report+='`timeout 10 python3 ux/bench/render.py --help`: both exit 0, both advertise --out.\n'
for c in checks:
    report+=f"\n### {c['id']}\n\n```sh\n{command_text(c['command'])}\n```\n\n"
    report+=f"Exit {c['exit']}; {c['seconds']} seconds. Output: [{c['output']}]({c['output']}).\n"

# Wrap prose only, retaining fenced commands and original paragraphs.
def normalize(text):
    lines=[];fenced=False
    for line in text.splitlines():
        if line.startswith('```'):
            fenced=not fenced
        if fenced or len(line)<=116:
            lines.append(line)
        else:
            lines.extend(textwrap.wrap(line,116,break_long_words=False,break_on_hyphens=False))
    return '\n'.join(lines)+'\n'
report=normalize(report)
assert all(len(line)<=116 for line in report.splitlines())
(lane/'report.md').write_text(report)
# The command logger expanded PATH. Restore the original shell spelling in the human log.
drive=(lane/'drive.md').read_text()
for c in checks:
    for arg in c['command']:
        if arg.startswith('PATH='):
            drive=drive.replace(arg,'PATH="$PATH"')
drive+='\n## Step 6: report\n\n'
drive+=f'{end}: report.md written. Elapsed from the initial dated setup check: {elapsed} seconds.\n'
drive+='34 logged command checks; 3 defects, 5 usability findings, 3 overlapping honesty findings.\n'
drive+='No mathematical mismatch and no finding against SPEC.\n'
drive=normalize(drive)
(lane/'drive.md').write_text(drive)
exclude={'brief.md','lane.log','stdout.log','report_body.md'}
files=sorted(str(p.relative_to(lane)) for p in lane.rglob('*') if p.is_file()
             and str(p.relative_to(lane)) not in exclude)
if 'files.txt' not in files:
    files.append('files.txt');files.sort()
(lane/'files.txt').write_text('\n'.join(files)+'\n')
print(json.dumps(dict(logged_checks=len(checks),elapsed_seconds=elapsed,
                     artifacts=len(files),report_lines=len(report.splitlines()),
                     report_max_line=max(map(len,report.splitlines())),
                     drive_max_line=max(map(len,drive.splitlines())))))
