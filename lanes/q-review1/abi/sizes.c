/* Layout claims of docs/api-3.md section 1, checked against the real headers and FLINT 3.0.1.
   Run: gcc -I include -o lanes/q-review1/abi/sizes lanes/q-review1/abi/sizes.c && ./lanes/q-review1/abi/sizes */
#include <stdio.h>
#include <stddef.h>
#include "adelefeld/adele.h"
typedef struct { int form; long len; adf_adele_struct * piece; } qclass_struct;
int main(void)
{
    printf("adf_adele_struct size %zu align %zu\n", sizeof(adf_adele_struct), _Alignof(adf_adele_struct));
    printf("adf_fball_struct size %zu align %zu\n", sizeof(adf_fball_struct), _Alignof(adf_fball_struct));
    printf("arb_struct size %zu align %zu\n", sizeof(arb_struct), _Alignof(arb_struct));
    printf("adele offsets inf %zu fin %zu\n", offsetof(adf_adele_struct, inf), offsetof(adf_adele_struct, fin));
    printf("qclass_struct size %zu align %zu\n", sizeof(qclass_struct), _Alignof(qclass_struct));
    printf("qclass offsets form %zu len %zu piece %zu\n", offsetof(qclass_struct, form),
           offsetof(qclass_struct, len), offsetof(qclass_struct, piece));
    return 0;
}
