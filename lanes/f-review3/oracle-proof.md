# Independent oracle

This lane uses the series specified in docs/proofs/functions.md Definition 1.
It imports no implementation or oracle from the author. The arguments below are my own.
Python's Fraction forms each summand in Q. Reduction occurs only after its exact division.
The oracle never divides an already rounded numerator.

## Tail bound

1. Counting multiples of p^j among 1,...,k gives v_p(k!) = sum_j floor(k/p^j).
   Write k in base p and sum the floors. Their sum is (k-s)/(p-1), where s is the digit sum.
   Since k >= 1 implies s >= 1, this is at most k-1 for p=2 and (k-1)/2 for odd p.
2. For exp with w=v_p(x), a degree k term has valuation k*w-v_p(k!).
   At 2, w >= 2, so this is at least k*(w-1)+1.
   At odd p, w >= 1, so it is at least k*(w-1/2)+1/2.
   Both lower bounds increase with k. The oracle omits degrees starting at
   ceil((n+4)/(w-1)) at 2 and ceil(2*(n+4)/(2*w-1)) at odd p.
   Each omitted degree has valuation at least n+4.
3. If e=v_p(k) >= 1, then k >= p^e >= 2^e >= 2e.
   The last inequality follows by induction: 2 >= 2, and 4e >= 2(e+1) for e >= 1.
   It also holds for e=0. Thus v_p(k) <= k/2.
4. For log(1+z), w=v_p(z) >= 1. A degree k term has valuation
   k*w-v_p(k) >= k*(w-1/2). The oracle omits degrees starting at
   ceil(2*(n+4)/(2*w-1)). Each omitted term has valuation at least n+4.
5. These increasing bounds tend to infinity. Finite tails have valuation at least their
   smallest term valuation, by the valuation inequality. The partial sums are Cauchy.
   Completion gives their limits. The closed set p^(n+4) Z_p contains the whole omitted tail.
6. Every retained term is p-integral. Its reduced Fraction denominator is a unit at p.
   Reducing it by the modular inverse therefore gives its exact residue modulo p^n.
   Finite addition commutes with this reduction. The tail proves that the resulting residue
   also equals the infinite series modulo p^n. This method needs no guard working precision.

For n >= 1000, balanced exact rational evaluation avoids one unit inverse per summand.
For exp, a block (a,b) returns P/Q=product_(k=a+1)^b (z/k) and
T/Q=sum_(j=a+1)^b product_(k=a+1)^j (z/k). A one-term block has P=T=num(z), Q=den(z)*b.
Combining adjacent blocks gives P=P1*P2, Q=Q1*Q2, T=T1*Q2+P1*T2.
The final partial sum is 1+T/Q. Induction proves these equalities in Q.
For log, a block returns sum_(k=a)^(b-1) (-1)^(k+1)*z^(k-a)/k.
Its two halves combine as S_left+z^(mid-a)*S_right. Multiply the final block by z.
Every operation is exact. Only the final rational sum is reduced modulo p^n.

## Log at an arbitrary nonzero rational

1. Strip p^m from the rational to obtain a unit a. This uses exact rational arithmetic.
   At 2, a lies in the raw series domain 1+2 Z_2. The oracle evaluates log(a), without the
   library's choice of sign. The equality to Log follows from the specified decomposition:
   a=s*u, s=+-1, and log(-1)=0; the series product identity gives log(a)=log(u).
2. At odd p, residues 1 and -1 have exact torsion lifts 1 and -1.
   Otherwise start with the nonzero residue w of a modulo p and f(T)=T^(p-1)-1.
   It is a root modulo p because the finite nonzero residues form a group of order p-1.
   To justify the exponent without a source: multiplication by a partitions that group
   into cycles of length equal to the order of a. That length divides p-1, giving a^(p-1)=1.
   Its derivative (p-1)*w^(p-2) is a unit.
3. If f(w) is divisible by p^h, put delta=-f(w)/f'(w) modulo p^(2h).
   Delta is divisible by p^h. Polynomial Taylor expansion has integral coefficients.
   All terms beyond the linear term are divisible by delta^2, hence by p^(2h).
   Therefore w+delta is a root modulo p^(2h). The oracle doubles h to reach H=n+3.
4. At every next digit the equation for its lift has a unit linear coefficient, so the next
   digit is unique. The compatible finite roots converge to the unique torsion root w_infty.
   Thus w=w_infty modulo p^H, and a/w differs from a/w_infty by p^H Z_p.
5. Both principal units have difference from 1 of valuation at least 1. For their log series,
   the degree k numerator difference factors as their argument difference times a sum of k
   integral products of valuation at least k-1. After division by k its valuation is at least
   H+k-1-v_p(k) >= H, since k >= p^e >= 1+e implies e <= k-1.
   Every finite partial-sum difference lies in p^H Z_p, and so does the limit difference.
   Therefore log(a/w) is the correct Log(a) modulo p^n.
6. The oracle evaluates that last series in Q with the bound above. It never forms the
   library's a^(p-1) route or divides its answer by p-1.

## Referee notes

Proposition 7b: e(k)=floor(log_p k) changes by at most 1 between adjacent positive integers.
Therefore k*v-e(k) is nondecreasing for v >= 1. If J is its first value at least n, all
later degrees have valuation at least n. Its proof is valid, including powers of p.
The comparison to the safe count follows from e(k) <= k/2, proved in step 3 above.

Proposition 8: a representative modulo p^W retains valuation at least v when W >= v.
For every retained nonconstant degree, k*v is greater than its denominator valuation e.
Also W >= n+D > e. Hence both the true numerator and the modulus are divisible by p^e;
their residue is divisible by p^e as an integer. Its exact division has error in p^(W-e) Z_p.
Unit inversion preserves that exponent. Addition introduces no digit loss. The constant
and empty sums do not need division. The proof's restrictions to nonconstant retained terms
handle n <= 0. I found no failure in these steps.

F4: the induction gives A_1(X)=sum_i (L!/i!)*X^i. Every summand has valuation at least
D=v_p(L!), including when x is a rational with a unit denominator. Congruence modulo
p^(K+D) passes through this integral polynomial. Dividing its residue by p^D leaves K digits.
The same division leaves an invertible residue of L!/p^D. No digit is lost after unit inversion.

F5: if a residue modulo p^W is nonzero, its valuation below W is the true valuation.
If it is zero, W is a lower bound. Increasing that bound reduces the required count.
The zero-centre shortcut additionally requires v(z) >= c; its three code routes satisfy this.

F1, F3 and F6: the domain balls are nested with or disjoint from each input ball.
The nonzero Log domain has the exact zero and zero-containing balls as its two exceptional cases.
At odd p, p-1 is a unit, so dividing the powered log route loses zero absolute digits.
At 2 the chosen sign gives a principal unit. The log image of relative exponent 1 is 4 Z_2.
For every other accepted ball, the isometry on the principal disc gives its stated image exponent.
The image exponents in F6 agree with those of the specified functions.

F7: input exponents are checked before subtraction. The difference M-v is bounded by 2^61.
K is checked before count or power arithmetic. With K*bits(p) <= 2^26, K <= 2^25.
Thus 2*K, K+D, mid*v and the count denominators used in the code cannot overflow a signed word.
The exp count denominator is positive on the domain: (p-1)*w-1 >= 1 for odd p with w >= 1,
and w-1 >= 1 at 2 with w >= 2. The author's domain mutant reaching division by zero is
therefore unreachable with a valid unmutated domain check. Exact results bypass requested N.

F2 proves the zero fibres claimed in its bold statement. Its final prose sentence at
docs/api-1f4.md:97-98 says more: all rational values at rational arguments are these zero values.
The proof has assumed Log(x)=0. It never considers Log(x)=q with q a nonzero rational.
Injectivity on the principal disc would give u=exp(q), not u=1. Nothing in that proof
excludes a rational x with such a q. This is a missing implication, not a numeric counterexample.
[source pending: a proof excluding nonzero rational log and Log values at rational arguments]
