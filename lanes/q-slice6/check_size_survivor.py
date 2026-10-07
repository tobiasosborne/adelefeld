#!/usr/bin/env python3
"""Retest the guard mutant from the final sweep against exact bit-cap component checks."""
from plant_faults import run_cases

if __name__ == '__main__':
    run_cases([('rational-size-or',
                'fmpz_bits(fmpq_numref(a)) <= ADF_QCLASS_BITS_MAX &&',
                'fmpz_bits(fmpq_numref(a)) <= ADF_QCLASS_BITS_MAX ||')], 'size-survivor-results.tsv')
