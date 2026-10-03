# IL5, IL8 integer CRT. Log(1)=log_abs(-1)=0. All expected lines written before the run.
prec 5
Log_refine (1 ; 4 * [1 mod 9]) with 3
Log_refine (1 ; 4 * [1 mod 9]) with 2 3
Log_refine (1 ; 4 * [1 mod 9]) with 3 5
Log_refine (1 ; 4 * [1 mod 9]) with 5 3 2
log_abs_refine (-1 ; 4 * [1 mod 9]) with 3
logabs_refine (-1 ; 4 * [1 mod 9]) with 3
Log_refine (1 ; 4 * [5 mod 6]) with 2 3 5
Log_refine (1 ; 1 * [-1]) with 2 3
Log_refine (1 ; 1 * [1]) with 3
Log_refine (1 ; 4 * [1 mod 9]) with none
prec 1
Log_refine (1 ; 4 * [1 mod 9]) with 2 3
