/* tests/fuzz/fuzz_driver.c: the coverage-guided fuzz target of the command-line driver
   adf (docs/PLAN.md 6, row 1.5).  Built and run by `make fuzz FUZZ_TARGET=driver`.

   The bytes of the input are the script of commands, and they are run by
   adf_driver_run, the function the program itself uses: the driver is included as a
   translation unit with its main() switched off, so the fuzzer reaches the line splitting,
   the operand splitting, every operation and every printer of tools/adf/adf.c.

   What it asserts (a run that does not crash is only half a claim):
   1. the exit status of adf_driver_run is 0, 1 or 2, the three values of the README;
   2. the number of lines written is at most the number of lines of the input: every
      command answers exactly one line, and an empty line, a comment and an over-long line
      do not change that;
   3. every line that begins with "error: " is followed by a status name of adf_status_str
      (conventions 3.1): a line with any other text is a bug of the driver;
   4. the exit status 0 comes with no error line at all, and the exit status 1 comes with
      at least one: the status counts the commands that failed. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>

/* the driver without its main(); adf.c is the unit under test */
#define ADF_DRIVER_NO_MAIN 1
#include "../../tools/adf/adf.c"

/* drv_is_error_line(line): 1 when the line is "error: " followed by a status name. */
static int
drv_is_error_line(const char * line)
{
    int k, ok = 0;

    if (strncmp(line, "error: ", 7) != 0)
        return 0;
    line += 7;
    for (k = 0; k < ADF_STATUS_COUNT; k++)
        if (strcmp(line, adf_status_str(k)) == 0)
            ok = 1;
    return ok;
}

/* drv_looks_like_error(line): 1 when the line begins with the prefix of an error line. */
static int
drv_looks_like_error(const char * line)
{
    return strncmp(line, "error:", 6) == 0;
}

/* drv_count_lines(s, len): the number of lines of the input; a last line without a
   newline is counted. */
static size_t
drv_count_lines(const char * s, size_t len)
{
    size_t i, n = 0;

    for (i = 0; i < len; i++)
        if (s[i] == '\n')
            n++;
    if (len > 0 && s[len - 1] != '\n')
        n++;
    return n;
}

int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    FILE * out;
    char * buf;
    size_t cap = 65536, lines = 0, errors = 0, expected;
    int rc;

    out = tmpfile();
    if (out == NULL)
        return 0;

    rc = adf_driver_run((const char *) data, size, out);
    if (rc != 0 && rc != 1 && rc != 2)
    {
        fprintf(stderr, "fuzz_driver: the status %d is not 0, 1 or 2\n", rc);
        abort();
    }

    buf = (char *) malloc(cap);
    if (buf == NULL)
    {
        fclose(out);
        return 0;
    }
    rewind(out);
    while (fgets(buf, (int) cap, out) != NULL)
    {
        size_t n = strlen(buf);

        lines++;
        while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
            buf[--n] = '\0';
        if (drv_looks_like_error(buf))
        {
            errors++;
            if (!drv_is_error_line(buf))
            {
                fprintf(stderr, "fuzz_driver: \"%s\" is not the name of a status\n", buf);
                abort();
            }
        }
    }
    free(buf);
    fclose(out);

    /* every command answers one line, and no line of the input is a command twice */
    expected = drv_count_lines((const char *) data, size);
    if (lines > expected)
    {
        fprintf(stderr, "fuzz_driver: %lu lines written for %lu lines of input\n",
                (unsigned long) lines, (unsigned long) expected);
        abort();
    }
    /* the status counts the commands that failed, and every failure writes its line */
    if (rc == 0 && errors != 0)
    {
        fprintf(stderr, "fuzz_driver: the status 0 with %lu error lines\n",
                (unsigned long) errors);
        abort();
    }
    if (rc == 1 && errors == 0)
    {
        fprintf(stderr, "fuzz_driver: the status 1 without an error line\n");
        abort();
    }
    return 0;
}
