| `AdfStatus` * | the eleven status codes, `OK = 0` to `LIMIT = 10`, as in `status.h:17-28` (`status.jl`) |
| `AdfResult` | the result of a `try_` method: `status` always, `value` only when the header certifies it |
| `AdfError` | thrown by every throwing form on a status other than OK; carries the whole result |
| `AdfAbiError` | a C function returned an int that is not a status; the raw value is kept |
| `AdfLoadError` | a load gate failed: the gate, the expected and the observed value, the library path, a hint |
| `unwrap` | returns `r.value` on OK; throws `AdfError` on every other status |
| `TextLimits` | the limits of a parser (length, decimal exponent, precision, items); defaults from C |
| `Rat` * | the exact global rational, a handle on C storage (`rat.jl`; its docstring sits on `_Raw`) |
| `try_rat` | a `Rat` from integers `n, d` or a `Rational`; `d = 0` gives DOMAIN |
| `try_parse` | reads the text of a type with its C parser; the parser's status is kept |
| `try_value_text` | the value form of `x` (the text `show` prints), as an `AdfResult{String}` |
| `value_text` * | the throwing form of `try_value_text`; `digits` for types with a real part (`adele.jl`) |
| `try_add` * | the sum as an `AdfResult`; `prec` for adelic types (`rat.jl`, `fball.jl`, `adele.jl`) |
| `try_sub` * | the difference as an `AdfResult`; `prec` for adelic types |
| `try_mul` * | the product as an `AdfResult`; a `Rat` times a ball scales it (`fball.h:219-226`) |
| `try_neg` * | the negative as an `AdfResult` |
| `try_div` | division by an exact `Rat` only: OK, or NOT_UNIT when the divisor is 0 |
| `try_inv` | the inverse of a `Rat`: OK, or NOT_UNIT when it is 0 |
| `add` * | the throwing form of `try_add`; also `+` |
| `sub` * | the throwing form of `try_sub`; also `-` |
| `mul` * | the throwing form of `try_mul`; also `*` |
| `neg` * | the throwing form of `try_neg`; also unary `-` |
| `divexact` * | the throwing form of `try_div`; also `/` |
| `identical` | the same stored representation (for `Rat` the same as `==`); not equality of sets |
| `FBall` | the finite ball, the set `a + N Zhat` of finite adeles (SPEC 4.1) |
| `try_fball` | the ball `c + N Zhat`, global and canonical; DOMAIN when `N < 0` |
| `PointComparison` * | `EQUAL`, `DIFFERENT`, `UNDECIDED`: the comparison of two unknown points (`status.jl`) |
| `equal_set` | the two sets are equal; a Bool, never a status |
| `overlaps` | the two sets meet; not transitive (SPEC 4.2) |
| `inside` | true when the set `x` is inside the set `y`: first inside second |
| `compare_points` | the comparison of two unknown points, one in each ball |
| `center` | the stored centre `a = A/d`, in `[0, N)` for `N > 0` |
| `radius` | the radius `N = H/d >= 0` in lowest terms, 0 for an exact value |
| `is_canonical` | the C canonical predicate holds; a Bool, never a status |
| `Adele` * | real ball at the real place and finite ball at the primes (`adele.jl`; SPEC 4.1) |
| `CAdele` * | complex ball at the real place and finite ball at the primes, the ring `C x A_f` (`adele.jl`) |
| `RealBall` | a numerical real ball; not an adele |
| `ComplexBall` | a numerical complex ball; not an adele |
| `finitepart` * | a fresh copy of the finite ball of an `Adele` or `CAdele` (`adele.jl`) |
| `realpart` * | a fresh `RealBall`, the real coordinate of an `Adele` (`carrier.jl`) |
| `complexpart` * | a fresh `ComplexBall`, the complex coordinate of a `CAdele` (`carrier.jl`) |
| `TextKind` * | the thirteen kinds of value text, `TEXT_RAT` to `TEXT_CHAR`, of `text.h:118-135` (`status.jl`) |
| `try_readvalue` | classifies an untyped text, then reads it with the parser of its kind |
| `readvalue` * | the throwing form of `try_readvalue` (`text.jl`) |
| `try_dump_text` | the lossless dump of `x`, copied from the C dumper |
| `dump_text` * | the throwing form of `try_dump_text` (`dump.jl`) |
| `try_load_dump` | the strict typed dump loader; the whole input is validated first |
| `load_dump` * | the throwing form of `try_load_dump` (`dump.jl`) |
| `try_reconstruct` | the single rational in an interval and a finite ball, or NO_SOLUTION, or NOT_UNIQUE |
| `reconstruct` * | the throwing form of `try_reconstruct` (`recon.jl`) |
| `ModCtx` | a modulus context: an immutable C context made by one of the explicit constructors |
| `try_modctx` | a new context from exactly one construction family |
| `modulus` | the modulus `K` of the context, as an exact integer `Rat` |
| `nblocks` | the number `k >= 0` of word blocks |
| `blocks` | the blocks `q_1, ..., q_k` in their order |
| `try_to_local` | the set of `x` as a local value of `ctx`, exactly; DOMAIN when it is not one |
| `to_local` * | the throwing form of `try_to_local` (`modctx.jl`) |
| `try_enclose_local` | the best enclosure of `x` among the local values of `ctx`, with `details.lost` |
| `enclose_local` * | the throwing form of `try_enclose_local` (`modctx.jl`) |
| `try_to_global` | `x` in the global backend; the set is unchanged |
| `to_global` * | the throwing form of `try_to_global` (`modctx.jl`) |
| `is_local` | the value is stored in the local backend; not part of the value text |
| `context` | the context a local value borrows, `nothing` for a global value |
