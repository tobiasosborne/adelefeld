#!exit 1
# Conventions 10.1/10.2; exact dyadics and golden dump rows. Expectations written before running.
dump union((0.5 ; 7)) + Q
dump (0.5 ; 1/3 mod 1) + Q
load adf1 Q qclass lift 1 1 -1 0 0 g 1 3 3
load adf1 Q qclass pieces 2 1 1 -2 0 0 g 0 3 1 1 3 -2 0 0 g 1 3 1
load adf1 Q qclass pieces 0
load adf1 Q qclass pieces 2 1 3 -2 0 0 g 1 3 1 1 1 -2 0 0 g 0 3 1
load adf1 Q qclass pieces 1 1 1 -1 0 0 l 2 6 2 2 3 0 0
load adf1 Q qclass pieces 1 1 1 -100001 0 0 g 0 1 1
