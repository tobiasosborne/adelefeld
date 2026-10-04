# WP 1F.9: hand values from catalogue.md Definition 1, not a second call as oracle.
using Test, Libdl
const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
ad(n) = Libdl.dlsym(LIB,n)
fl(n) = Libdl.dlsym(FLINT,n)
struct Place
    opaque::UInt64
end
function place(p)
    v=Ref(Place(0))
    @test ccall(ad(:adf_place_prime),Cint,(Ref{Place},Culong),v,p)==0
    v[]
end
function integer(n)
    a=Ref{Clong}(0)
    ccall(fl(:fmpz_init),Cvoid,(Ref{Clong},),a)
    @test ccall(fl(:fmpz_set_str),Cint,(Ref{Clong},Cstring,Cint),a,string(n),10)==0
    a
end
function object(kind,text)
    p=Ptr{UInt8}(Libc.malloc(ccall(ad(Symbol(:adf_sizeof_,kind)),Csize_t,())))
    ccall(ad(Symbol(:adf_,kind,:_init)),Cvoid,(Ptr{UInt8},),p)
    if kind==:idele
        @test ccall(ad(:adf_idele_set_str),Cint,
                    (Ptr{UInt8},Cstring,Csize_t,Clong,Ptr{Cvoid}),p,text,sizeof(text),64,C_NULL)==0
    else
        @test ccall(ad(Symbol(:adf_,kind,:_set_str)),Cint,
                    (Ptr{UInt8},Cstring,Csize_t,Ptr{Cvoid}),p,text,sizeof(text),C_NULL)==0
    end
    p
end

function release(kind,p)
    ccall(ad(Symbol(:adf_,kind,:_clear)),Cvoid,(Ptr{UInt8},),p)
    Libc.free(p)
end
@testset "Hilbert symbols" begin
    z,w=Ref{Cint}(99),Ref(place(97))
    a,b=object(:rat,"3"),object(:rat,"3")
    @test ccall(ad(:adf_rat_hilbert_at),Cint,
                (Ref{Cint},Ref{Place},Ptr{UInt8},Ptr{UInt8},Place),z,w,a,b,place(2))==0 && z[]==-1
    @test w[]==place(97)
    release(:rat,a); release(:rat,b)
    a,b=object(:lball,"[p=2: 1 + O(2^2)]"),object(:lball,"[p=2: 2]")
    z[]=99
    @test ccall(ad(:adf_lball_hilbert),Cint,
                (Ref{Cint},Ref{Place},Ptr{UInt8},Ptr{UInt8}),z,w,a,b)==1 && z[]==99
    @test w[]==place(2)
    release(:lball,a); release(:lball,b)
    a,b=object(:idele,"(1 ; 3 * [1])"),object(:idele,"(1 ; 3 * [1])")
    @test ccall(ad(:adf_idele_hilbert_at),Cint,
                (Ref{Cint},Ref{Place},Ptr{UInt8},Ptr{UInt8},Place),z,w,a,b,place(2))==0 && z[]==-1
    release(:idele,a); release(:idele,b)
    # arb is 48 bytes on this binding target (idele.h), initialized and released by FLINT.
    a,b=Ptr{UInt8}(Libc.malloc(48)),Ptr{UInt8}(Libc.malloc(48))
    for x in (a,b)
        ccall(fl(:arb_init),Cvoid,(Ptr{UInt8},),x)
        ccall(fl(:arb_set_si),Cvoid,(Ptr{UInt8},Clong),x,-1)
    end
    w[]=place(97)
    @test ccall(ad(:adf_real_hilbert),Cint,
                (Ref{Cint},Ref{Place},Ptr{UInt8},Ptr{UInt8}),z,w,a,b)==0 && z[]==-1
    @test w[]==place(97)
    for x in (a,b)
        ccall(fl(:arb_clear),Cvoid,(Ptr{UInt8},),x); Libc.free(x)
    end
end
@testset "residue symbols" begin
    a,b=integer(3),integer(2)
    z,w=Ref{Cint}(99),Ref(place(97))
    @test ccall(ad(:adf_fmpz_kronecker),Cint,
                (Ref{Cint},Ref{Place},Ref{Clong},Ref{Clong}),z,w,a,b)==0 && z[]==-1
    @test w[]==place(97)
    ccall(fl(:fmpz_set_si),Cvoid,(Ref{Clong},Clong),a,2)
    ccall(fl(:fmpz_set_si),Cvoid,(Ref{Clong},Clong),b,15)
    @test ccall(ad(:adf_fmpz_jacobi),Cint,
                (Ref{Cint},Ref{Place},Ref{Clong},Ref{Clong}),z,w,a,b)==0 && z[]==1
    @test ccall(ad(:adf_fmpz_legendre),Cint,
                (Ref{Cint},Ref{Place},Ref{Clong},Place),z,w,a,place(7))==0 && z[]==1
    for kind in (:fball,:ucoset)
        f=object(kind,kind==:fball ? "3 mod 8" : "[3 mod 8]")
        ccall(fl(:fmpz_set_si),Cvoid,(Ref{Clong},Clong),b,2)
        @test ccall(ad(Symbol(:adf_,kind,:_kronecker)),Cint,
                    (Ref{Cint},Ref{Place},Ptr{UInt8},Ref{Clong}),z,w,f,b)==0 && z[]==-1
        ccall(ad(Symbol(:adf_,kind,:_clear)),Cvoid,(Ptr{UInt8},),f); Libc.free(f)
        f=object(kind,kind==:fball ? "2 mod 15" : "[2 mod 15]")
        ccall(fl(:fmpz_set_si),Cvoid,(Ref{Clong},Clong),b,15)
        @test ccall(ad(Symbol(:adf_,kind,:_jacobi)),Cint,
                    (Ref{Cint},Ref{Place},Ptr{UInt8},Ref{Clong}),z,w,f,b)==0 && z[]==1
        @test ccall(ad(Symbol(:adf_,kind,:_legendre)),Cint,
                    (Ref{Cint},Ref{Place},Ptr{UInt8},Place),z,w,f,place(3))==0 && z[]==-1
        ccall(ad(Symbol(:adf_,kind,:_clear)),Cvoid,(Ptr{UInt8},),f); Libc.free(f)
    end
    f=object(:fball,"1 mod 4")
    ccall(fl(:fmpz_set_si),Cvoid,(Ref{Clong},Clong),b,2)
    z[]=99
    @test ccall(ad(:adf_fball_kronecker),Cint,
                (Ref{Cint},Ref{Place},Ptr{UInt8},Ref{Clong}),z,w,f,b)==1 && z[]==99
    ccall(ad(:adf_fball_clear),Cvoid,(Ptr{UInt8},),f); Libc.free(f)
    for x in (a,b) ccall(fl(:fmpz_clear),Cvoid,(Ref{Clong},),x) end
end
