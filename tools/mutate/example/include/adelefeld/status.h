/* tools/mutate/example/include/adelefeld/status.h: a status.h for the self-test only.

   mutate.py's `status` kind (status_names in tools/mutate/mutate.py) reads every #define ADF_X
   of <root>/include/adelefeld/status.h and, for a bare `return ADF_X;`, offers ADF_OK ->
   ADF_DOMAIN and any other name -> ADF_OK. This file gives the example project the same shape
   as the real include/adelefeld/status.h so that shape is exercised end to end, compiled and
   run, and not only asserted against a synthetic snippet. ADF_STATUS_COUNT and an ADF_CMP_
   prefix are excluded by mutate.py itself; neither is needed here. */

#ifndef ADF_EXAMPLE_STATUS_H
#define ADF_EXAMPLE_STATUS_H

#define ADF_OK           0
#define ADF_DOMAIN       1
#define ADF_UNSUPPORTED  2

#endif /* ADF_EXAMPLE_STATUS_H */
