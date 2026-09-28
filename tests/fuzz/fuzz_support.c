/* tests/fuzz/fuzz_support.c: the fuzzer of the two support readers, tests/support/jsonl.c and
   tests/support/golden.c.

   libFuzzer calls LLVMFuzzerTestOneInput with arbitrary bytes. The bytes are read twice: once
   as a JSON-lines file, once as a golden file. The address and undefined-behaviour sanitizers
   of `make fuzz` then watch every allocation and every index of the two readers, which is the
   claim: a hostile vector file or golden file must not read out of bounds, must not leak and
   must not run into undefined behaviour. FLINT's loaders are never reached (PLAN.md 9: our
   loaders validate first).

   A parse that succeeds must also satisfy the invariants below, so that a reader that accepts
   something it should refuse, or that loses a byte, is caught here and not only in a test:

   - the number of records is the number of lines of the buffer;
   - a string is NUL-terminated at its own length, and an integer is a literal that a loop of
     its own reads again;
   - the keys of an object are distinct, and the size the reader reports is the number of
     children walked;
   - for a golden file: records + comment lines + blank lines = lines, a decoded input is
     NUL-terminated at input_len, the expected field is NUL-terminated at expected_len, and a
     status vector has a name one byte shorter than its field, the '!'. A NUL inside the
     expected field is a byte and not the end of the field, so strlen may be shorter than the
     length; what the reader reports is what the file holds.

   The golden reader is given a limit of 64 KiB, so that an @gen: line with a huge count is
   refused instead of allocated. */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "support/golden.h"
#include "support/jsonl.h"

/* The limit given to the golden reader here. A fuzzer that only ever sees the first kilobyte
   of a line would never reach the expansion; this bound keeps the memory of a run flat. */

#define FUZZ_GOLDEN_MAX ((size_t) 1 << 16)

static int
is_decimal(const char *s, size_t len)
{
    size_t i = 0;

    if (len > 0 && s[0] == '-')
        i = 1;
    if (i == len)
        return 0;
    if (s[i] == '0')
        return len - i == 1;
    for (; i < len; i++)
    {
        if (s[i] < '0' || s[i] > '9')
            return 0;
    }
    return 1;
}

/* Walk every value of a parsed file. Returns 0 if an invariant does not hold, so that libFuzzer
   saves the input as a crash. */

static int
check_jsonl_value(const jsonl_value *v, int depth)
{
    size_t i;
    size_t n;

    if (depth > JSONL_MAX_DEPTH)
        return 1;
    n = jsonl_size(v);
    switch (jsonl_kind(v))
    {
    case JSONL_NULL:
    case JSONL_BOOL:
        return n == 0;
    case JSONL_INT:
    {
        jsonl_error_t err;
        const char *t = jsonl_int_text(v, &err);

        return n == 0 && t != NULL && is_decimal(t, strlen(t));
    }
    case JSONL_STR:
    {
        jsonl_error_t err;
        size_t len = 0;
        const char *t = jsonl_string(v, &len, &err);

        return n == 0 && t != NULL && t[len] == '\0' && strlen(t) <= len;
    }
    case JSONL_ARRAY:
        for (i = 0; i < n; i++)
        {
            const jsonl_value *child = jsonl_at(v, i, NULL);

            if (child == NULL || !check_jsonl_value(child, depth + 1))
                return 0;
        }
        return 1;
    case JSONL_OBJECT:
        for (i = 0; i < n; i++)
        {
            const char *key = jsonl_key(v, i);
            const jsonl_value *child = jsonl_at(v, i, NULL);
            size_t j;

            if (key == NULL || child == NULL || !check_jsonl_value(child, depth + 1))
                return 0;
            for (j = 0; j < i; j++)
            {
                if (strcmp(jsonl_key(v, j), key) == 0)
                    return 0; /* a repeated key is refused by the reader */
            }
        }
        return 1;
    }
    return 0;
}

static size_t
count_lines(const char *data, size_t size)
{
    size_t n = 0;
    size_t i;

    for (i = 0; i < size; i++)
    {
        if (data[i] == '\n')
            n++;
    }
    if (size > 0 && data[size - 1] != '\n')
        n++;
    return n;
}

static void
fuzz_jsonl(const char *data, size_t size)
{
    jsonl_error_t err;
    jsonl_file *f = NULL;
    size_t i;

    if (jsonl_parse(data, size, "fuzz", &f, &err))
    {
        if (jsonl_count(f) != count_lines(data, size))
            abort();
        for (i = 0; i < jsonl_count(f); i++)
        {
            if (!check_jsonl_value(jsonl_record(f, i), 0))
                abort();
        }
        jsonl_close(f);
        f = NULL;
    }
    /* a failed parse leaves nothing behind and says where it failed */
    if (f != NULL)
        abort();
}

static void
fuzz_golden(const char *data, size_t size)
{
    golden_error_t err;
    golden_file *f = NULL;
    size_t i;

    if (golden_parse(data, size, "fuzz", FUZZ_GOLDEN_MAX, &f, &err))
    {
        if (golden_count(f) + golden_comment_lines(f) + golden_blank_lines(f) !=
            count_lines(data, size))
            abort();
        for (i = 0; i < golden_count(f); i++)
        {
            const golden_record *r = golden_record_at(f, i);

            if (r->input == NULL || r->expected == NULL)
                abort();
            if (r->input[r->input_len] != '\0')
                abort();
            /* the expected field is a byte string: a NUL inside it is a byte, not the end,
               so the length may be larger than strlen; it is the caller that decides what a
               valid expected text is */
            if (strlen(r->expected) > r->expected_len)
                abort();
            if (strlen(r->input) > r->input_len)
                abort();
            if (r->is_status)
            {
                if (r->expected[0] != '!' || strlen(r->status) + 1 != r->expected_len)
                    abort();
            }
            else if (r->expected[0] == '!')
                abort();
        }
        golden_close(f);
        f = NULL;
    }
    /* a failed parse leaves nothing behind and says where it failed */
    if (f != NULL)
        abort();
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    fuzz_jsonl((const char *) data, size);
    fuzz_golden((const char *) data, size);
    return 0;
}
