/* Source-including parts of the focused UBSan reproducer. No repository source is modified. */
#if defined(EDGE_LBALL)
#include "../../src/lball.c"
#elif defined(EDGE_DECOMP)
#include "../../src/lball_decomp.c"
#elif defined(EDGE_LFUNC)
#include "../../src/lfunc.c"
#else
#error Select one implementation for this focused reproducer
#endif
