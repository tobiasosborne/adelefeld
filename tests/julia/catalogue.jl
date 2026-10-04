# WP 1F.9 catalogue calls. Hand values, from catalogue Propositions 10 to 15.
using Test, Libdl
const FLINT = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
const LIB = Libdl.dlopen(ARGS[1])
ad(n) = Libdl.dlsym(LIB,n)
fl(n) = Libdl.dlsym(FLINT,n)
function object(kind,text)
    p=Ptr{UInt8}(Libc.malloc(ccall(ad(Symbol(:adf_sizeof_,kind)),Csize_t,())))
    ccall(ad(Symbol(:adf_,kind,:_init)),Cvoid,(Ptr{UInt8},),p)
    if kind==:idclass
        @test ccall(ad(:adf_idclass_set_str),Cint,
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
function value(kind,p)
    n=Ref{Csize_t}(0)
    s=ccall(ad(Symbol(:adf_,kind,:_get_str)),Ptr{UInt8},(Ref{Csize_t},Ptr{UInt8}),n,p)
    text=unsafe_string(s,n[])
    ccall(ad(:adf_str_free),Cvoid,(Ptr{UInt8},),s)
    text
end
@testset "binomial" begin
    x=object(:fball,"0 mod 8"); y=object(:fball,"(* ; 99)")
    @test ccall(ad(:adf_fball_binom_tight),Cint,
                (Ptr{UInt8},Ptr{Cvoid},Ptr{UInt8},Culong),y,C_NULL,x,4)==0
    @test value(:fball,y)=="(* ; 0 mod 2)"
    @test ccall(ad(:adf_fball_binom),Cint,
                (Ptr{UInt8},Ptr{Cvoid},Ptr{UInt8},Culong),x,C_NULL,x,0)==0
    @test value(:fball,x)=="(* ; 1)"
    @test ccall(ad(:adf_fball_binom_tight),Cint,
                (Ptr{UInt8},Ptr{Cvoid},Ptr{UInt8},Culong),x,C_NULL,x,257)==10
    @test value(:fball,x)=="(* ; 1)"
    release(:fball,x); release(:fball,y)
end

@testset "profinite power" begin
    a=object(:ucoset,"[2 mod 5]"); x=object(:fball,"2 mod 4"); y=object(:ucoset,"[-1]")
    @test ccall(ad(:adf_ucoset_profpow_fine),Cint,
                (Ptr{UInt8},Ptr{Cvoid},Ptr{UInt8},Ptr{UInt8}),y,C_NULL,a,x)==0
    @test value(:ucoset,y)=="[49 mod 120]"
    release(:fball,x); x=object(:fball,"0 mod 2")
    @test ccall(ad(:adf_ucoset_profpow),Cint,
                (Ptr{UInt8},Ptr{Cvoid},Ptr{UInt8},Ptr{UInt8}),y,C_NULL,a,x)==1
    @test value(:ucoset,y)=="[49 mod 120]"
    @test ccall(ad(:adf_ucoset_profpow_coarse),Cint,
                (Ptr{UInt8},Ptr{Cvoid},Ptr{UInt8},Ptr{UInt8}),a,C_NULL,a,x)==0
    @test value(:ucoset,a)=="[1 mod 1]"
    release(:ucoset,a); release(:ucoset,y); release(:fball,x)
end
@testset "Haar volume" begin
    x=object(:fball,"1/2 mod 8/3"); q=object(:rat,"99")
    ccall(ad(:adf_fball_haar_volume),Cvoid,(Ptr{UInt8},Ptr{UInt8}),q,x)
    @test value(:rat,q)=="3/8"
    release(:fball,x); x=object(:fball,"(* ; 1/2)")
    ccall(ad(:adf_fball_haar_volume),Cvoid,(Ptr{UInt8},Ptr{UInt8}),q,x)
    @test value(:rat,q)=="0"
    release(:fball,x); release(:rat,q)
end
@testset "cyclotomic exponents" begin
    x=object(:idclass,"<17 ; [5 mod 7]>")
    n=Ref{Clong}(0); j=Ref{Clong}(0)
    ccall(fl(:fmpz_init),Cvoid,(Ref{Clong},),n)
    ccall(fl(:fmpz_init),Cvoid,(Ref{Clong},),j)
    ccall(fl(:fmpz_set_ui),Cvoid,(Ref{Clong},Culong),n,7)
    @test ccall(ad(:adf_idclass_cyclo_exp_uinv),Cint,
                (Ref{Clong},Ptr{Cvoid},Ptr{UInt8},Ref{Clong}),j,C_NULL,x,n)==0
    @test ccall(fl(:fmpz_get_si),Clong,(Ref{Clong},),j)==3
    @test ccall(ad(:adf_idclass_cyclo_exp_u),Cint,
                (Ref{Clong},Ptr{Cvoid},Ptr{UInt8},Ref{Clong}),j,C_NULL,x,n)==0
    @test ccall(fl(:fmpz_get_si),Clong,(Ref{Clong},),j)==5
    ccall(fl(:fmpz_set_ui),Cvoid,(Ref{Clong},Culong),n,49)
    @test ccall(ad(:adf_idclass_cyclo_exp_u),Cint,
                (Ref{Clong},Ptr{Cvoid},Ptr{UInt8},Ref{Clong}),j,C_NULL,x,n)==1
    @test ccall(fl(:fmpz_get_si),Clong,(Ref{Clong},),j)==5
    ccall(fl(:fmpz_clear),Cvoid,(Ref{Clong},),n)
    ccall(fl(:fmpz_clear),Cvoid,(Ref{Clong},),j)
    release(:idclass,x)
end
