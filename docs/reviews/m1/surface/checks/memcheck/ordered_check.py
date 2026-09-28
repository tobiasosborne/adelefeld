#!/usr/bin/env python3
"""ordered_check.py: tools/memcheck/check_uninit.py with one change: a use is reported when it
stands before the first init in the text (the tool reports it only when there is no init at
all, check_uninit.py:496).  Used to see whether the tree has a defect of shape S1.

    python3 docs/reviews/m1/surface/checks/memcheck/ordered_check.py src/*.c tests/*.c"""
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, "tools/memcheck")
import check_uninit as C


class Ordered(C.Analyzer):
    def record_use(self, name, line, text):
        var = self.lookup(name)
        if var is not None and var.init_line is None and var.use_line is None:
            self.findings.append(C.Finding(self.path, line, name, "use-before-first-init",
                                           "declared line %d" % var.line))
        super().record_use(name, line, text)


n = 0
for path in sys.argv[1:]:
    src = open(path, encoding="utf-8").read()
    toks = C.tokenize(src)
    funcs = C.find_functions(toks)
    helpers = C.infer_init_helpers(funcs)
    for _p, body, _l in funcs:
        for f in Ordered(path, body, helpers).run():
            if f.category == "use-before-first-init":
                print(f)
                n += 1
print("use-before-first-init findings:", n)
