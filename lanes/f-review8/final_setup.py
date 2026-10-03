from pathlib import Path
p=Path('lanes/f-review8/assemble_report.py')
s=p.read_text().replace('import hashlib','import hashlib\nimport sys')
s=s.replace("(P/'report.md').write_text(text)",
'''if len(sys.argv)>1:
    (P/'report-preview.md').write_text(text)
    print('preview_lines',len(text.splitlines()),'max_line',max(map(len,text.splitlines())))
    sys.exit(0)
(P/'report.md').write_text(text)''')
s=s.replace('The source hashes in source-sha256.txt match the files used by the scratch copies.',
'''The source hashes in source-sha256.txt match the files used by the scratch copies.
`timeout 30 python3 -B lanes/f-review8/final_checks.py preflight` and the same command
without preflight validate 36 cost rows, 16 fault rows, 94 final trials, 4 source hashes,
report structure/line lengths and exact survivor/precision residue certificates.
The preflight result is 0 failures. The final check also validates the file manifest.''')
s=s.replace('Small setup scripts are also listed in the manifest.',
'''Small setup scripts are also listed in the manifest. Their commands were
`timeout 30 python3 -B lanes/f-review8/NAME.py`, with NAME=add_root_fault,
fix_mutation_scope, finish_cost_setup, cost_flush, refine_sign_fault, prose_corrections,
final_setup; every command exited 0. They alter only this lane's scratch harness or prose.''')
p.write_text(s)
p=Path('lanes/f-review8/report_cost.txt')
s=p.read_text().replace('cost.c:53-56','cost.c:51-53').replace('lfunc.c:275-283','lfunc.c:276-280')
p.write_text(s)
# Reflow progress prose and staging prose to the requested line limit; preserve headings and bullet markers.
import textwrap
p=Path('lanes/f-review8/progress.md')
s=p.read_text()
parts=[]
for b in s.split('\n\n'):
    if b.startswith('#'): parts.append(b)
    else: parts.append(textwrap.fill(' '.join(x.strip() for x in b.splitlines()),width=116,
                                    break_long_words=False,break_on_hyphens=False))
p.write_text('\n\n'.join(parts).rstrip()+'\n')
