/* lanes/m1-repair-dump/checks/probe.c: the status of adf_modctx_new_from_dump on a few texts
   given on the command line (each argument is one text, with \t, \n, \r written as escapes). */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adelefeld.h>

static size_t
unescape(const char * in, char * out)
{
    size_t n = 0;

    while (*in != '\0')
    {
        if (in[0] == '\\' && in[1] == 't')
        {
            out[n++] = '\t';
            in += 2;
        }
        else if (in[0] == '\\' && in[1] == 'n')
        {
            out[n++] = '\n';
            in += 2;
        }
        else if (in[0] == '\\' && in[1] == 'r')
        {
            out[n++] = '\r';
            in += 2;
        }
        else if (in[0] == '\\' && in[1] == '0')
        {
            out[n++] = '\0';
            in += 2;
        }
        else if (in[0] == '\\' && in[1] == 'x' && in[3] == '\0')
        {
            char h[3];
            h[0] = in[2];
            h[1] = in[2];
            h[2] = '\0';
            out[n++] = (char) strtol(h, NULL, 16);
            in += 4;
        }
        else
            out[n++] = *in++;
    }
    return n;
}

int
main(int argc, char ** argv)
{
    int a;

    for (a = 1; a < argc; a++)
    {
        char buf[4096];
        size_t n = unescape(argv[a], buf);
        adf_modctx_struct * c = NULL;
        int st = adf_modctx_new_from_dump(&c, buf, n, 0, NULL);
        size_t nctx = 7;
        adf_ctx_desc_t d;
        int ist;

        adf_ctx_desc_init(&d);
        ist = adf_scaled_dump_inspect(&nctx, &d, buf, n, NULL);
        printf("%-70s from_dump=%-12s blocks=%-4ld scaled_inspect=%s nctx=%zu\n", argv[a], adf_status_str(st),
               c ? (long) adf_modctx_nblocks(c) : -1L, adf_status_str(ist), nctx);
        adf_ctx_desc_clear(&d);
        adf_modctx_free(c);
    }
    flint_cleanup_master();
    return 0;
}
