#include <adelefeld.h>
#include <stdio.h>
int main(void){
    mag_t m; mag_init(m);
    mag_set_ui_2exp_si(m, 536870913, -31); printf("A get_d=%.17g\n", mag_get_d(m));
    mag_one(m); printf("one get_d=%.17g\n", mag_get_d(m));
    mag_set_ui_2exp_si(m, 3, 0); printf("3 get_d=%.17g\n", mag_get_d(m));
    mag_set_ui_2exp_si(m, 1, 0); printf("1 get_d=%.17g\n", mag_get_d(m));
    mag_set_ui_2exp_si(m, 1, -1); printf("half get_d=%.17g\n", mag_get_d(m));
    mag_set_ui_2exp_si(m, 5, 2); printf("5*4 get_d=%.17g\n", mag_get_d(m));
    mag_set_ui_2exp_si(m, 123456789, 3); printf("123456789*8 get_d=%.17g\n", mag_get_d(m));
    return 0;
}
