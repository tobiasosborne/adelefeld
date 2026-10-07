#!exit 1
# Conventions 10.1/10.2. Zero polynomial terms persist. Expected lines written before the run.
dump rfun()
dump rfun(term(P=[], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
load adf1 Q rfun 0
load adf1 Q rfun 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
load adf1 Q rfun 1 1 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
load adf1 Q rfun 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
load adf1 Q rfun z
