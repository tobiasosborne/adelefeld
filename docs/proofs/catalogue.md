# Proofs for the Tier A arithmetic catalogue

This file addresses `docs/SPEC.md` 9.3.7 and section 5. No source text is present under `refs/` at the time of
writing. Standard theorems used below are stated exactly and marked as pending. The formulas are proved here from
those theorems or by finite enumeration. `proto/catalogue_checks.py` checks each numbered statement.

For `N >= 1`, write `U(N) = {u in Zhat^x : u = 1 mod N}`. A unit coset `c U(N)` requires `gcd(c,N)=1`.
An additive ball `a + N Zhat` in this file has integer `a` and positive integer `N`, unless stated otherwise.
The symbol of a set of inputs is determined when it has one value. Otherwise return its finite set of values or
`NOT_DETERMINED`. A status is preferable when enumerating the set would be too expensive.

## Residue symbols

**Definition 1 (quadratic symbols).** Let `p` be an odd prime and `a,b` integers. The Legendre symbol `(a/p)` is
0 if `p | a`, 1 if the nonzero residue of `a` is a square in `F_p`, and -1 otherwise. For a positive odd integer
`m = product p^h` and any integer `a`, the Jacobi symbol `(a/m)` is `product (a/p)^h`; the empty product at `m=1`
is 1. For `b=2^t m > 0`, `m` odd, set `(a/b)=(a/m)(a/2)^t`, where `(a/2)=0` for even `a` and
`(a/2)=(-1)^((a^2-1)/8)` for odd `a`. At `b<0`, set `(a/b)=(a/-1)(a/|b|)`, where `(a/-1)` is -1 for `a<0` and
1 for `a>=0`. At `b=0`, set `(a/0)=1` for `a=1` or `a=-1`, and 0 otherwise. The last two rules apply only to
exact integer `a` in the catalogue API. These are the Kronecker conventions of the specification; an external
source for them is pending.

Proof. These are definitions. The even rule has integer exponent because every odd square is 1 modulo 8.
The check's Euler test follows directly: among the `p-1` nonzero residues there are `(p-1)/2` distinct squares;
each satisfies `a^((p-1)/2)=1` by Fermat's little theorem, proved by multiplying the nonzero residues after
multiplication by `a`. The degree bound gives no further roots, while every nonzero residue has
`a^(p-1)=1`; hence the other values of `a^((p-1)/2)` are -1.

Check: `check_symbols`. Used by: `docs/SPEC.md` 9.3.7, symbol rows.

**Proposition 2 (symbol precision).** Under Definition 1, Legendre at odd `p` depends on `a mod p`; Jacobi at
positive odd `m` depends on `a mod m`. For positive `b=2^t m`, `m` odd, Kronecker depends on `a mod m` if `t=0`,
and on `a mod lcm(m,8)` if `t>0`. These are sufficient moduli, not claims of minimality. In particular
`(1/2)=1` and `(3/2)=-1`, though `1=3 mod 2`.

Proof. (1) Squareness in `F_p` uses only the residue. Jacobi is a product of such terms. (2) For odd `a`, the
exponent `(a^2-1)/8 mod 2` depends on `a mod 8`; for even `a` the factor is 0. (3) Combine the Jacobi and
2 factors by the Chinese remainder theorem. (4) Direct substitution gives the two examples. When `t` is even,
the 2 factor is 1 for odd `a`, so the displayed modulus can be larger than needed.

Check: `check_symbols`. Used by: `docs/SPEC.md` 9.3.7, symbol precision column (R1).

**Proposition 3 (symbols on finite input sets).** Let the lower entry be `p` odd prime, `m` positive odd, or
`b>0` as in Definition 1. Let `K` be the sufficient modulus from Proposition 2. For a unit coset `c U(N)` with
`N>=1`, the exact possible symbols are those of unit residues `r mod lcm(N,K)` satisfying `r=c mod N`. For an
additive ball `a+N Zhat` with integer `a`, `N>=1`, they are those of all residues `r mod lcm(N,K)` satisfying
`r=a mod N`. Return their unique value if the set is a singleton, or the set/status otherwise. If `K | N`, the
value is determined. Negative and zero lower entries require an exact integer numerator.

Proof. (1) The projections of `c U(N)` to `Z/lcm(N,K)` are exactly the unit residues stated: prime by prime,
any allowed unit residue lifts to a profinite unit. (2) The projection of the additive ball is exactly its stated
congruence class. (3) Proposition 2 makes the symbol constant on each residue modulo `K`, so both enumerations
are exact. (4) Divisibility `K | N` makes the residue modulo `K` fixed. This condition is only sufficient; a
coarser input can also happen to give one value. The finite additive ball can include nonunits and hence a zero
symbol; a unit coset cannot give zero for a positive lower entry.

Check: `check_symbols`. Used by: `docs/SPEC.md` 9.3.7, symbol set/status rule.

## Hilbert symbols

**Definition 4 (local symbol and formulas).** Let `a,b` be nonzero elements of `Q_p` for a prime `p`, or nonzero
reals for `v=infinity`. Define `(a,b)_v=1` if `a x^2+b y^2=z^2` has a solution `(x,y,z)` not all zero over
`Q_v`; otherwise set it to -1. For finite `p`, write `a=p^alpha u`, `b=p^beta w` with `alpha,beta` any integers
and unit parts `u,w`. At odd `p`, the claimed formula is

    (a,b)_p = (-1)^(alpha beta (p-1)/2) (u/p)^beta (w/p)^alpha.

The Legendre factors are signs, so negative exponents use the same parity. At `p=2`, for odd unit residues
`u,w mod 8`, put `eps(u)=(u-1)/2 mod 2` and `om(u)=(u^2-1)/8 mod 2`. The claimed formula is

    (a,b)_2 = (-1)^(eps(u)eps(w) + alpha om(w) + beta om(u)).

At infinity it is -1 exactly when `a<0` and `b<0`.

Proof. The formulas are proved in Propositions 5 and 6. At infinity two negative terms cannot sum to a
nonnegative square unless all coordinates vanish; in each other sign pattern a nonzero solution is immediate.

Check: `check_hilbert`. Used by: `docs/SPEC.md` 9.3.7, Hilbert row.

**Proposition 5 (odd-prime formula).** For every odd prime `p` and nonzero `a,b in Q_p`, the odd-prime formula
in Definition 4 equals the solvability definition.

Proof. (1) Scaling `a` or `b` by a square changes neither solvability nor the displayed formula. We may take
`alpha,beta` to be 0 or 1. (2) If both are 0, the sets `{z^2}` and `{u x^2+w}` in `F_p` each have `(p+1)/2`
elements. They intersect. At an intersection with `y=1`, `x,z` cannot both be zero since `w` is a unit. Thus
one derivative `2ux` or `-2z` is a unit. Simple Hensel lifts the solution. (3) If `alpha=1,beta=0`, reduction
modulo `p` gives `w y^2=z^2`. If `(w/p)=1`, choose `x=0,y=1`, then lift `z`. If `(w/p)=-1`, `y,z` are both
divisible by `p`; reduction modulo `p^2` forces `x` divisible by `p`, contrary to primitivity. The interchanged
case is symmetric. (4) If both are 1, reduction first gives `p|z`. Divide the equation by `p` and reduce:
`u x^2+w y^2=0 mod p`, with `x,y` not both zero. This is possible exactly when `(-uw/p)=1`, and a unit
derivative lifts a solution with `z=0`. (5) The four formula cases are respectively 1, `(w/p)`, `(u/p)`, and
`(-uw/p)`. Simple Hensel here means: a polynomial over `Z_p` with a root modulo `p` and a unit derivative has
a root in `Z_p`. [source pending: a local-fields text stating simple Hensel's lemma]
For coefficients of valuation 0 or 1, a primitive solution modulo `p^2` is therefore equivalent to local
solvability. The checks use 9 and 25. Higher powers of those same primes also suffice by reduction to `p^2`.

Check: `check_hilbert`. Used by: `docs/SPEC.md` 9.3.7, Hilbert formula and finite oracle (R2).

**Proposition 6 (2-adic formula and all square classes).** For nonzero `a,b in Q_2`, the 2-adic formula in
Definition 4 equals solvability. There are eight square classes, represented in this order by
`1,3,5,7,2,6,10,14`. The following is the full table; `+` means 1 and `-` means -1. It is printed by
`check_hilbert`, which separately enumerates primitive solutions modulo 16.

| `a` / `b` | 1 | 3 | 5 | 7 | 2 | 6 | 10 | 14 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | + | + | + | + | + | + | + | + |
| 3 | + | - | + | - | - | + | - | + |
| 5 | + | + | + | + | - | - | - | - |
| 7 | + | - | + | - | + | - | + | - |
| 2 | + | - | - | + | + | - | - | + |
| 6 | + | + | - | - | - | - | + | + |
| 10 | + | - | - | + | - | + | + | - |
| 14 | + | + | - | - | + | + | - | - |

Proof. (1) Every odd square is 1 modulo 8. Conversely, if `u=1 mod 8`, apply strong Hensel to `X^2-u` at
`X=1`: `v_2(F)>=3 > 2 v_2(F')=2`. Thus two nonzero 2-adic numbers have the same square class exactly when
their valuation parities and odd unit residues modulo 8 agree. (2) A nonzero local solution can be scaled to
a primitive integral solution; it reduces modulo 16. For coefficients of valuation 0 or 1, the converse holds.
If a primitive modular solution has an odd coordinate with odd coefficient, the derivative of the ternary form
in that coordinate has valuation 1 and its value is 0 modulo 16, so strong Hensel lifts it. If both coefficients
have valuation 1, a primitive solution has odd `x,y` and even `z=2z'`; divide the equation by 2. The resulting
equation has value 0 modulo 8 and derivative valuation 1 in `x` or `y`, so strong Hensel lifts it. The remaining
case, an odd coordinate only at an even coefficient with all odd-coefficient coordinates even, contradicts the
equation modulo 4. (3) Enumerating `x,y,z mod 16` with at least one odd coordinate for the eight representatives
gives the displayed 64 signs. Substitution of each representative into Definition 4 gives the same table.
Square scaling and step (1) extend equality to every nonzero pair. Strong Hensel means: if
`v_p(F(x_0)) > 2 v_p(F'(x_0))`, a root exists in `Z_p`. [source pending: a local-fields text stating this form]
Higher powers of 2 than 16 suffice for the same reduced coefficients. Modulo 8 alone does not suffice:
`(2,6)_2=-1`, but `(x,y,z)=(1,1,0)` solves the equation modulo 8.

Check: `check_hilbert`. Used by: `docs/SPEC.md` 9.3.7, 2-adic Hilbert row and R2 oracle.

**Proposition 7 (Hilbert precision and exceptional places).** Let `p` be prime, `a,b` nonzero in `Q_p`, and
`A,B` integers. Let the local inputs be balls `a+p^A Z_p` and `b+p^B Z_p` that exclude zero. If
`A-v_p(a)>=1` and `B-v_p(b)>=1` for odd `p`, or both are at least 3 for
`p=2`, the symbol is determined by the centre formula. At infinity, known signs suffice. For ideles with exact
positive rational scales `r,s`, only infinity, 2, and odd primes where `v_p(r)` or `v_p(s)` is odd can have a
nontrivial symbol. At each candidate place return one sign only if it is constant on all permitted square
classes; otherwise return `{+1,-1}` or `NOT_DETERMINED`. The whole family is determined exactly when every
candidate is determined. Finding the candidate primes may require factoring `r,s` or a certified support list.

Proof. (1) Under the relative precision bounds, every element of a ball has the centre's valuation and unit
residue modulo `p`, or modulo 8 at 2. Apply Propositions 5 and 6. (2) The bounds are sufficient, not necessary:
the displayed formulas can ignore a missing unit digit if its coefficient parity vanishes. (3) An idele of scale
`r` has local valuation `v_p(r)` because its unit factor has valuation 0. If both valuations are even at odd `p`,
the odd-prime formula is 1 for all units. (4) At each remaining place finite precision gives a finite set of
square classes. Evaluating all allowed pairs gives exactly the stated possible signs. (5) A scale-1 unit coset
with no 2-adic unit digits permits `1` and `3`; `(1,1)_2=1` and `(3,3)_2=-1` by the table. Thus finite support
does not imply a determined family. Choosing both 2-components as 3, all other finite components as 1, and
positive real components gives a pair of ideles whose all-place product is -1. These are not diagonal rationals.

Check: `check_hilbert`. Used by: `docs/SPEC.md` 9.3.7, Hilbert precision and notes (R2).

**Proposition 8 (rational product formula).** For nonzero rationals `a,b`, the product of `(a,b)_v` over all
finite primes and infinity is 1. All but finitely many factors equal 1.

Proof. (1) The formulas are multiplicative in each argument modulo squares: Legendre is multiplicative, and
`eps(uw)=eps(u)+eps(w)` and `om(uw)=om(u)+om(w)` modulo 2 for odd units. Thus it suffices to check pairs of
generators `-1,2`, and odd positive primes of `Q^x/(Q^x)^2`. (2) For distinct odd primes `p,q`, the potentially
nontrivial factors are `(q/p)` at `p`, `(p/q)` at `q`, and `(-1)^(eps(p)eps(q))` at 2. Their product is 1 by
quadratic reciprocity. (3) For `(p,p)` or `(-1,p)`, the factors at `p` and 2 cancel by the supplementary law
`(-1/p)=(-1)^eps(p)`. For `(2,p)`, the factors at `p` and 2 cancel by
`(2/p)=(-1)^om(p)`. (4) `(2,2)` and `(-1,2)` have all factors 1; `(-1,-1)` has -1 at 2 and infinity.
This exhausts generators. Quadratic reciprocity and its supplementary laws are used exactly in steps (2)-(3).
[source pending: a local copy of a number-theory text stating those laws]

Check: `check_hilbert_product`. Used by: `docs/SPEC.md` 9.3.7, Hilbert product formula.

## Local factors and finite measures

**Proposition 9 (trivial local zeta factors).** For prime `p` and complex `s` with `Re(s)>0`, the finite local
factor is `sum_(j>=0) p^(-js) = (1-p^(-s))^-1`. Its meromorphic continuation has simple poles exactly at
`s=2 pi i k/log p`, `k in Z`. For complex `s` with `Re(s)>0`, the real local factor from the Gaussian integral
`2 integral_0^infinity exp(-pi x^2) x^(s-1) dx` is `pi^(-s/2) Gamma(s/2)`. Its meromorphic continuation has
simple poles exactly at `s=0,-2,-4,...`. A complex input ball containing any pole returns a pole/domain status
or an explicitly unbounded enclosure, never a finite complex ball.

Proof. (1) The finite factor is a convergent geometric series for `Re(s)>0`; its ratio is `p^-s` and its tail
after `J` terms is `p^(-Js)/(1-p^-s)`. The denominator vanishes precisely at the listed points, and its
derivative there is `log p`, so each pole is simple. (2) At infinity substitute `t=pi x^2`; the integral becomes
`pi^(-s/2) integral_0^infinity exp(-t)t^(s/2-1)dt`, the defining Gamma integral. (3) Gamma is meromorphic with
simple poles at the nonpositive integers and no zeros. [source pending: a complex-analysis text stating Gamma's
continuation and pole set] The factor `pi^(-s/2)` is entire and nonzero. (4) Near a pole, values are unbounded;
therefore no finite complex ball encloses the value set of an input ball containing that pole.

Check: `check_zeta`. Used by: `docs/SPEC.md` 9.3.7, local zeta row (R5).

**Proposition 10 (content and Haar volume).** Let an idele have exact positive rational scale `r` as in
`docs/SPEC.md` section 5. Its finite content is `r`; its finite fractional ideal is `r Z`, while its finite
multiplicative absolute value is `1/r`. Let finite additive Haar measure be normalised by
`vol(Zhat)=1`. For rational `N>0`, `vol(a+N Zhat)=1/N` for every rational `a`; a singleton has volume 0.

Proof. (1) At each finite prime the idele component is `r u_p` with `u_p` a unit, so its valuation is `v_p(r)`.
The corresponding fractional ideal is `product p^(v_p(r)) Z=r Z`; its positive generator is `r`. (2) The
local scaling rule is `vol_p(N E)=|N|_p vol_p(E)`. [source pending: a Haar-measure reference stating local
scaling] The product of `|N|_p` over finite primes is `1/N` by direct prime factorisation of a positive rational.
Translation preserves volume. (3) A singleton lies in balls with arbitrarily large integer radius and hence
volume tending to 0; monotonicity gives volume 0.

Check: `check_content_volume`. Used by: `docs/SPEC.md` section 5 and 9.3.7, content and Haar volume.

## Profinite powers and binomials

**Definition 11 (continuous profinite power).** For a profinite unit `u` and `x in Zhat`, define `u^x mod L`
for each integer `L>=1` by exponentiating `u mod L` with exponent `x mod ord_L(u)`. The values are compatible
as `L` varies and define one profinite unit. For exact negative integer `x`, this is the inverse of `u^(-x)`.

Proof. Each `(Z/L)^x` is finite, so the order exists. Reduction from `L` to a divisor preserves exponentiation
and its order divides the original order. Thus the residues are compatible. Integer exponents, positive and
negative, are dense in `Zhat`; this definition is their continuous extension.

Check: `check_power`. Used by: `docs/SPEC.md` 9.3.7, profinite power definition.

**Proposition 12 (target precision and coarsening).** Let `c U(N)` be a unit coset with `N>=1`, and let
`x in e+M Zhat`, `M>=1`, `e` any integer. The result `u^x mod N` is determined exactly when
`c^M=1 mod N`. Whether or not this holds, put `D=gcd(N,c^M-1)`. Then every result lies in `c^e U(D)`, and
`D` is the largest divisor of `N` on which the result is determined. A strict target-`N` interface returns
`NOT_DETERMINED` if `D<N`; an enclosure interface returns this coarser coset. For an exact signed integer
exponent `e`, the result is always determined modulo `N`, using the modular inverse for `e<0`; at `e=0` it is
the exact unit 1.

Proof. (1) Modulo `N`, every base is `c`, and exponents `e` and `e+M` give values `c^e` and `c^e c^M`.
Thus constancy is necessary and sufficient for `c^M=1 mod N`; sufficiency extends from integer exponents by
continuity in Definition 11. (2) The same argument at any divisor `L|N` gives constancy exactly when
`L|c^M-1`, equivalently `L|D`. This proves both enclosure and maximality among divisors of `N`. (3) If the
exponent is exact there is no exponent variation, and all bases agree modulo `N`; zero powers are identically
1 at every modulus. Inverse powers are well-defined because `gcd(c,N)=1`.

Check: `check_power`. Used by: `docs/SPEC.md` 9.3.7, power precision (R3).

**Proposition 13 (canonical modulus and unrestricted finest modulus).** For every odd `m>=1`,
`U(2m)=U(m)`; hence a modulus `N=2m` with `m` odd canonically reduces to `m`. Apply this reduction before
comparing unit cosets. Under Proposition 12 with canonical `N`, put `g=gcd(e,M)>0`, and let `v_p(0)=infinity`.
The largest integer modulus `F` on which all output powers are determined has these prime exponents:

| Prime | `v_p(F)` |
|---|---|
| `p|N` | `min(v_p(N)+v_p(g), v_p(c^M-1))` |
| odd `p` not dividing `N` | `1+v_p(g)` if `p-1|g`, else 0 |
| `p=2` not dividing `N` | 1 if `g` is odd, else `2+v_2(g)` |

In particular `D` need not be the finest modulus: for `N=5,c=2,e=0,M=2`, `D=1` but `F=24`.
For exact nonzero integer `e`, use the same table with `M=0,g=|e|` and `v_p(c^0-1)=infinity`.
For exact `e=0`, the output is exact 1 and has no largest finite modulus.

Proof. (1) Every profinite unit is 1 modulo 2. For odd `m`, the conditions modulo `m` and `2m` are therefore
the same on units. (2) At `p|N`, the base ranges over `c U(p^n)`, `n=v_p(N)`. Constancy of exponents requires
`c^M=1 mod p^k`. Constancy under base variation requires every member of `U(p^n)` raised to both `e` and `M`
to be 1 modulo `p^k`, equivalently every member raised to `g` is 1. The principal-unit logarithm gives the
depth `n+v_p(g)` for odd `p`, and for `p=2` because canonical even `N` has `n>=2`. The standard theorem used
here states: for odd `p,n>=1`, or `p=2,n>=2`, `log` maps `1+p^n Z_p` isomorphically to `p^n Z_p`, with
`v_p(log u)=v_p(u-1)` for `u != 1`. [source pending: a local-fields text stating this logarithm theorem]
Combining the two bounds gives the first row. (3) At `p` outside `N`, the base ranges over
all `Z_p^x`. Since the base 1 is allowed, constancy holds exactly when the exponent of
`(Z/p^k)^x` divides both `e` and `M`, hence `g`. For odd `p` that exponent is `(p-1)p^(k-1)`. At 2 it is 1
for `k=1`, 2 for `k=2`, and `2^(k-2)` for `k>=3`. [source pending: a finite-unit-group reference stating
these exponents] This gives the other rows. (4) Only primes dividing `N` or satisfying `p<=g+1` occur, so `F`
is finite. For the example the table gives `v_2(F)=3`, `v_3(F)=1`, and all other exponents 0. (5) Exact signed
exponents use the same base-variation argument with `M=0`; `e=0` gives the constant unit 1.
(6) An independent finite algorithm takes `B=8 N g (g+1)!`, which is divisible by `F` by the table.
Enumerate unit residues `b mod B` with `b=c mod N`, choose one `b_0`, and take the gcd of `B`, all
`b^e-b_0^e`, and all `b^M-1`, using modular powers for negative `e`. The result is `F`: the first differences
test base variation, and the second test exponent variation. The code compares this enumeration with the
prime-exponent table on small inputs.

Check: `check_power`. Used by: `docs/SPEC.md` section 5 and 9.3.7, canonical and power rules (R3).

**Proposition 14 (binomial enclosure and smallest ball).** Let `k>=0`, `a` any integer, and `N>=1` an integer.
The polynomial `binom(x,k)=x(x-1)...(x-k+1)/k!` maps `Zhat` into `Zhat`. On `a+N Zhat`, its values lie in the
ball with centre `binom(a,k)` and conservative radius `N/gcd(N,k!)`. Its smallest enclosing additive ball has
the same centre and radius

    R = gcd(binom(a+Nj,k)-binom(a,k) : j=1,...,k).

For `k=0`, the empty gcd is 0 and the value is the exact 1. At `(a,N,k)=(0,8,4)`, `R=2`.

Proof. (1) For every nonnegative integer `x`, `binom(x,k)` is integral. Those integers are dense in every
`Z_p`; polynomial continuity implies the integer-valued polynomial maps `Z_p` to itself. (2) The falling
factorial numerator changes by a multiple of `N` when `x` changes by `Nt`. Division by `k!` gives a
congruence modulo `N/gcd(N,k!)`: at each prime the difference is integral and has valuation at least
`v_p(N)-v_p(k!)`, hence at least `max(0,v_p(N)-v_p(k!))`. (3) Write
`h(t)=binom(a+Nt,k)`. Its Newton expansion is `h(t)=sum_(j=0)^k Delta^j h(0) binom(t,j)`. All coefficients
are integers. For `j>=1`, `Delta^j h(0)` is an integer combination of `h(i)-h(0)`, `1<=i<=j`, by the
finite-difference formula; conversely `h(j)-h(0)=sum_(i=1)^j binom(j,i) Delta^i h(0)`. Thus the gcd of the
nonconstant coefficients equals the gcd of `h(j)-h(0)` for `1<=j<=k`. This gcd divides every difference at
integral `t`, then at all
profinite `t` by continuity. Conversely those `k` differences occur in the input ball, so every enclosing
radius divides their gcd. (4) At `k=0` the polynomial is identically 1. Direct evaluation at `j=1,...,4`
gives gcd 2 for the stated example.

Check: `check_binomial`. Used by: `docs/SPEC.md` 9.3.7, binomial row.

## Cyclotomic action

**Proposition 15 (the two conventions and their precision).** Let an idele class of `Q` have coordinates
`(t,u') in R_{>0} x Zhat^x` as in `docs/SPEC.md` section 5, and let `zeta_n` have exact order `n>=1`.
The arithmetic convention sends `zeta_n` to `zeta_n^(u'^-1 mod n)`; the geometric convention sends it to
`zeta_n^(u' mod n)`. The positive real coordinate acts trivially. For `u' in c U(N)`, `N>=1`, either action
on all `n`th roots is determined exactly when `U(N) subset U(n)`, equivalently `canon(n)|canon(N)` where
`canon(2m)=m` for odd `m` and `canon(j)=j` otherwise. If not determined, return the possible automorphisms
or `NOT_DETERMINED`. The class of the idele with component `p` at one prime `p`, and component 1 elsewhere,
has unit coordinate `p^-1` away from `p` and 1 at `p`. For `gcd(p,n)=1`, arithmetic action is
`zeta_n -> zeta_n^p`; on roots of `p`-power order this idele acts trivially.

Proof. (1) The two exponent rules are the two conventions specified in the catalogue. Their attribution to
external reciprocity sources remains pending; this proposition proves their consequences as definitions.
(2) A connected group has no nonconstant continuous map to the finite group `(Z/n)^x`, so `t` acts trivially.
(3) Reduction of `c U(N)` modulo `n` is a singleton exactly when `U(N) subset U(n)`. Inversion is a bijection,
so both conventions have the same precision. Since `U(2m)=U(m)` for odd `m` by Proposition 13, containment is
equivalent to divisibility after canonicalising both moduli. Thus `[2 mod 3]` determines action on sixth roots.
(4) For the test idele, take scale `r=p`. The finite unit is 1 at `p` and `p^-1` at every other prime.
When `p` is invertible modulo `n`, its reduction is `p^-1`, whose inverse is `p`. At `p` the unit is 1, so its
action on `p`-power roots is trivial. The coprimality guard is essential: `zeta_p -> zeta_p^p` would not be an
automorphism. [source pending: local copies of reciprocity references fixing the arithmetic and geometric
names for these two exponent conventions]

Check: `check_cyclotomic`. Used by: `docs/SPEC.md` section 5 and 9.3.7, cyclotomic row (R5/C1).

## Statement index

| No. | Content | Status | Check |
|---:|---|---|---|
| 1 | Residue symbol definitions | definitions; convention source pending | `check_symbols` |
| 2 | Symbol precision and `(1/2)`, `(3/2)` | proved here | `check_symbols` |
| 3 | Exact symbol set on unit cosets and additive balls | proved here | `check_symbols` |
| 4 | Hilbert definition and displayed formulas | proved by 5-6; theorem pending | `check_hilbert` |
| 5 | Odd-prime formula equals solvability | proved modulo simple Hensel | `check_hilbert` |
| 6 | 2-adic formula and 64 class pairs | proved modulo strong Hensel | `check_hilbert` |
| 7 | Hilbert precision, exceptional places, ambiguity | proved here | `check_hilbert` |
| 8 | Rational product formula | proved modulo quadratic reciprocity and supplements | `check_hilbert_product` |
| 9 | Local trivial factors, poles, pole status | proved modulo Gamma continuation | `check_zeta` |
| 10 | Idele content and Haar volume | proved modulo local Haar scaling | `check_content_volume` |
| 11 | Continuous profinite power | proved here | `check_power` |
| 12 | Power criterion and largest divisor of `N` | proved here | `check_power` |
| 13 | Canonical and unrestricted finest modulus | proved modulo local unit-group theorems | `check_power` |
| 14 | Binomial conservative and smallest balls | proved here | `check_binomial` |
| 15 | Cyclotomic conventions, precision, vector | consequences proved; attribution pending | `check_cyclotomic` |

No statement is left open as a mathematical implication. The external convention attributions and named standard
theorems remain source obligations; their pending status is not evidence that the cited texts have been checked.
