from checks import *
unique=list(dict.fromkeys([(4,3) if x==(8,7) else (5,3) if x==(10,3) else x for x in GOLD+EXTRA]))
part=int(sys.argv[1]);subset=unique[:16] if part==0 else unique[16+8*(part-1):16+8*part]
cases=[];rows=[]
for q,n in subset:
    for s,rad,bits in [(mp.mpc('.375','.75'),9999,53),(mp.mpc('.5','7'),9999,53),(mp.mpc(-parity(q,n)-2),9999,80),(mp.mpc(2,3),9999,20),(mp.mpc('.375','.75'),-60,40)]:
        cases.append((q,n,s,rad,bits));rows.append(row(len(rows),4,q,n,0,s,rad,bits=bits,prec=160));rows.append(row(len(rows),4,q,pow(n,-1,q) if q>1 else 1,0,1-s,rad,bits=bits+4,prec=160))
lines=run(rows)
for i,(q,n,s,rad,bits) in enumerate(cases):
    sa,pa,_,A=results(lines[2*i])[0];sb,pb,_,B=results(lines[2*i+1])[0];check(sa==sb==0,('FE status',q,n,s,rad,bits,sa,sb))
    if sa or sb:continue
    W=rootnumber(q,n);mb=mp.mpc(B[0][0],B[1][0]);mr=W*mb
    R=((mr.real,abs(W.real)*B[0][1]+abs(W.imag)*B[1][1]),(mr.imag,abs(W.imag)*B[0][1]+abs(W.real)*B[1][1]))
    check(all(abs(a[0]-r[0])<=a[1]+r[1] for a,r in zip(A,R)),('FE overlap',q,n,s,A,R,W))
    check(width(A)<=mp.power(2,-bits) and width(R)<=mp.power(2,-bits),('FE width',q,n,s,A,R))
    for t in corners(s,rad):
        ref=global_value(q,n,t)[1];fr=flint_value(q,n,t)[1]
        check(contains(A,ref) and contains(R,ref),('FE against own L',q,n,t,A,R,ref))
        check(contains(A,fr) and contains(R,fr),('FE against FLINT L',q,n,t,A,R,fr))
    counts['FE_cases']+=1
counts['FE_descriptors']=len(subset)
print(json.dumps({'mode':'FE','part':part,'counts':dict(counts),'failures':repr(failures)}));raise SystemExit(bool(failures))
