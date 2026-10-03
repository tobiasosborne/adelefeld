# Idele Log, IL1-IL5. Only exported lifecycle/accessors and Libdl/Test.
using Libdl, Test
const LIB = Libdl.dlopen(ARGS[1])
ad(s) = Libdl.dlsym(LIB,s)
struct Place
    opaque::UInt64
end
function place(p)
    v=Ref(Place(0))
    @test ccall(ad(:adf_place_prime),Cint,(Ref{Place},Culong),v,p)==0
    v[]
end
const INF=ccall(ad(:adf_place_inf),Place,())
function obj(kind)
    p=Libc.malloc(ccall(ad(Symbol(:adf_sizeof_,kind)),Csize_t,()))
    ccall(ad(Symbol(:adf_,kind,:_init)),Cvoid,(Ptr{Cvoid},),p)
    p
end
function text(kind,p)
    n=Ref{Csize_t}(0)
    s=ccall(ad(Symbol(:adf_,kind,:_get_str)),Ptr{UInt8},(Ref{Csize_t},Ptr{Cvoid}),n,p)
    s==C_NULL && error("printer")
    try unsafe_string(s,n[]) finally ccall(ad(:adf_str_free),Cvoid,(Ptr{UInt8},),s) end
end
function parse!(x,t)
    @test ccall(ad(:adf_idele_set_str),Cint,(Ptr{Cvoid},Cstring,Csize_t,Clong,Ptr{Cvoid}),
                x,t,sizeof(t),128,C_NULL)==0
end
function global_log!(y,x,absval=false)
    w=Ref(place(97))
    @test ccall(ad(absval ? :adf_idele_log_abs : :adf_idele_Log),Cint,
                (Ptr{Cvoid},Ref{Place},Ptr{Cvoid},Clong),y,w,x,128)==0
    @test w[].opaque==place(97).opaque
end
function at!(s,x,p,N=5,absval=false)
    w=Ref(place(97))
    @test ccall(ad(absval ? :adf_idele_log_abs_at : :adf_idele_Log_at),Cint,
                (Ptr{Cvoid},Ref{Place},Ptr{Cvoid},Place,Clong),s,w,x,p,N)==0
    @test w[].opaque==place(97).opaque
end
x,y,s,f,l,r= obj(:idele),obj(:adele),obj(:sball),obj(:fball),obj(:lball),obj(:adele)
try
    @testset "idele Log slice A" begin
        parse!(x,"(2 ; 4 * [5 mod 6])")
        global_log!(y,x)
        ccall(ad(:adf_adele_get_fin),Cvoid,(Ptr{Cvoid},Ptr{Cvoid}),f,y)
        @test text(:fball,f)=="(* ; 0 mod 4)"
        # The real coordinate is independent from content 4. Check log(2) at 512 bits.
        len=Ref{Csize_t}(0)
        t=ccall(ad(:adf_adele_get_str),Ptr{UInt8},(Ref{Csize_t},Ptr{Cvoid},Clong),len,y,30)
        st=unsafe_string(t,len[])
        ccall(ad(:adf_str_free),Cvoid,(Ptr{UInt8},),t)
        a=split(st[2:end])
        setprecision(BigFloat,512) do
            @test abs(parse(BigFloat,a[1])-log(BigFloat(2)))<=parse(BigFloat,a[3])
        end
        println("Log (2 ; 4 * [5 mod 6]): ",st)
        for (p,E) in ((2,2),(3,1),(5,1))
            v=place(p); at!(s,x,v)
            @test ccall(ad(:adf_sball_get_lball),Cint,(Ptr{Cvoid},Ptr{Cvoid},Place),l,s,v)==0
            @test text(:lball,l)=="[p=$p: 0 + O($p^$E)]"
            println("Log at $p: ",text(:lball,l))
        end
        # At unrestricted 5 the content contributes Log(4)=45 mod 5^3. Translating
        # 5 Z_5 by this value leaves 5 Z_5. At restricted 3 with M=9 it survives.
        parse!(x,"(2 ; 4 * [1])"); at!(s,x,place(5),3)
        @test ccall(ad(:adf_sball_get_lball),Cint,(Ptr{Cvoid},Ptr{Cvoid},Place),l,s,place(5))==0
        @test text(:lball,l)=="[p=5: 45 + O(5^3)]"
        parse!(x,"(2 ; 4 * [1 mod 9])"); at!(s,x,place(3))
        @test ccall(ad(:adf_sball_get_lball),Cint,(Ptr{Cvoid},Ptr{Cvoid},Place),l,s,place(3))==0
        @test text(:lball,l)=="[p=3: 3 + O(3^2)]"
        at!(s,x,INF,128)
        parse!(x,"(-2 ; 4 * [5 mod 6])"); global_log!(y,x,true); at!(s,x,INF,128,true)
    end
    @testset "idele Log slice B" begin
        parse!(x,"(2 ; 4 * [5 mod 6])")
        ps=[place(5),place(3),place(2)]
        w=Ref(place(97))
        @test ccall(ad(:adf_idele_Log_refine),Cint,
                    (Ptr{Cvoid},Ref{Place},Ptr{Cvoid},Ptr{Place},Clong,Clong,Clong),
                    y,w,x,ps,3,5,128)==0
        ccall(ad(:adf_adele_get_fin),Cvoid,(Ptr{Cvoid},Ptr{Cvoid}),f,y)
        @test text(:fball,f)=="(* ; 0 mod 60)"
        @test w[].opaque==place(97).opaque
        println("Log refined at 2,3,5 for modulus 6: ",text(:fball,f))
        parse!(x,"(2 ; 4 * [1 mod 9])")
        ps=[place(3)]
        @test ccall(ad(:adf_idele_Log_refine),Cint,
                    (Ptr{Cvoid},Ref{Place},Ptr{Cvoid},Ptr{Place},Clong,Clong,Clong),
                    y,w,x,ps,1,5,128)==0
        ccall(ad(:adf_adele_get_fin),Cvoid,(Ptr{Cvoid},Ptr{Cvoid}),f,y)
        @test text(:fball,f)=="(* ; 12 mod 36)"
        println("Log refined at 3 with content 4, modulus 9: ",text(:fball,f))
        parse!(x,"(-2 ; 4 * [1 mod 9])")
        @test ccall(ad(:adf_idele_log_abs_refine),Cint,
                    (Ptr{Cvoid},Ref{Place},Ptr{Cvoid},Ptr{Place},Clong,Clong,Clong),
                    y,w,x,ps,1,5,128)==0
        ccall(ad(:adf_adele_get_fin),Cvoid,(Ptr{Cvoid},Ptr{Cvoid}),f,y)
        @test text(:fball,f)=="(* ; 12 mod 36)"
        # n=0 can borrow NULL, and is identical to the conservative all-places result.
        global_log!(r,x,true)
        @test ccall(ad(:adf_idele_log_abs_refine),Cint,
                    (Ptr{Cvoid},Ref{Place},Ptr{Cvoid},Ptr{Place},Clong,Clong,Clong),
                    y,w,x,C_NULL,0,-2,128)==0
        @test ccall(ad(:adf_adele_identical),Cint,(Ptr{Cvoid},Ptr{Cvoid}),y,r)==1
    end
finally
    for (p,kind) in ((x,:idele),(y,:adele),(s,:sball),(f,:fball),(l,:lball),(r,:adele))
        ccall(ad(Symbol(:adf_,kind,:_clear)),Cvoid,(Ptr{Cvoid},),p)
        Libc.free(p)
    end
end
