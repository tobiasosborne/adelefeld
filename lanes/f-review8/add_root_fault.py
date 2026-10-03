from pathlib import Path
p=Path('lanes/f-review8/mutations.py')
s=p.read_text()
old="\n]\ndef prepare():"
new="""
 ('root_wrong_unsampled_lift', 'lroot', '    adf_lball_set(y,res);',
  '    if (x->p==65537 && seed==42 && !res->exact && !fmpq_is_zero(res->u)) '
  'fmpz_add_ui(fmpq_numref(res->u),fmpq_numref(res->u),x->p); adf_lball_set(y,res);'),
]
def prepare():"""
assert s.count(old)==1
p.write_text(s.replace(old,new))
