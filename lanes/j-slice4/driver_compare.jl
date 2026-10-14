# Emit the same five dump and six reconstruction lines as driver.cmd.
using Adelefeld
for s in ("7/3", "(* ; 2 mod 6)", "(1 ; 0)", "((1) + (0.5)*i ; 2 mod 6)", "(0.5 ; 0)")
    x = readvalue(s)
    try
        println(dump_text(x))
    finally
        close(x)
    end
end
for (s, l, h) in (("146 mod 1000", 0, 50), ("146 mod 1000", 100, 200))
    x = parse(FBall, s); lo = Rat(l); hi = Rat(h)
    r = try_reconstruct(x, lo, hi)
    if r.status === Adelefeld.OK
        println(r.value); close(r.value)
    else
        println("error: ", r.status)
    end
    close.((x, lo, hi))
end
for s in ("(2 ; 2 mod 1)", "(1 ; 1)", "(0 ; 3 mod 12)")
    x = parse(Adele, s)
    r = try_reconstruct(x)
    if r.status === Adelefeld.OK
        println(r.value); close(r.value)
    else
        println("error: ", r.status)
    end
    close(x)
end
x = parse(FBall, "1 mod 2"); lo = Rat(0); hi = Rat(10)
r = try_reconstruct(x, lo, hi)
println("error: ", r.status)
close.((x, lo, hi))
