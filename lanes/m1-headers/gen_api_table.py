"""Write the function table of docs/api-m1.md from the declarations of the public headers.

The list of functions comes from `gcc -aux-info` on include/adelefeld.h (so the table cannot miss a
declaration); the work package and the source of each function come from the rules below, which
repeat the citations of the header comments. Prints the Markdown table to stdout.
Run from the repository root: python3 lanes/m1-headers/gen_api_table.py
"""
import os
import re
import subprocess
import sys
import tempfile

C = "conventions"
P = "docs/proofs"

# Exact sources, by function name. Anything not listed falls to the rules in source().
EXACT = {
    "adf_str_free": ("1.4", C + " 4.2, 8.1, 12.8 (CV-43)"),
    "adf_flint_version_compiled": ("1.9", C + " 12.11 (CV-44)"),
    "adf_version_check": ("1.9", C + " 12.11 (CV-44)"),
    "adf_status_str": ("1.9", C + " 3.1, 11.1"),
    "adf_rat_div": ("1.2", C + " 3.2 (division row)"),
    "adf_rat_inv": ("1.2", C + " 3.2 (division row)"),
    "adf_fball_set_fmpz3": ("1.2", C + " 5.2, L5.2; policies S26"),
    "adf_fball_set_center_radius": ("1.2", C + " 4.4; tests/ref/adfref/fball.py"),
    "adf_fball_canonicalise": ("1.2", C + " 5 (general rule), 5.2"),
    "adf_fball_get_fmpz3": ("1.2", "SPEC 4.1; policies P24.1, L18"),
    "adf_fball_get_center": ("1.2", C + " 5.2"),
    "adf_fball_get_radius": ("1.2", C + " 5.2; seams R2"),
    "adf_fball_get_den": ("1.2", "SPEC 4.1"),
    "adf_fball_is_exact": ("1.2", "SPEC 4.1"),
    "adf_fball_prec_at": ("1.2", "SPEC 4.1; " + C + " 5.2; seams R2"),
    "adf_fball_haar_volume": ("1.2", "catalogue P10; SPEC 9.3.7"),
    "adf_fball_add": ("1.2", "precision P1, L2"),
    "adf_fball_sub": ("1.2", "precision P1; policies T3"),
    "adf_fball_neg": ("1.2", "policies T3 step 2, P21.1"),
    "adf_fball_mul": ("1.2", "precision P2; policies L1"),
    "adf_fball_mul_rat": ("1.2", "precision P2 (scalar radius 0), P6(2)"),
    "adf_fball_div_rat": ("1.2", "precision P6(2); " + C + " 3.2"),
    "adf_fball_equal_set": ("1.2", "precision P3(1)"),
    "adf_fball_overlaps": ("1.2", "precision P3(2)"),
    "adf_fball_contains": ("1.2", "precision P3(3)"),
    "adf_fball_contains_rat": ("1.2", "precision L1"),
    "adf_fball_compare": ("1.2", "SPEC 4.2; " + C + " 2.1 (closure C1)"),
    "adf_adele_set_rat": ("1.3", "SPEC 4.1"),
    "adf_cadele_set_rat": ("1.3", "SPEC 4.1"),
    "adf_adele_set_arb_fball": ("1.3", C + " 3.2, 4.4, 5.5"),
    "adf_cadele_set_acb_fball": ("1.3", C + " 3.2, 4.4, 5.5"),
    "adf_adele_get_real": ("1.3", C + " 5.5; seams R3"),
    "adf_adele_get_arb_at": ("1.3", C + " 7; seams R3"),
    "adf_adele_div_rat": ("1.3", "SPEC 4.5; " + C + " 3.2"),
    "adf_cadele_div_rat": ("1.3", "SPEC 4.5; " + C + " 3.2"),
    "adf_cadele_set_adele": ("1.3", "SPEC 4.1"),
    "adf_fball_reconstruct": ("1.6", "quotient P11; SPEC 9.2; " + C + " 6.8 (CV-51)"),
    "adf_adele_reconstruct": ("1.6", "quotient P11; SPEC 9.2; " + C + " 6.8 (CV-51)"),
    "adf_text_limits_default": ("1.4", C + " 8.4 (CV-27)"),
    "adf_text_classify": ("1.4", C + " 9.7 (G14)"),
    "adf_modctx_new_blocks": ("1.8", C + " 4.6, 5.14"),
    "adf_modctx_new_prime_powers": ("1.8", C + " 4.6, 5.14"),
    "adf_modctx_new_fmpz": ("1.7", C + " 4.6, 5.14"),
    "adf_modctx_new_factorial": ("1.8", C + " 5.14 (CV-21)"),
    "adf_modctx_new_primorial_pow": ("1.8", C + " 5.14; PLAN 4"),
    "adf_modctx_new_from_dump": ("1.4", C + " 10.2 (closure C2, C3)"),
    "adf_modctx_free": ("1.7", C + " 4.6"),
    "adf_modctx_get_modulus": ("1.7", C + " 5.14"),
    "adf_modctx_nblocks": ("1.8", C + " 5.14"),
    "adf_modctx_block": ("1.8", C + " 5.14"),
    "adf_modctx_dump_str": ("1.4", C + " 10.1 (body modctx)"),
    "adf_ctx_desc_init": ("1.4", C + " 10.2 (closure C2)"),
    "adf_ctx_desc_clear": ("1.4", C + " 10.2 (closure C2)"),
    "adf_modctx_matches_desc": ("1.4", C + " 10.2"),
    "adf_fball_set_local": ("1.8", "policies P19; " + C + " 5.3"),
    "adf_fball_set_local_enclose": ("1.8", "policies P20; " + C + " 5.3"),
    "adf_fball_set_global": ("1.8", "policies L17.3, P24.1; " + C + " 5.3"),
    "adf_fball_is_local": ("1.8", C + " 5.2, 5.3"),
    "adf_fball_context": ("1.8", C + " 5.3"),
    "adf_scaled_init": ("1.7", C + " 2.3, 5.4"),
    "adf_scaled_set_rat": ("1.7", "policies D4; " + C + " 5.4"),
    "adf_scaled_set_fball": ("1.7", "policies P7, L6; " + C + " 5.4 (closure C5)"),
    "adf_scaled_set_context": ("1.7", "policies P11; " + C + " 5.4 (closure C5)"),
    "adf_scaled_get_fball": ("1.7", C + " 5.4, 9.4"),
    "adf_scaled_add": ("1.7", "precision P5(1); policies P8; " + C + " 4.6 (E1)"),
    "adf_scaled_sub": ("1.7", "precision P5(1); policies P8; " + C + " 4.6 (E1)"),
    "adf_scaled_mul": ("1.7", "precision P5(2); policies P9, P10.2 (CV-48)"),
    "adf_scaled_mul_tight": ("1.7", "policies P10.3; " + C + " 4.6 (E1)"),
    "adf_scaled_neg": ("1.7", "policies P9 (q = -1)"),
    "adf_scaled_mul_rat": ("1.7", "policies P9"),
    "adf_scaled_add_rat": ("1.7", "policies P8"),
    "adf_scaled_context": ("1.7", C + " 5.4"),
    "adf_scaled_is_exact": ("1.7", C + " 5.4 (CV-14)"),
    "adf_scaled_get_str": ("1.4", C + " 9.4 (scaled row)"),
    "adf_fball_cap": ("1.7", "policies D13, P14 (CV-47)"),
    "adf_fball_add_cap": ("1.7", "policies D13, P14, P15"),
    "adf_fball_sub_cap": ("1.7", "policies D13, P14, P15"),
    "adf_fball_mul_cap": ("1.7", "policies D13, P14"),
    "adf_fball_mul_rat_cap": ("1.7", "policies D13, P14.2"),
}

WP_BY_HEADER = {
    "place.h": "1.2", "rat.h": "1.2", "fball.h": "1.2", "adele.h": "1.3", "recon.h": "1.6",
    "text.h": "1.4", "modctx.h": "1.8", "scaled.h": "1.7", "dump.h": "1.4",
}


def source(name, header):
    if name in EXACT:
        return EXACT[name]
    base = header.split("/")[-1]
    wp = WP_BY_HEADER.get(base, "1.9")
    if name.startswith("adf_sizeof_") or name.startswith("adf_alignof_"):
        return "1.9", C + " 12.4 (CV-40)"
    if name.startswith("adf_place_"):
        return "1.2", C + " 7 (CV-18, CV-56); seams R1"
    if re.search(r"_(init|clear|set|swap|is_canonical|identical)$", name):
        typ = name.split("_")[1]
        sec = {"rat": "5.1", "fball": "5.2", "adele": "5.5", "cadele": "5.5", "scaled": "5.4"}[typ]
        return wp, C + " 2.3, " + sec
    if name.endswith("_set_str") or name.endswith("_get_str"):
        return "1.4", C + " 8.1, 9"
    if re.search(r"_(load_str|load_str_binds|dump_str)$", name):
        return "1.4", C + " 8.1, 10 (G3, CV-38, CV-52)"
    if name.endswith("_dump_inspect"):
        return "1.4", C + " 10.2 (closure C2)"
    if header.endswith("rat.h"):
        return "1.2", C + " 5.1; SPEC 4.1"
    if name.startswith("adf_fball_set_") or name in ("adf_fball_zero", "adf_fball_one"):
        return "1.2", C + " 5.2; SPEC 4.1"
    if header.endswith("adele.h"):
        return "1.3", "SPEC 4.1, 4.3; " + C + " 5.5"
    return wp, "?"


def main():
    root = os.getcwd()
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "all.c")
        aux = os.path.join(tmp, "aux.txt")
        with open(src, "w") as f:
            f.write("#include <adelefeld.h>\n")
        subprocess.run(["gcc", "-std=c11", "-I", os.path.join(root, "include"), "-fsyntax-only",
                        "-aux-info", aux, src], check=True)
        lines = [l for l in open(aux) if "include/adelefeld" in l]
    rows = []
    unknown = 0
    for l in lines:
        m = re.match(r"/\* (\S*include/)(adelefeld[^:]*):(\d+):\w+ \*/ (.*?);", l)
        header = m.group(2)
        decl = m.group(4)
        name = re.search(r"([A-Za-z_][A-Za-z_0-9]*) \(", decl).group(1)
        inline = decl.startswith("static ")
        wp, src_ = source(name, header)
        if src_ == "?":
            unknown += 1
        rows.append((name + (" (inline)" if inline else ""), "include/" + header, wp, src_))
    print("| Function | Header | WP | Proof or convention |")
    print("|---|---|---|---|")
    for r in rows:
        print("| `%s` | `%s` | %s | %s |" % r)
    print()
    print("%d functions." % len(rows))
    if unknown:
        print("unmapped: %d" % unknown, file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
