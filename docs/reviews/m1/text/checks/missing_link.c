#include "adelefeld.h"
int main(void) {
adf_rat_t x; size_t n; adf_rat_init(x);
char *s = adf_rat_dump_str(&n, x); adf_str_free(s); adf_rat_clear(x); return 0; }
