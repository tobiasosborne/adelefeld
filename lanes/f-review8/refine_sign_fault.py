from pathlib import Path
p=Path('lanes/f-review8/mutations.py')
s=p.read_text()
old="'st = signed_one(res, p, -1, min2(Nc, 2), 0);          /* injected one sign */'"
new="""'{ adf_lball_t odd; adf_lball_init(odd); odd->p=2; fmpq_one(odd->u); '
  'odd->v=0; odd->N=1; odd->exact=0; '
  'st=principal(res,u,odd,A,1,0,-1,-1,Nc); adf_lball_clear(odd); }'"""
assert s.count(old)==1
p.write_text(s.replace(old,new))
