# lanes/ux-notebook-r1/records_diff.py: compare the scenario records at HEAD with the regenerated ones, by field.
import json, subprocess, sys
for n in ("a_roots", "b_which_rational"):
    p = f"ux/notebook/out/{n}.records.jsonl"
    old = [json.loads(l) for l in subprocess.run(["git", "show", f"HEAD:{p}"], capture_output=True,
                                                   text=True).stdout.splitlines()]
    new = [json.loads(l) for l in open(p)]
    print(n, len(old), "records at HEAD,", len(new), "now")
    for o, w in zip(old, new):
        changed = [k for k in o if k in w and o[k] != w[k]]
        added = [k for k in w if k not in o]
        print(f"  cell {w['cell']} {w['kind']}: value same: {o['value'] == w['value']}; status same: "
              f"{o['status'] == w['status']}; changed: {changed}; added: {added}")
        for k in changed:
            if k in ("driver", "dump"):
                print(f"    {k}: {o[k]} -> {w[k]}")
