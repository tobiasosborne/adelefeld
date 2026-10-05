#include <adelefeld.h>
#include <stdio.h>
static const char * sn(int s){switch(s){case ADF_OK:return "OK";case ADF_PARSE:return "PARSE";
 case ADF_LIMIT:return "LIMIT";case ADF_DOMAIN:return "DOMAIN";case ADF_UNSUPPORTED:return "UNSUP";
 case ADF_NOT_DETERMINED:return "ND";default:return "?";}}
int main(int argc, char ** argv)
{
    adf_adele_t a; adf_qclass_t q;
    adf_text_limits_t lim;
    int st, i;
    char buf[64];
    adf_adele_init(a); adf_qclass_init(q);
    adf_text_limits_default(&lim);
    lim.max_exp10 = 10000000;
    for (i = 1; i < argc; i++) {
        snprintf(buf, sizeof(buf), "(1e%s ; 0)", argv[i]);
        st = adf_adele_set_str(a, buf, strlen(buf), 64, &lim);
        printf("adele  %-24s -> %s", buf, sn(st));
        snprintf(buf, sizeof(buf), "(1e%s ; 0) + Q", argv[i]);
        st = adf_qclass_set_str(q, buf, strlen(buf), 64, &lim);
        printf("   qclass -> %s\n", sn(st));
    }
    return 0;
}
