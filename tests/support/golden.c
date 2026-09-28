/* tests/support/golden.c: the reader of tests/support/golden.h.

   C11, libc only. The file is walked line by line; a vector is a line with exactly one TAB.
   The escapes of conventions.md 11.1 are decoded in the input and in the parts of an @gen:
   line, and nowhere else. Every path that fails fills the error with the file, the line and
   the column, and stops; nothing is skipped silently. */

#include "golden.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct golden_file
{
    char *name;
    golden_record *records;
    size_t nrecords;
    size_t cap;
    size_t comments;
    size_t blanks;
    golden_error_t err;
    int failed;
};

typedef struct
{
    golden_file *f;
    const char *name;
    unsigned long line;
    const char *line_start; /* for the column of an error */
    size_t max_input;
    golden_error_t err;
} gctx;

static void
golden_error_set(golden_error_t *err, const char *file, unsigned long line, unsigned long column,
                 const char *fmt, ...)
{
    va_list ap;

    if (err == NULL)
        return;
    snprintf(err->file, sizeof(err->file), "%s", file == NULL ? "(no file)" : file);
    err->line = line;
    err->column = column;
    va_start(ap, fmt);
    vsnprintf(err->message, sizeof(err->message), fmt, ap);
    va_end(ap);
}

static int
golden_fail(gctx *c, const char *at, const char *fmt, ...)
{
    va_list ap;
    unsigned long column = 1;

    if (at != NULL && at >= c->line_start)
        column = (unsigned long) (at - c->line_start) + 1;
    if (!c->f->failed)
    {
        c->f->failed = 1;
        va_start(ap, fmt);
        vsnprintf(c->f->err.message, sizeof(c->f->err.message), fmt, ap);
        va_end(ap);
        snprintf(c->f->err.file, sizeof(c->f->err.file), "%s",
                 c->name == NULL ? "(no file)" : c->name);
        c->f->err.line = c->line;
        c->f->err.column = column;
    }
    va_start(ap, fmt);
    vsnprintf(c->err.message, sizeof(c->err.message), fmt, ap);
    va_end(ap);
    snprintf(c->err.file, sizeof(c->err.file), "%s", c->name == NULL ? "(no file)" : c->name);
    c->err.line = c->line;
    c->err.column = column;
    return 0;
}

static golden_record *
golden_new_record(gctx *c)
{
    golden_file *f = c->f;

    if (f->nrecords == f->cap)
    {
        size_t cap = f->cap == 0 ? 64 : 2 * f->cap;
        golden_record *r = (golden_record *) realloc(f->records, cap * sizeof(golden_record));

        if (r == NULL)
        {
            golden_fail(c, NULL, "out of memory");
            return NULL;
        }
        f->records = r;
        f->cap = cap;
    }
    memset(&f->records[f->nrecords], 0, sizeof(golden_record));
    return &f->records[f->nrecords++];
}

static int
golden_is_hex(int ch)
{
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

static int
golden_hex_value(int ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return ch - 'A' + 10;
}

/* A growable byte string. */

typedef struct
{
    char *p;
    size_t len;
    size_t cap;
} gbuf;

static int
gbuf_put(gctx *c, gbuf *b, int byte)
{
    if (b->len + 2 > b->cap)
    {
        size_t cap = b->cap == 0 ? 64 : 2 * b->cap;
        char *p = (char *) realloc(b->p, cap);

        if (p == NULL)
            return golden_fail(c, NULL, "out of memory");
        b->p = p;
        b->cap = cap;
    }
    b->p[b->len++] = (char) byte;
    return 1;
}

static void
gbuf_free(gbuf *b)
{
    free(b->p);
    b->p = NULL;
    b->len = 0;
    b->cap = 0;
}

static int
gbuf_finish(gctx *c, gbuf *b, char **out, size_t *out_len)
{
    if (!gbuf_put(c, b, '\0'))
        return 0;
    *out = b->p;
    *out_len = b->len - 1;
    return 1;
}

/* Decode the escapes of conventions.md 11.1 over [p, end). Any other backslash sequence is
   an error in the file. */

static int
golden_decode(gctx *c, const char *p, const char *end, gbuf *out)
{
    while (p < end)
    {
        if (*p != '\\')
        {
            if (!gbuf_put(c, out, (unsigned char) *p))
                return 0;
            p++;
            continue;
        }
        if (p + 1 >= end)
            return golden_fail(c, p, "a backslash at the end of a field");
        p++;
        switch (*p)
        {
        case '\\':
            if (!gbuf_put(c, out, '\\'))
                return 0;
            p++;
            break;
        case 't':
            if (!gbuf_put(c, out, '\t'))
                return 0;
            p++;
            break;
        case 'n':
            if (!gbuf_put(c, out, '\n'))
                return 0;
            p++;
            break;
        case 'r':
            if (!gbuf_put(c, out, '\r'))
                return 0;
            p++;
            break;
        case 'x':
            if (p + 2 >= end || !golden_is_hex((unsigned char) p[1]) ||
                !golden_is_hex((unsigned char) p[2]))
                return golden_fail(c, p - 1, "\\x needs two hexadecimal digits");
            if (!gbuf_put(c, out, 16 * golden_hex_value((unsigned char) p[1]) +
                                       golden_hex_value((unsigned char) p[2])))
                return 0;
            p += 3;
            break;
        default:
            return golden_fail(c, p - 1, "unknown escape \\%c", *p);
        }
    }
    return 1;
}

/* @gen:PREFIX|UNIT|COUNT|SUFFIX over the raw text of the input field [p, end). The four
   parts are decoded; COUNT is a plain decimal number; the expansion is materialised and
   refused before anything is allocated if it is longer than max_input. */

static int
golden_expand(gctx *c, const char *p, const char *end, char **out, size_t *out_len, int *generated)
{
    const char *part[4];
    const char *stop[4];
    gbuf buf = {NULL, 0, 0};
    unsigned long long count = 0;
    size_t total;
    size_t k;
    int i;

    *generated = 0;
    p += 5; /* "@gen:" */
    for (i = 0; i < 4; i++)
    {
        part[i] = p;
        while (p < end && *p != '|')
            p++;
        stop[i] = p;
        if (i < 3)
        {
            if (p >= end)
            {
                gbuf_free(&buf);
                return golden_fail(c, part[0], "@gen: needs four parts, PREFIX|UNIT|COUNT|SUFFIX");
            }
            p++; /* the '|' */
        }
    }
    if (p != end)
    {
        gbuf_free(&buf);
        return golden_fail(c, p, "@gen: needs four parts, PREFIX|UNIT|COUNT|SUFFIX");
    }

    /* COUNT: at least one digit, no sign, no leading zero unless the number is 0. */

    if (stop[2] == part[2])
    {
        gbuf_free(&buf);
        return golden_fail(c, part[2], "@gen: the count is empty");
    }
    if (stop[2] - part[2] > 1 && part[2][0] == '0')
    {
        gbuf_free(&buf);
        return golden_fail(c, part[2], "@gen: the count has a leading zero");
    }
    for (k = 0; k < (size_t) (stop[2] - part[2]); k++)
    {
        if (part[2][k] < '0' || part[2][k] > '9')
        {
            gbuf_free(&buf);
            return golden_fail(c, part[2] + k, "@gen: the count is not a decimal number");
        }
        if (count > (0xFFFFFFFFFFFFFFFFULL - (unsigned long long) (part[2][k] - '0')) / 10ULL)
        {
            gbuf_free(&buf);
            return golden_fail(c, part[2] + k, "@gen: the count is too large");
        }
        count = 10ULL * count + (unsigned long long) (part[2][k] - '0');
    }

    /* The length first, so that a hostile count is refused before it is allocated. The three
       steps are the additions of PREFIX, of COUNT * UNIT and of SUFFIX; each is checked. */

    total = (size_t) (stop[0] - part[0]);
    if (count != 0 && (size_t) (stop[1] - part[1]) > ((size_t) -1 - total) / (size_t) count)
    {
        gbuf_free(&buf);
        return golden_fail(c, part[2], "@gen: the expansion is too large");
    }
    total += (size_t) count * (size_t) (stop[1] - part[1]);
    if ((size_t) (stop[3] - part[3]) > (size_t) -1 - total)
    {
        gbuf_free(&buf);
        return golden_fail(c, part[2], "@gen: the expansion is too large");
    }
    total += (size_t) (stop[3] - part[3]);
    if (total > c->max_input)
    {
        gbuf_free(&buf);
        return golden_fail(c, part[2], "@gen: %llu bytes of input, the limit is %llu",
                          (unsigned long long) total, (unsigned long long) c->max_input);
    }

    if (!golden_decode(c, part[0], stop[0], &buf))
    {
        gbuf_free(&buf);
        return 0;
    }
    for (k = 0; k < (size_t) count; k++)
    {
        if (!golden_decode(c, part[1], stop[1], &buf))
        {
            gbuf_free(&buf);
            return 0;
        }
    }
    if (!golden_decode(c, part[3], stop[3], &buf))
    {
        gbuf_free(&buf);
        return 0;
    }
    if (!gbuf_finish(c, &buf, out, out_len))
    {
        gbuf_free(&buf);
        return 0;
    }
    *generated = 1;
    return 1;
}

/* One line of the file. `p` is the first byte, `end` the byte after the last (the LF is not
   part of it). */

static int
golden_read_line(gctx *c, const char *p, const char *end)
{
    const char *tab = NULL;
    const char *q;
    int ntabs = 0;
    golden_record *r;
    gbuf buf = {NULL, 0, 0};
    char *expected = NULL;
    size_t expected_len = 0;

    for (q = p; q < end; q++)
    {
        if (*q == '\t')
        {
            ntabs++;
            if (ntabs == 1)
                tab = q;
        }
    }
    if (ntabs != 1)
    {
        if (ntabs == 0)
            return golden_fail(c, p, "a vector is input<TAB>expected, and this line has no TAB");
        return golden_fail(c, tab, "a vector has exactly one TAB, and this line has %d", ntabs);
    }

    r = golden_new_record(c);
    if (r == NULL)
        return 0;
    r->line = c->line;

    if ((size_t) (tab - p) >= 5 && memcmp(p, "@gen:", 5) == 0)
    {
        if (!golden_expand(c, p, tab, &r->input, &r->input_len, &r->generated))
            return 0;
    }
    else
    {
        if (!golden_decode(c, p, tab, &buf))
        {
            gbuf_free(&buf);
            return 0;
        }
        if (!gbuf_finish(c, &buf, &r->input, &r->input_len))
        {
            gbuf_free(&buf);
            return 0;
        }
    }

    expected_len = (size_t) (end - tab - 1);
    expected = (char *) malloc(expected_len + 1);
    if (expected == NULL)
        return golden_fail(c, tab, "out of memory");
    memcpy(expected, tab + 1, expected_len);
    expected[expected_len] = '\0';
    r->expected = expected;
    r->expected_len = expected_len;

    if (expected_len == 0)
        return golden_fail(c, tab, "the expected field is empty");
    if (expected[0] == '!')
    {
        size_t i;

        r->is_status = 1;
        if (expected_len - 1 >= GOLDEN_STATUS_MAX)
            return golden_fail(c, tab + 1, "the status name is longer than %d characters",
                               GOLDEN_STATUS_MAX - 1);
        for (i = 1; i < expected_len; i++)
        {
            char ch = expected[i];

            if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_'))
                return golden_fail(c, tab + 1 + i, "'%c' is not a letter of a status name", ch);
            r->status[i - 1] = ch;
        }
        if (expected_len == 1)
            return golden_fail(c, tab + 1, "a status is '!' followed by a name");
    }
    return 1;
}

int
golden_parse(const char *text, size_t len, const char *name, size_t max_input, golden_file **out,
             golden_error_t *err)
{
    golden_file *f;
    gctx c;
    const char *p;
    const char *end;
    char *copy;

    if (out != NULL)
        *out = NULL;
    if (out == NULL || text == NULL || name == NULL)
    {
        golden_error_set(err, name, 0, 0, "a null argument");
        return 0;
    }
    f = (golden_file *) calloc(1, sizeof(*f));
    copy = (char *) malloc(len + 1);
    if (f == NULL || copy == NULL || (f->name = (char *) malloc(strlen(name) + 1)) == NULL)
    {
        free(f);
        free(copy);
        golden_error_set(err, name, 0, 0, "out of memory");
        return 0;
    }
    memcpy(f->name, name, strlen(name) + 1);
    memcpy(copy, text, len);
    copy[len] = '\0';

    c.f = f;
    c.name = f->name;
    c.line = 0;
    c.line_start = copy;
    c.max_input = max_input;
    memset(&c.err, 0, sizeof(c.err));

    if (len == 0)
    {
        free(f->name);
        free(f);
        free(copy);
        golden_error_set(err, name, 1, 1, "the file is empty");
        return 0;
    }

    p = copy;
    end = copy + len;
    while (p <= end)
    {
        const char *nl = (const char *) memchr(p, '\n', (size_t) (end - p));
        const char *stop = (nl != NULL) ? nl : end;

        c.line++;
        c.line_start = p;
        if (stop == p)
        {
            if (nl == NULL)
                break; /* the last line has no LF, and it is empty: end of file */
            f->blanks++;
        }
        else if (*p == '#')
            f->comments++;
        else if (!golden_read_line(&c, p, stop))
            break;
        if (nl == NULL)
            break;
        p = nl + 1;
    }

    if (f->failed)
    {
        if (err != NULL)
            *err = f->err;
        golden_close(f);
        free(copy);
        return 0;
    }
    f->err.file[0] = '\0'; /* the file holds no error */
    free(copy);
    *out = f;
    if (err != NULL)
        memset(err, 0, sizeof(*err));
    return 1;
}

int
golden_open(const char *path, size_t max_input, golden_file **out, golden_error_t *err)
{
    FILE *fp;
    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;
    int ok;

    if (out != NULL)
        *out = NULL;
    if (out == NULL || path == NULL)
    {
        golden_error_set(err, path, 0, 0, "a null argument");
        return 0;
    }
    fp = fopen(path, "rb");
    if (fp == NULL)
    {
        golden_error_set(err, path, 0, 0, "cannot open the file for reading");
        return 0;
    }
    for (;;)
    {
        size_t got;

        if (len + 65536 + 1 > cap)
        {
            size_t ncap = cap == 0 ? 131072 : 2 * cap;
            char *nbuf = (char *) realloc(buf, ncap);

            if (nbuf == NULL)
            {
                free(buf);
                fclose(fp);
                golden_error_set(err, path, 0, 0, "out of memory");
                return 0;
            }
            buf = nbuf;
            cap = ncap;
        }
        got = fread(buf + len, 1, 65536, fp);
        len += got;
        if (got < 65536)
            break;
    }
    if (ferror(fp))
    {
        free(buf);
        fclose(fp);
        golden_error_set(err, path, 0, 0, "error while reading the file");
        return 0;
    }
    fclose(fp);
    ok = golden_parse(buf, len, path, max_input, out, err);
    free(buf);
    return ok;
}

void
golden_close(golden_file *f)
{
    size_t i;

    if (f == NULL)
        return;
    for (i = 0; i < f->nrecords; i++)
    {
        free(f->records[i].input);
        free(f->records[i].expected);
    }
    free(f->records);
    free(f->name);
    free(f);
}

size_t
golden_count(const golden_file *f)
{
    return f == NULL ? 0 : f->nrecords;
}

const golden_record *
golden_record_at(const golden_file *f, size_t index)
{
    if (f == NULL || index >= f->nrecords)
        return NULL;
    return &f->records[index];
}

const golden_error_t *
golden_file_error(const golden_file *f)
{
    if (f == NULL || !f->failed)
        return NULL;
    return &f->err;
}

const char *
golden_error_message(const golden_error_t *err)
{
    static char buf[512];

    if (err == NULL)
        return "(no error)";
    snprintf(buf, sizeof(buf), "%s:%lu:%lu: %s", err->file, err->line, err->column, err->message);
    return buf;
}

size_t
golden_comment_lines(const golden_file *f)
{
    return f == NULL ? 0 : f->comments;
}

size_t
golden_blank_lines(const golden_file *f)
{
    return f == NULL ? 0 : f->blanks;
}
