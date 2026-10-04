/* lanes/drv-ball/lib_text.c: the text adf_sball_get_str / adf_lball_get_str give for the values the driver
   prints with adf_drv_put_sball and adf_drv_lball_text (lane drv-ball, step 3).

   It reads one line per value on standard input and writes one line per value on standard output.

     lib_text PREC DIGITS sball   the line is the text of the driver for a partial ball:
                                  "real: r; 5: c + O(5^N); ...".  The labels are rewritten to the labels of
                                  conventions 9.4 ("inf: " and "p=P: ") and the whole text is enclosed in
                                  braces, which is the value form of adf_sball; it is read with
                                  adf_sball_set_str at PREC and printed with adf_sball_get_str at DIGITS.
     lib_text PREC DIGITS lball   the line is "<p>: <local coordinate>" as the driver writes it.  The value
                                  form of adf_lball is "[p=<p>: <local coordinate>]"; it is read with
                                  adf_lball_set_str and printed with adf_lball_get_str.

   The program is an instrument of the comparison of step 3, not part of the driver; it is in this lane
   directory and is not built by any Makefile of the repository. */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adelefeld.h>

int
main(int argc, char ** argv)
{
    char * line = NULL;
    size_t cap = 0;
    ssize_t len;
    slong prec, digits;
    int mode_sball;

    if (argc != 4)
    {
        fprintf(stderr, "usage: lib_text PREC DIGITS sball|lball\n");
        return 2;
    }
    prec = atol(argv[1]);
    digits = atol(argv[2]);
    mode_sball = (strcmp(argv[3], "sball") == 0);

    while ((len = getline(&line, &cap, stdin)) > 0)
    {
        char * buf, * out = NULL, * tok, * save, * p;
        size_t n = 0, outlen = 0;

        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';
        buf = flint_malloc((size_t) len + 1);
        memcpy(buf, line, (size_t) len + 1);
        for (tok = strtok_r(buf, ";", &save); tok != NULL; tok = strtok_r(NULL, ";", &save))
        {
            char * colon;
            size_t k;

            while (*tok == ' ')
                tok++;
            k = strlen(tok);
            while (k > 0 && tok[k - 1] == ' ')
                tok[--k] = '\0';
            if (k == 0)
                continue;
            out = flint_realloc(out, outlen + k + 32);
            colon = strchr(tok, ':');
            if (mode_sball)
            {
                if (colon == NULL)
                    continue;
                if (n > 0)
                    out[outlen++] = ';';
                if (strncmp(tok, "real", 4) == 0)
                    outlen += (size_t) sprintf(out + outlen, "inf: %s", colon + 2);
                else
                    outlen += (size_t) sprintf(out + outlen, "p=%.*s: %s", (int) (colon - tok), tok, colon + 2);
            }
            else
            {
                if (colon == NULL)
                    continue;
                outlen += (size_t) sprintf(out + outlen, "[p=%.*s: %s", (int) (colon - tok), tok, colon + 1);
            }
            n++;
        }
        if (n == 0)
        {
            /* an empty partial ball: the driver prints no component at all, and the empty text of the
               value form of conventions 9.2 is "{}", but the driver writes no line for it */
            printf("(none)\n");
            flint_free(buf);
            flint_free(out);
            continue;
        }
        if (mode_sball)
        {
            adf_sball_t s;
            char * t;

            p = flint_malloc(outlen + 3);
            sprintf(p, "{%s}", out);
            adf_sball_init(s);
            if (adf_sball_set_str(s, p, strlen(p), prec, NULL) != ADF_OK)
                printf("(read failed)\n");
            else
            {
                t = adf_sball_get_str(&outlen, s, digits);
                printf("%s\n", t != NULL ? t : "(null)");
                if (t != NULL)
                    adf_str_free(t);
            }
            adf_sball_clear(s);
            flint_free(p);
        }
        else
        {
            adf_lball_t l;
            char * t;

            p = flint_malloc(outlen + 2);
            sprintf(p, "%s]", out);
            adf_lball_init(l);
            if (adf_lball_set_str(l, p, strlen(p), NULL) != ADF_OK)
                printf("(read failed)\n");
            else
            {
                t = adf_lball_get_str(&outlen, l);
                printf("%s\n", t != NULL ? t : "(null)");
                if (t != NULL)
                    adf_str_free(t);
            }
            adf_lball_clear(l);
            flint_free(p);
        }
        flint_free(buf);
        flint_free(out);
    }
    free(line);
    return 0;
}