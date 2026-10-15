/* tests/support/timing.h: the factor on the wall-clock bounds of the tests (lane t-timing1, bead adf-mis2).
   Makefile lines 47-51: `make SAN=1` adds -fsanitize=address,undefined. Under that build the wall-clock checks
   failed at host load 6 to 17 (2.16 s against 2.0; 4.9 and 5.7 s against 3.5; one child over 5 s) and passed
   alone: the sanitizer build measures the sanitizer, not the library. Decision of the orchestrator, 2026-10-15:
   the bounds get a factor 4 under the address sanitizer and stay unchanged in the plain build. */
#ifndef ADF_TESTS_SUPPORT_TIMING_H
#define ADF_TESTS_SUPPORT_TIMING_H

#if defined(__SANITIZE_ADDRESS__)
#define ADF_TIMING_ASAN 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define ADF_TIMING_ASAN 1
#endif
#endif

#ifdef ADF_TIMING_ASAN
#define ADF_TIMING_FACTOR 4
#else
#define ADF_TIMING_FACTOR 1
#endif

#endif
