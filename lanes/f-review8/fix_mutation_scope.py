from pathlib import Path
p=Path('lanes/f-review8/mutations.py')
s=p.read_text()
s=s.replace("('powrat_wrong_centre_65537', 'lpow', 'if (st == ADF_OK) adf_lball_set(y, res);',",
"('powrat_wrong_centre_65537', 'lpow', 'if (st == ADF_OK) adf_lball_set(y, res);\\n    adf_lball_clear(r);',")
s=s.replace("'fmpz_add_ui(fmpq_numref(res->u), fmpq_numref(res->u), p); adf_lball_set(y, res); }'),",
"'fmpz_add_ui(fmpq_numref(res->u), fmpq_numref(res->u), p); adf_lball_set(y, res); }\\n    adf_lball_clear(r);'),")
p.write_text(s)
