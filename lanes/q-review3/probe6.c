#include <adelefeld.h>
#include <stdio.h>
static void dump(const char *t, mag_t m){
    unsigned char *b = (unsigned char*) m; int i;
    printf("%s:", t);
    for (i = 0; i < 16; i++) printf(" %02x", b[i]);
    printf("  as slongs: %ld %ld   get_d=%.17g\n", (long)((long*)m)[0], (long)((long*)m)[1], mag_get_d(m));
}
int main(void){
    mag_t m; mag_init(m);
    mag_set_ui_2exp_si(m, 536870913, -31); dump("set_ui_2exp_si(536870913,-31)", m);
    mag_set_ui_2exp_si(m, 123456789, -20); dump("set_ui_2exp_si(123456789,-20)", m);
    mag_one(m); dump("one", m);
    mag_zero(m); MAG_MAN(m) = 536870913UL; fmpz_set_si(MAG_EXPREF(m), -31); dump("direct", m);
    return 0;
}
