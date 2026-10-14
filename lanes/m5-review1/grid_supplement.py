from checks import *
ds=list(dict.fromkeys([(4,3) if x==(8,7) else (5,3) if x==(10,3) else x for x in GOLD+EXTRA]));ds=[x for x in ds if parity(*x)]
rows=[];cases=[]
for q,n in ds:
 for bits in (20,53,80):
  cases.append((q,n,bits));rows.append(row(len(rows),1,q,n,0,mp.mpc('-4.5'),bits=bits,prec=128))
lines=run(rows)
for (q,n,bits),line in zip(cases,lines):
 refs=global_value(q,n,mp.mpc('-4.5'));fr=flint_value(q,n,mp.mpc('-4.5'))
 for fn,(st,preserve,where,b) in enumerate(results(line)):
  if fn==0:check(st==DOMAIN and preserve==1,('supplement domain',q,n,bits,st));continue
  check(st==OK,('supplement status',q,n,bits,fn,st))
  check(width(b)<=mp.power(2,-bits),('supplement width',q,n,bits,fn,b))
  check(contains(b,refs[int(fn==2)]) and contains(b,fr[int(fn==2)]),('supplement enclosure',q,n,bits,fn,b))
  counts['supplement_OK']+=1
counts['supplement_descriptors']=len(ds);counts['supplement_inputs']=len(rows)
print(json.dumps({'mode':'grid supplement','counts':dict(counts),'failures':repr(failures)}));raise SystemExit(bool(failures))
