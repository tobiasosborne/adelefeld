#include <adelefeld.h>
#include <stdio.h>
static void show(const char *tag, mag_t m, long n, int d){
    fmpq_t q; fmpq_init(q); mag_get_fmpq(q, m);
    printf("%s man=%lu exp=", tag, MAG_MAN(m)); fmpz_print(MAG_EXPREF(m));
    printf("  fmpq="); fmpq_print(q);
    printf("  want=%ld/%d\n", n, d); fmpq_clear(q);
}
int main(void){
    mag_t m; mag_init(m);
    mag_set_ui_2exp_si(m, 536870913, -31); show("A", m, 536870913L, 1L<<31);
    mag_one(m); show("one", m, 1, 1);
    mag_set_ui_2exp_si(m, 3, 0); show("3", m, 3, 1);
    mag_set_ui_2exp_si(m, 1, 0); show("1", m, 1, 1);
    mag_set_ui_2exp_si(m, 1, -1); show("half", m, 1, 2);
    mag_set_ui_2exp_si(m, 5, 2); show("5*4", m, 5, 4);
    return 0;
}
