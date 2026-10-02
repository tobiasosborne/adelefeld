"""Make renamed old and instrumented current printer copies, only inside this lane."""
from pathlib import Path
import subprocess

lane = Path("lanes/n-review2")
symbols = []
for name in ("text", "text_idele"):
    out = subprocess.check_output(
        ["nm", "--defined-only", "-g", str(lane / "build" / (name + ".o"))], text=True,
        timeout=10)
    symbols += [line.split()[-1] for line in out.splitlines() if " T " in line]
for prefix in ("old", "review"):
    defines = "".join(f"#define {s} {prefix}_{s}\n" for s in symbols)
    for name in ("text", "text_idele"):
        if prefix == "old":
            source = subprocess.check_output(
                ["git", "show", f"674c9db:src/{name}.c"], text=True, timeout=10)
        else:
            source = Path(f"src/{name}.c").read_text()
        if prefix == "review" and name == "text":
            source = source.replace("#define TX_COND_WORK_MAX", "unsigned long review_levels, review_passes;\n"
                "unsigned long review_work, review_attempt, review_first_size, review_max_size;\n"
                "#define TX_COND_WORK_MAX", 1)
            source = source.replace("    ulong work = 0;", "    ulong work = 0;\n"
                "    review_levels = review_passes = review_work = review_attempt = 0;\n"
                "    review_first_size = review_max_size = 0;", 1)
            source = source.replace("        ulong size = tx_cond_size(mid, rad);",
                "        ulong size = tx_cond_size(mid, rad);\n"
                "        if (!review_passes) review_first_size = size;\n"
                "        review_passes++;\n"
                "        if (size > review_max_size) review_max_size = size;", 1)
            source = source.replace("            work += size;", "            work += size;\n"
                "            review_attempt = work;", 1)
            source = source.replace("            tx_real_level(&lb, M, R, mid, rad, n, k);",
                "            review_levels++; review_work = work;\n"
                "            tx_real_level(&lb, M, R, mid, rad, n, k);", 1)
            source += "\nchar *review_level(size_t *len, fmpq_t M, fmpq_t R, " \
                "const fmpq_t m, const fmpq_t r, slong n, slong k) {\n" \
                "tx_buf b; tx_buf_init(&b); tx_real_level(&b,M,R,m,r,n,k); return tx_finish(&b,len); }\n"
        (lane / f"{prefix}_{name}.c").write_text(defines + source)
print(f"renamed_symbols={len(symbols)} source_copies=4")
