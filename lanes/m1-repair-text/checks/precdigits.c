/* lanes/m1-repair-text/checks/precdigits.c: report the sets of prec and digits that the
   accepted adele and cadele inputs of a text-fuzz corpus reach (finding R9 of reviewer text).

   It decodes the two control bytes exactly as tests/fuzz/fuzz_text.c does
   (prec = 2 + data[0] % 190, digits = 1 + data[1] % 30) and calls adf_adele_set_str and
   adf_cadele_set_str with the same limits (defaults, max_exp10 = 5000). An input counts for a
   set when the corresponding parser returns ADF_OK.

   Usage: ./precdigits CORPUS_DIR
   Prints: accepted_adele, accepted_cadele, and the sorted sets. */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adelefeld.h"

static int
read_file(const char * path, unsigned char ** data, size_t * size)
{
    FILE * f = fopen(path, "rb");
    long n;
    unsigned char * p;

    if (f == NULL)
        return 0;
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        return 0;
    }
    p = (unsigned char *) malloc((size_t) n + 1);
    if (p == NULL)
    {
        fclose(f);
        return 0;
    }
    if (n > 0 && fread(p, 1, (size_t) n, f) != (size_t) n)
    {
        free(p);
        fclose(f);
        return 0;
    }
    fclose(f);
    *data = p;
    *size = (size_t) n;
    return 1;
}

int
main(int argc, char ** argv)
{
    DIR * d;
    struct dirent * e;
    int prec_seen[190], dig_seen[30];
    long n_adele = 0, n_cadele = 0, n_files = 0;
    int i;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s CORPUS_DIR\n", argv[0]);
        return 2;
    }
    memset(prec_seen, 0, sizeof(prec_seen));
    memset(dig_seen, 0, sizeof(dig_seen));
    d = opendir(argv[1]);
    if (d == NULL)
    {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return 2;
    }
    while ((e = readdir(d)) != NULL)
    {
        char path[4096];
        unsigned char * data;
        size_t size, n;
        slong prec, digits;
        adf_text_limits_t lim;
        adf_adele_t x;
        adf_cadele_t z;
        int a, c;

        if (e->d_name[0] == '.')
            continue;
        if (snprintf(path, sizeof(path), "%s/%s", argv[1], e->d_name) >= (int) sizeof(path))
            continue;
        if (!read_file(path, &data, &size))
            continue;
        n_files++;
        if (size < 2)
        {
            free(data);
            continue;
        }
        prec = 2 + (slong) (data[0] % 190);
        digits = 1 + (slong) (data[1] % 30);
        n = size - 2;
        adf_text_limits_default(&lim);
        lim.max_exp10 = 5000;

        adf_adele_init(x);
        a = adf_adele_set_str(x, (const char *) data + 2, n, prec, &lim);
        adf_adele_clear(x);
        adf_cadele_init(z);
        c = adf_cadele_set_str(z, (const char *) data + 2, n, prec, &lim);
        adf_cadele_clear(z);

        if (a == ADF_OK)
        {
            n_adele++;
            prec_seen[prec - 2] = 1;
            dig_seen[digits - 1] = 1;
        }
        if (c == ADF_OK)
        {
            n_cadele++;
            prec_seen[prec - 2] = 1;
            dig_seen[digits - 1] = 1;
        }
        free(data);
    }
    closedir(d);
    printf("files=%ld accepted_adele=%ld accepted_cadele=%ld\n", n_files, n_adele, n_cadele);
    printf("prec:");
    for (i = 0; i < 190; i++)
        if (prec_seen[i])
            printf(" %d", i + 2);
    printf("\ndigits:");
    for (i = 0; i < 30; i++)
        if (dig_seen[i])
            printf(" %d", i + 1);
    printf("\n");
    return 0;
}
