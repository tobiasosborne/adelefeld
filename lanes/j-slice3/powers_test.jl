using Test, Adelefeld
@testset "all integer literal powers are refused" begin
    for T in (FBall, Adele, CAdele)
        x = T(1)
        for f in (() -> x^0, () -> x^1, () -> x^2, () -> x^-1, () -> x^-2)
            e = try; f(); catch err; err; end
            @test e isa ArgumentError && occursin("pow", sprint(showerror, e))
        end
        close(x)
    end
end
