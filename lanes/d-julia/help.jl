# Record installed help, not a claim about a Julia version from memory.
using Libdl, REPL
println("Julia ", VERSION, "; installed ?help text, rendered by Base.Docs.doc")
for (name, owner, symbol) in [
    ("?finalizer", Base, :finalizer),
    ("?finalize", Base, :finalize),
    ("?GC.@preserve", GC, Symbol("@preserve")),
    ("?GC.gc", GC, :gc),
    ("?Libc.malloc", Libc, :malloc),
    ("?Libc.free", Libc, :free),
    ("?Libdl.dlopen", Libdl, :dlopen),
    ("?Libdl.dlsym", Libdl, :dlsym),
    ("?unsafe_string", Base, :unsafe_string),
    ("?Base.get_extension", Base, :get_extension),
]
    println("\n", name)
    show(stdout, MIME"text/plain"(), Base.Docs.doc(Base.Docs.Binding(owner, symbol)))
    println()
end
