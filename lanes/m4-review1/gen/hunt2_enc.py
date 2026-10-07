"""Re-encoding of a parsed C rfun as harness input (exact balls)."""


def enc_out_rfun(R):
    """Parsed C rfun -> input encoding with exact balls."""
    def eb(b):
        m_, r_ = b
        if r_ == 0:
            return "%d %d 0 0" % (m_.numerator, m_.denominator)
        e = 0
        while r_.denominator != 1:
            r_ *= 2; e -= 1
        while r_.numerator % 2 == 0 and r_ > 1:
            r_ /= 2; e += 1
        return "%d %d %d %d" % (m_.numerator, m_.denominator, int(r_), e)
    parts = [str(len(R))]
    for (P, A, B, C) in R:
        parts.append(str(len(P)))
        for c in list(P) + [A, B, C]:
            parts.append(eb(c[0]) + " " + eb(c[1]))
    return " ".join(parts)


