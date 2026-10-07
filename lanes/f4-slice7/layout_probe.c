#include <adelefeld.h>
#include <stdio.h>
int main(void)
{
    printf("acb bytes %zu; rterm bytes %zu; ulong bytes %zu; slong bytes %zu\n",
           sizeof(acb_struct), sizeof(adf_rterm_struct), sizeof(ulong), sizeof(slong));
    return 0;
}
