#!/usr/bin/env python3
"""Run every literal golden equality and retain all decimal-enclosure differences."""
from pathlib import Path
import ctypes as C

ROOT = Path(__file__).resolve().parents[2]
lib = C.CDLL(str(ROOT / "lanes/q-slice4/build/libadelefeld.so"))
lib.adf_sizeof_qclass.restype = C.c_size_t
lib.adf_qclass_init.argtypes = [C.c_void_p]
lib.adf_qclass_clear.argtypes = [C.c_void_p]
lib.adf_qclass_set_str.argtypes = [C.c_void_p, C.c_void_p, C.c_size_t, C.c_long, C.c_void_p]
lib.adf_qclass_get_str.argtypes = [C.POINTER(C.c_size_t), C.c_void_p, C.c_long]
lib.adf_qclass_get_str.restype = C.c_void_p
lib.adf_str_free.argtypes = [C.c_void_p]
x = C.create_string_buffer(lib.adf_sizeof_qclass())
lib.adf_qclass_init(x)
rows = differences = status_rows = 0
try:
    for line in (ROOT / "tests/golden/qclass.tsv").read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        rows += 1
        source, expected = line.split("\t")
        raw = source.encode("ascii")
        status = lib.adf_qclass_set_str(x, raw, len(raw), 128, None)
        if expected.startswith("!"):
            status_rows += 1
            names = {7: "DOMAIN", 9: "PARSE", 10: "LIMIT", 8: "UNSUPPORTED"}
            assert expected == "!" + names[status]
            continue
        assert status == 0
        n = C.c_size_t()
        p = lib.adf_qclass_get_str(C.byref(n), x, 2 if source.startswith("union") else 6)
        try:
            actual = C.string_at(p, n.value).decode("ascii")
        finally:
            lib.adf_str_free(p)
        if actual != expected:
            differences += 1
            print(f"row {rows}: expected {expected}\nactual: {actual}")
finally:
    lib.adf_qclass_clear(x)
    lib.flint_cleanup()
print(f"{rows} rows, {status_rows} matching status rows, {differences} literal equality differences")
raise SystemExit(1 if differences else 0)
