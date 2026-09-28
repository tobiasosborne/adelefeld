/* Exercise the unchanged project fuzz target with its valid, smallest rational dump. */
#include "R3_fuzz.c"
int main(void)
{
    const char *s = "adf1 Q rat 1 1";
    LLVMFuzzerTestOneInput((const uint8_t *)s, strlen(s));
    for (int i = 0; i < 3; i++) adf_modctx_free(fx[i]);
    flint_cleanup_master();
    puts("valid_seed_completed=1");
    return 0;
}
