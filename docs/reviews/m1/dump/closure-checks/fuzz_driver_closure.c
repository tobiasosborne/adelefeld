/* Exercise every repaired dump-length site, nearby signs/backends, and every prefix.
   The production fuzz assertions are unchanged. Their oracle is byte identity (CV-38). */
#include "tests/fuzz/fuzz_dump.c"

int main(void)
{
    static const char * const seeds[] = {
        "adf1 Q rat 1 1", "adf1 Q rat -10000000000000001 3", "adf1 Q rat 0 1",
        "adf1 Q fball g 5 12 1", "adf1 Q fball g -5 0 2",
        "adf1 Q fball l 2 6 2 2 3 0 0",
        "adf1 Q scaled x -1 3 6 2 2 3", "adf1 Q scaled s 1 2 3 6 1 6",
        "adf1 Q adele 1 -1 -1 1 -4 g 0 2 1",
        "adf1 Q adele 1 1 0 0 0 l 1 6 2 2 3 1 2",
        "adf1 Q cadele 1 1 -1 0 0 -1 0 1 -3 g 0 0 1",
        "adf1 Q cadele 1 1 0 1 -3 0 0 0 0 l 2 6 2 2 3 0 0",
        "adf1 Q modctx 6 2 3 2", "adf1 Q qclass pieces 1 1 1 -1 0 0 l 1 2 1 2 0"
    };
    size_t cases = 0;
    for (size_t i = 0; i < sizeof(seeds) / sizeof(*seeds); i++)
        for (size_t n = 0; n <= strlen(seeds[i]); n++)
        {
            LLVMFuzzerTestOneInput((const uint8_t *) seeds[i], n);
            cases++;
        }
    for (int i = 0; i < 3; i++) adf_modctx_free(fx[i]);
    flint_cleanup_master();
    printf("seeds=14 prefix_cases=%zu failures=0\n", cases);
    return 0;
}
