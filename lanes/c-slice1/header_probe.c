#include <adelefeld/char.h>
#include <assert.h>
int main(void)
{
    adf_char_t x;
    adf_char_init(x);
    assert(adf_char_is_canonical(x));
    assert(adf_sizeof_char() == sizeof(adf_char_struct));
    assert(adf_alignof_char() == ADF_ALIGNOF(adf_char_struct));
    assert(ADF_CHAR_MOD_MAX == 65536);
    adf_char_clear(x);
    return 0;
}
