#include <adelefeld.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void)
{
    char line[65536];
    while (fgets(line, sizeof(line), stdin)) {
        char *sep = strchr(line, '\t');
        if (!sep) return 2;
        *sep++ = '\0';
        adf_place_t v = adf_place_inf();
        if (strcmp(line, "real") && adf_place_prime(&v, strtoul(line, NULL, 10))) return 2;
        sep[strcspn(sep, "\n")] = '\0';
        adf_cadele_t c;
        adf_fball_t zero;
        acb_t s, y;
        adf_cadele_init(c); adf_fball_init(zero); acb_init(s); acb_init(y);
        int st = adf_cadele_set_str(c, sep, strlen(sep), 128, NULL);
        if (!st) {
            adf_cadele_get_complex(s, c);
            st = adf_local_zeta_factor_at(y, NULL, s, v, 128);
        }
        if (st) printf("error: %s\n", adf_status_str(st));
        else {
            st = adf_cadele_set_acb_fball(c, y, zero);
            if (st) return 2;
            size_t n;
            char *out = adf_cadele_get_str(&n, c, 40);
            if (!out || n < 7) return 2;
            printf("%.*s\n", (int)(n - 6), out + 1);
            adf_str_free(out);
        }
        acb_clear(s); acb_clear(y); adf_cadele_clear(c); adf_fball_clear(zero);
    }
    flint_cleanup(); return 0;
}
