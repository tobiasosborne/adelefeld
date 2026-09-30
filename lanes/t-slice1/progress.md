t-slice1 progress (times from date)
2026-09-30 03:02:39  read the brief, COMMON*, workflow, conventions 5.6 5.7 8 9 10, headers, text.c, adf.c, proto; plan: shared helpers in text.c hidden (adf_tx_*), glue in text_idele.c
2026-09-30 03:14:00  C reader/printer green: build/test_text_idele 13 tests, 73457 checks; next: driver cases (red), driver code, julia, fuzz, docs
2026-09-30 03:24:59  driver: i-01..i-06 cases green (41 cases); old cases 06,07,12,13 edited: [5 mod 6] -> [p=5: 3] where it stood for an unsupported kind (consequence of the new kinds; listed in the result)
2026-09-30 04:04:34  all checks green (check-all, SAN=1, clang, INV=1, check_headers, SAN=1 driver); result.md written; done
