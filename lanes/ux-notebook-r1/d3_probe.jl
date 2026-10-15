push!(LOAD_PATH, "ux/notebook/src"); using Adelefeld, Adelica
m = Adelica.new_notebook_module()
for c in ["root(9, 2; at = 4, seed = 3)", "root(9, 2; at = 5, seed = 4)", "roots(2, 2; at = 5)"]
    o = Adelica.evaluate(m, c, "c1"); println(c, "\n  ", o.extra[4].second, "\n  ", o.extra[5].second)
end
