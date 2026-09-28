#include <stdint.h>
#include "../../../../../tests/fuzz/fuzz_text.c"

int
main(void)
{
    const uint8_t inputs[][11] = {
        {0, 0, '(', '1', ' ', ';', ' ', '0', ')'},
        {189, 29, '(', '1', ' ', ';', ' ', '0', ')'},
        {0, 29, '(', '1', ' ', ';', ' ', '0', ')'},
        {189, 0, '(', '1', ' ', ';', ' ', '0', ')'}
    };
    size_t i;

    for (i = 0; i < 4; i++)
        if (LLVMFuzzerTestOneInput(inputs[i], 9) != 0)
            return 1;
    puts("accepted_inputs=4 control_pairs=4 failed=0");
    return 0;
}
