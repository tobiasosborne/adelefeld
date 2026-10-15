# The J-b.1 session line (brief): a local value of the context of blocks [8, 9], printed.
using Adelefeld
ctx = ModCtx(blocks=[8, 9]); y = to_local(FBall(Rat(5), Rat(72)), ctx)
display(y); println()
println("ctx: ", ctx)
println("is_local=", is_local(y), " context(y)===ctx: ", context(y) === ctx, " modulus=", modulus(ctx))
println("to_global: ", to_global(y), " identical to the global input: ",
        identical(to_global(y), FBall(Rat(5), Rat(72))))
