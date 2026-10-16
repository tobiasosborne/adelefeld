# lanes/docs-julia/exports.jl: every exported name of Adelefeld with the first prose line of its docstring,
# for the API table of julia/Adelefeld/README.md. "(none)" when the name has no docstring of its own.
using Adelefeld, Markdown
names_ = sort([n for n in names(Adelefeld) if n !== :Adelefeld]; by = string)
println("exported: ", length(names_))
for n in names_
    b = Docs.Binding(Adelefeld, n)
    first_line = "(none)"
    if haskey(Docs.meta(Adelefeld), b)
        md = Docs.meta(Adelefeld)[b]
        for sig in md.order
            d = md.docs[sig]
            txt = join(d.text, "")
            for l in split(txt, "\n")
                s = strip(l)
                (isempty(s) || startswith(l, "    ")) && continue
                first_line = s; break
            end
            break
        end
    end
    println(n, "\t", first_line)
end
