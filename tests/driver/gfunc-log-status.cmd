#!exit 1
# Real domain, explicit unsupported local log_abs, operand kind and place grammar.
Log (-1 ; 4 * [1 mod 9])
Log_at (-1 ; 4 * [1 mod 9]) with real
log_abs_at (1 ; 4 * [1 mod 9]) with 3
Log 1
Log_at (1 ; 4 * [1 mod 9]) with 3 5
Log_at (1 ; 4 * [1 mod 9]) with 4
Log_at (1 ; 4 * [1 mod 9]) with 03
