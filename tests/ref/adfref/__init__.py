"""Reference implementation for the ring of finite adeles of Q (work package 1.1).

Written from the proofs, for clarity, not speed. See tests/ref/README.md.
"""
from . import fball, membership, policies, rat, recon

__all__ = ["rat", "fball", "policies", "recon", "membership"]
