#include "../../../../../tests/fuzz/fuzz_text.c"

int main(void)
{
    const uint8_t bad[] = {'@'};
    return LLVMFuzzerTestOneInput(bad, sizeof(bad));
}
