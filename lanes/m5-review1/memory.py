from checks import *
part=int(sys.argv[1]);rng=random.Random(5419+part);cases=[];rows=[]
binary=sys.argv[2] if len(sys.argv)>2 else 'runner-san'
number=int(sys.argv[3]) if len(sys.argv)>3 else 500
for i in range(number):
    q,n=rng.choice([(1,1),(4,3)] if binary=='runner-valgrind' else [(1,1),(4,3),(5,2),(7,3),(9,2)])
    s=mp.mpc(rng.randrange(-24,25)/8,rng.randrange(1,81)/8);r=rng.choice([9999,-60]);bits=20 if binary=='runner-valgrind' else rng.choice([20,53]);kind=i%2
    p=0 if kind else (2 if q==4 else 3 if q==9 else q if q>1 else rng.choice([2,3,5,7,11,13]))
    cases.append((kind,q,n,p,s,r,bits));rows.append(row(i,kind,q,n,p,s,r,a=mp.mpc('.5','.75'),bits=bits,prec=64 if binary=='runner-valgrind' else 128))
lines=run(rows,binary)
for c,l in zip(cases,lines):
    kind,q,n,p,s,r,bits=c
    for fn,(st,preserve,where,b) in enumerate(results(l)):
        if st:check(preserve==1 and (kind!=0 or where==1),('san atomicity',c,fn,st))
        else:
            if kind:
                check(width(b)<=mp.power(2,-bits),('san width',c,fn,b))
                ref=global_value(q,n,s)[int(fn==2)]
                check(contains(b,ref),('san global value',c,fn,b,ref))
            if not kind:
                ref=local(q,n,p,mp.mpc('.5','.75'),s)[fn];check(contains(b,ref),('san local value',c,fn,b,ref))
        counts[f'san_status_{st}']+=1
    counts['san_global_inputs' if kind else 'san_local_inputs']+=1
print(json.dumps({'mode':'memory','part':part,'counts':dict(counts),'failures':repr(failures)}));raise SystemExit(bool(failures))
