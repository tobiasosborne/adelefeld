from pathlib import Path
p=Path('lanes/f-review8/cost.c')
s=p.read_text().replace('if (argc!=5) return 2;', 'if (argc!=5 && argc!=6) return 2;')
s=s.replace('int ball=atoi(argv[4]), unit=', 'int trials=argc==6 ? 1 : 3;\n    int ball=atoi(argv[4]), unit=')
s=s.replace('for (int i=0;i<3;i++)','for (int i=0;i<trials;i++)')
s=s.replace('qsort(total,3,sizeof(double),cmp); qsort(component,3,sizeof(double),cmp);',
 '''if (trials==1) { total[1]=total[2]=total[0]; component[1]=component[2]=component[0]; }
    qsort(total,trials,sizeof(double),cmp); qsort(component,trials,sizeof(double),cmp);''')
s=s.replace('trials=3\\n",','trials=%d\\n",')
s=s.replace('fmpz_bits(fmpq_numref(x->u)),fmpz_bits(fmpq_numref(s->u)));',
            'fmpz_bits(fmpq_numref(x->u)),fmpz_bits(fmpq_numref(s->u)),trials);')
p.write_text(s)
p=Path('lanes/f-review8/cost_run.py')
s=p.read_text()
s=s.replace("for fn in ('powrat','powunit'):","counter=-1\nfor fn in ('powrat','powunit'):")
s=s.replace("                log=OUT/", "                counter+=1\n                if len(sys.argv)>1 and not(int(sys.argv[1])<=counter<int(sys.argv[2])): continue\n                log=OUT/")
s=s.replace("                with log.open('w') as f:", "                assert not log.exists(), log\n                with log.open('w') as f:")
s=s.replace("str(p),str(N),str(ball)],", "str(p),str(N),str(ball),*(['single'] if p==18446744073709551557 and N==10000 else [])],")
p.write_text(s)
