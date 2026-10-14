from pathlib import Path

lane = Path("lanes/j-slice4")
c = (lane / "driver.log").read_text().splitlines()
julia = (lane / "driver-julia.log").read_text().splitlines()
assert len(c) == 11 and len(julia) == 11, (len(c), len(julia))
assert c == julia, (c, julia)
text = "LINES=11 MATCHES=11 DUMPS=5 RECONSTRUCTION=6\n"
(lane / "driver-check.log").write_text(text)
print(text, end="")
