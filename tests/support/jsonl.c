/* tests/support/jsonl.c: the strict JSON-lines reader of tests/support/jsonl.h.

   C11, libc only. The rules it enforces are listed in the header. The parser is a recursive
   descent over one line at a time; every value is an entry of one growable array, so that
   closing a file is a free of the array and of the strings. The recursion is bounded by
   JSONL_MAX_DEPTH. Nothing here is ignored: every path that fails fills the error with the
   file, the line and the column and stops the parse. */

#include "jsonl.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct jsonl_value
{
    jsonl_kind_t kind;
    struct jsonl_file *owner; /* the file the value belongs to; holds the array of values */
    const char *file;
    unsigned long line;
    unsigned long column;
    size_t first_child; /* an array or an object: its first child, or JSONL_NO_INDEX */
    size_t last_child;  /* an array or an object: its last child, for appending */
    size_t next;        /* the next sibling, or JSONL_NO_INDEX */
    char *text;    /* string: the decoded bytes, NUL-terminated; integer: the literal;
                      NULL otherwise */
    char *key;     /* object: the key of this member; NULL otherwise */
    size_t len;    /* string: the number of decoded bytes, a NUL inside included */
    int boolean;
};

/* The index of no value. */

#define JSONL_NO_INDEX ((size_t) -1)

struct jsonl_file
{
    char *name;
    jsonl_value *items;   /* every value of the file */
    size_t nitems;
    size_t cap;
    size_t *records;     /* the index of the value of each line */
    size_t nrecords;
    size_t caprecords;
    jsonl_error_t err;
    int failed;
};

/* The parser state. line and column follow the bytes consumed, so that the error points at
   the offending byte and not at the start of the line. */

typedef struct
{
    jsonl_file *f;
    const char *p;
    const char *end;
    unsigned long line;
    unsigned long column;
    int depth;
    jsonl_error_t err;
} jctx;

static void
jsonl_error_set(jsonl_error_t *err, const char *file, unsigned long line, unsigned long column,
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

/* Record the error in the parser state and in the file, and return 0 so that the caller can
   write `return fail(...)`. The parse of the file stops at the first error. */

static int
jsonl_fail(jctx *c, const char *fmt, ...)
{
    va_list ap;

    if (!c->f->failed)
    {
        c->f->failed = 1;
        va_start(ap, fmt);
        vsnprintf(c->f->err.message, sizeof(c->f->err.message), fmt, ap);
        va_end(ap);
        snprintf(c->f->err.file, sizeof(c->f->err.file), "%s",
                 c->f->name == NULL ? "(no file)" : c->f->name);
        c->f->err.line = c->line;
        c->f->err.column = c->column;
    }
    va_start(ap, fmt);
    vsnprintf(c->err.message, sizeof(c->err.message), fmt, ap);
    va_end(ap);
    snprintf(c->err.file, sizeof(c->err.file), "%s", c->f->name == NULL ? "(no file)" : c->f->name);
    c->err.line = c->line;
    c->err.column = c->column;
    return 0;
}

/* One byte of input consumed. */

static void
jsonl_advance(jctx *c, size_t n)
{
    size_t i;

    for (i = 0; i < n && c->p < c->end; i++)
    {
        if (*c->p == '\n')
        {
            c->line++;
            c->column = 1;
        }
        else
            c->column++;
        c->p++;
    }
}

static int
jsonl_at_end(const jctx *c)
{
    return c->p >= c->end;
}

static int
jsonl_peek(const jctx *c)
{
    return jsonl_at_end(c) ? -1 : (unsigned char) *c->p;
}

static int
jsonl_eat(jctx *c, int ch)
{
    if (jsonl_peek(c) != ch)
        return 0;
    jsonl_advance(c, 1);
    return 1;
}

/* Space and tab only: a newline ends the record. */

static void
jsonl_skip_blanks(jctx *c)
{
    while (!jsonl_at_end(c) && (*c->p == ' ' || *c->p == '\t'))
        jsonl_advance(c, 1);
}

/* Add an empty value of that kind and return its index, or SIZE_MAX after an error. */

static size_t
jsonl_new_value(jctx *c, jsonl_kind_t kind)
{
    jsonl_file *f = c->f;
    jsonl_value *v;

    if (f->nitems == f->cap)
    {
        size_t cap = f->cap == 0 ? 64 : 2 * f->cap;
        jsonl_value *items = (jsonl_value *) realloc(f->items, cap * sizeof(jsonl_value));

        if (items == NULL)
        {
            jsonl_fail(c, "out of memory");
            return JSONL_NO_INDEX;
        }
        f->items = items;
        f->cap = cap;
    }
    v = &f->items[f->nitems];
    memset(v, 0, sizeof(*v));
    v->kind = kind;
    v->owner = f;
    v->file = f->name;
    v->line = c->line;
    v->column = c->column;
    v->first_child = JSONL_NO_INDEX;
    v->last_child = JSONL_NO_INDEX;
    v->next = JSONL_NO_INDEX;
    return f->nitems++;
}

/* The value at an index, for the parser; it never moves while the parse runs, because the
   array is only appended to. */

static jsonl_value *
jsonl_at_index(jsonl_file *f, size_t index)
{
    return &f->items[index];
}

static int jsonl_parse_value(jctx *c, size_t *out);

/* A string literal. The decoded bytes go into a fresh buffer of the value; *len receives the
   number of decoded bytes. Returns 1 on success. */

static int
jsonl_parse_string(jctx *c, char **out, size_t *out_len, size_t *out_cap)
{
    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;

#define JSONL_PUT(byte)                                                            \
    do                                                                             \
    {                                                                              \
        if (len + 2 > cap)                                                         \
        {                                                                          \
            size_t ncap = cap == 0 ? 32 : 2 * cap;                                 \
            char *nbuf = (char *) realloc(buf, ncap);                              \
            if (nbuf == NULL)                                                      \
            {                                                                      \
                free(buf);                                                         \
                return jsonl_fail(c, "out of memory");                             \
            }                                                                      \
            buf = nbuf;                                                            \
            cap = ncap;                                                            \
        }                                                                          \
        buf[len++] = (char) (byte);                                                \
    } while (0)

#define JSONL_PUT_CODE(cp)                                                         \
    do                                                                             \
    {                                                                              \
        unsigned long u_ = (unsigned long) (cp);                                   \
        if (u_ < 0x80UL)                                                           \
            JSONL_PUT(u_);                                                         \
        else if (u_ < 0x800UL)                                                     \
        {                                                                          \
            JSONL_PUT(0xC0UL | (u_ >> 6));                                         \
            JSONL_PUT(0x80UL | (u_ & 0x3FUL));                                      \
        }                                                                          \
        else if (u_ < 0x10000UL)                                                   \
        {                                                                          \
            JSONL_PUT(0xE0UL | (u_ >> 12));                                        \
            JSONL_PUT(0x80UL | ((u_ >> 6) & 0x3FUL));                               \
            JSONL_PUT(0x80UL | (u_ & 0x3FUL));                                      \
        }                                                                          \
        else                                                                       \
        {                                                                          \
            JSONL_PUT(0xF0UL | (u_ >> 18));                                        \
            JSONL_PUT(0x80UL | ((u_ >> 12) & 0x3FUL));                              \
            JSONL_PUT(0x80UL | ((u_ >> 6) & 0x3FUL));                               \
            JSONL_PUT(0x80UL | (u_ & 0x3FUL));                                      \
        }                                                                          \
    } while (0)

    if (!jsonl_eat(c, '"'))
        return jsonl_fail(c, "expected a string");

    for (;;)
    {
        int ch = jsonl_peek(c);

        if (ch < 0)
        {
            free(buf);
            return jsonl_fail(c, "unterminated string");
        }
        if (ch == '"')
        {
            jsonl_advance(c, 1);
            break;
        }
        if (ch < 0x20)
        {
            free(buf);
            return jsonl_fail(c, "raw byte 0x%02x below 0x20 in a string", ch);
        }
        if (ch != '\\')
        {
            JSONL_PUT(ch);
            jsonl_advance(c, 1);
            continue;
        }
        jsonl_advance(c, 1);
        ch = jsonl_peek(c);
        if (ch < 0)
        {
            free(buf);
            return jsonl_fail(c, "a backslash at the end of a string");
        }
        jsonl_advance(c, 1);
        switch (ch)
        {
        case '"':
            JSONL_PUT('"');
            break;
        case '\\':
            JSONL_PUT('\\');
            break;
        case '/':
            JSONL_PUT('/');
            break;
        case 'b':
            JSONL_PUT('\b');
            break;
        case 'f':
            JSONL_PUT('\f');
            break;
        case 'n':
            JSONL_PUT('\n');
            break;
        case 'r':
            JSONL_PUT('\r');
            break;
        case 't':
            JSONL_PUT('\t');
            break;
        case 'u':
        {
            unsigned long u = 0;
            int i;

            for (i = 0; i < 4; i++)
            {
                int h = jsonl_peek(c);
                int d;

                if (h >= '0' && h <= '9')
                    d = h - '0';
                else if (h >= 'a' && h <= 'f')
                    d = h - 'a' + 10;
                else if (h >= 'A' && h <= 'F')
                    d = h - 'A' + 10;
                else
                {
                    free(buf);
                    return jsonl_fail(c, "\\u needs four hexadecimal digits");
                }
                u = 16 * u + (unsigned long) d;
                jsonl_advance(c, 1);
            }
            if (u >= 0xD800UL && u <= 0xDBFFUL)
            {
                unsigned long low = 0;

                if (!jsonl_eat(c, '\\') || !jsonl_eat(c, 'u'))
                {
                    free(buf);
                    return jsonl_fail(c, "a high surrogate must be followed by \\uDC00-\\uDFFF");
                }
                for (i = 0; i < 4; i++)
                {
                    int h = jsonl_peek(c);
                    int d;

                    if (h >= '0' && h <= '9')
                        d = h - '0';
                    else if (h >= 'a' && h <= 'f')
                        d = h - 'a' + 10;
                    else if (h >= 'A' && h <= 'F')
                        d = h - 'A' + 10;
                    else
                    {
                        free(buf);
                        return jsonl_fail(c, "\\u needs four hexadecimal digits");
                    }
                    low = 16 * low + (unsigned long) d;
                    jsonl_advance(c, 1);
                }
                if (low < 0xDC00UL || low > 0xDFFFUL)
                {
                    free(buf);
                    return jsonl_fail(c, "a high surrogate must be followed by \\uDC00-\\uDFFF");
                }
                u = 0x10000UL + ((u - 0xD800UL) << 10) + (low - 0xDC00UL);
            }
            else if (u >= 0xDC00UL && u <= 0xDFFFUL)
            {
                free(buf);
                return jsonl_fail(c, "a low surrogate without a high surrogate");
            }
            JSONL_PUT_CODE(u);
            break;
        }
        default:
            free(buf);
            return jsonl_fail(c, "unknown escape \\%c", ch);
        }
    }

    if (len + 1 > cap)
    {
        size_t ncap = cap == 0 ? 1 : 2 * cap;
        char *nbuf = (char *) realloc(buf, ncap);

        if (nbuf == NULL)
        {
            free(buf);
            return jsonl_fail(c, "out of memory");
        }
        buf = nbuf;
        cap = ncap;
    }
    buf[len] = '\0';
    *out = buf;
    *out_len = len;
    *out_cap = cap;
    return 1;

#undef JSONL_PUT
#undef JSONL_PUT_CODE
}

/* An integer literal. The literal is copied out as it stands, sign included, and is the text
   the caller gives to fmpz_set_str. */

static int
jsonl_parse_int(jctx *c, char **out, size_t *out_len)
{
    const char *start = c->p;
    size_t n = 0;

    if (jsonl_peek(c) == '-')
    {
        jsonl_advance(c, 1);
        n++;
    }
    if (jsonl_at_end(c))
        return jsonl_fail(c, "a number needs a digit");
    if (jsonl_peek(c) == '0')
    {
        jsonl_advance(c, 1);
        n++;
        if (!jsonl_at_end(c) && jsonl_peek(c) >= '0' && jsonl_peek(c) <= '9')
            return jsonl_fail(c, "a number with a leading zero");
    }
    else if (jsonl_peek(c) >= '1' && jsonl_peek(c) <= '9')
    {
        while (!jsonl_at_end(c) && jsonl_peek(c) >= '0' && jsonl_peek(c) <= '9')
        {
            jsonl_advance(c, 1);
            n++;
        }
    }
    else
        return jsonl_fail(c, "a number needs a digit");

    /* A fraction or an exponent would not be an integer; the vector files have none. */

    if (!jsonl_at_end(c) && (*c->p == '.' || *c->p == 'e' || *c->p == 'E'))
        return jsonl_fail(c, "only integer numbers are read");

    *out_len = n;
    *out = (char *) malloc(n + 1);
    if (*out == NULL)
        return jsonl_fail(c, "out of memory");
    memcpy(*out, start, n);
    (*out)[n] = '\0';
    return 1;
}

/* Parse one value; *out is its index. */

static int
jsonl_parse_value(jctx *c, size_t *out)
{
    int ch = jsonl_peek(c);
    size_t index;

    if (c->depth >= JSONL_MAX_DEPTH)
        return jsonl_fail(c, "nested deeper than %d", JSONL_MAX_DEPTH);
    if (ch < 0)
        return jsonl_fail(c, "expected a value, found the end of the line");

    if (ch == '{' || ch == '[')
    {
        int is_object = (ch == '{');
        int close = is_object ? '}' : ']';

        index = jsonl_new_value(c, is_object ? JSONL_OBJECT : JSONL_ARRAY);
        if (index == JSONL_NO_INDEX)
            return 0;
        jsonl_advance(c, 1);
        c->depth++;
        jsonl_skip_blanks(c);
        if (jsonl_eat(c, close))
        {
            c->depth--;
            *out = index;
            return 1;
        }
        for (;;)
        {
            char *key = NULL;
            size_t key_len = 0;
            size_t key_cap = 0;
            size_t child = 0;
            size_t i;
            jsonl_value *parent;

            if (is_object)
            {
                if (jsonl_peek(c) != '"')
                {
                    jsonl_fail(c, "a key must be a string");
                    c->depth--;
                    return 0;
                }
                if (!jsonl_parse_string(c, &key, &key_len, &key_cap))
                {
                    c->depth--;
                    return 0;
                }
                for (i = jsonl_at_index(c->f, index)->first_child; i != JSONL_NO_INDEX;
                     i = jsonl_at_index(c->f, i)->next)
                {
                    const char *other = jsonl_at_index(c->f, i)->key;

                    if (other != NULL && strcmp(other, key) == 0)
                    {
                        jsonl_fail(c, "the key \"%s\" is given twice", key);
                        free(key);
                        c->depth--;
                        return 0;
                    }
                }
                jsonl_skip_blanks(c);
                if (!jsonl_eat(c, ':'))
                {
                    free(key);
                    jsonl_fail(c, "expected ':' after a key");
                    c->depth--;
                    return 0;
                }
                jsonl_skip_blanks(c);
            }
            if (!jsonl_parse_value(c, &child))
            {
                free(key);
                c->depth--;
                return 0;
            }
            /* append the child to the list of children of the container; the values of every
               level share one array, so the children are a list and not a range */
            parent = jsonl_at_index(c->f, index);
            jsonl_at_index(c->f, child)->key = key;
            if (parent->last_child == JSONL_NO_INDEX)
                parent->first_child = child;
            else
                jsonl_at_index(c->f, parent->last_child)->next = child;
            parent->last_child = child;
            jsonl_skip_blanks(c);
            if (jsonl_eat(c, ','))
            {
                jsonl_skip_blanks(c);
                continue;
            }
            if (jsonl_eat(c, close))
                break;
            jsonl_fail(c, "expected ',' or '%c' in a value", close);
            c->depth--;
            return 0;
        }
        c->depth--;
        *out = index;
        return 1;
    }

    if (ch == '"')
    {
        char *text = NULL;
        size_t len = 0;
        size_t cap = 0;

        index = jsonl_new_value(c, JSONL_STR);
        if (index == JSONL_NO_INDEX)
            return 0;
        if (!jsonl_parse_string(c, &text, &len, &cap))
            return 0;
        jsonl_at_index(c->f, index)->text = text;
        jsonl_at_index(c->f, index)->len = len;
        *out = index;
        return 1;
    }

    if (ch == 't' || ch == 'f' || ch == 'n')
    {
        const char *word = (ch == 't') ? "true" : (ch == 'f' ? "false" : "null");
        size_t wlen = strlen(word);
        jsonl_kind_t kind = (ch == 't') ? JSONL_BOOL : (ch == 'f' ? JSONL_BOOL : JSONL_NULL);

        if ((size_t) (c->end - c->p) < wlen || memcmp(c->p, word, wlen) != 0)
            return jsonl_fail(c, "expected %s", word);
        jsonl_advance(c, wlen);
        index = jsonl_new_value(c, kind);
        if (index == JSONL_NO_INDEX)
            return 0;
        jsonl_at_index(c->f, index)->boolean = (ch == 't');
        *out = index;
        return 1;
    }

    if (ch == '-' || (ch >= '0' && ch <= '9'))
    {
        char *text = NULL;
        size_t len = 0;

        index = jsonl_new_value(c, JSONL_INT);
        if (index == JSONL_NO_INDEX)
            return 0;
        if (!jsonl_parse_int(c, &text, &len))
            return 0;
        jsonl_at_index(c->f, index)->text = text;
        jsonl_at_index(c->f, index)->len = len;
        *out = index;
        return 1;
    }

    return jsonl_fail(c, "unexpected byte 0x%02x", ch);
}

int
jsonl_parse(const char *text, size_t len, const char *name, jsonl_file **out, jsonl_error_t *err)
{
    jsonl_file *f;
    jctx c;
    char *copy;

    if (out != NULL)
        *out = NULL;
    if (out == NULL || text == NULL || name == NULL)
    {
        jsonl_error_set(err, name, 0, 0, "a null argument");
        return 0;
    }

    f = (jsonl_file *) calloc(1, sizeof(*f));
    copy = (char *) malloc(len + 1);
    if (f == NULL || copy == NULL)
    {
        free(f);
        free(copy);
        jsonl_error_set(err, name, 0, 0, "out of memory");
        return 0;
    }
    memcpy(copy, text, len);
    copy[len] = '\0';
    /* The name is copied as well: it may be a buffer of the caller, and the error messages
       of the values outlive the call. */
    f->name = (char *) malloc(strlen(name) + 1);
    if (f->name == NULL)
    {
        free(f);
        free(copy);
        jsonl_error_set(err, name, 0, 0, "out of memory");
        return 0;
    }
    memcpy(f->name, name, strlen(name) + 1);
    f->items = NULL;
    f->nitems = 0;
    f->cap = 0;
    f->records = NULL;
    f->nrecords = 0;
    f->caprecords = 0;
    f->failed = 0;

    c.f = f;
    c.p = copy;
    c.end = copy + len;
    c.line = 1;
    c.column = 1;
    c.depth = 0;
    memset(&c.err, 0, sizeof(c.err));

    if (len == 0)
        jsonl_fail(&c, "the file is empty");

    while (c.p < c.end)
    {
        size_t index = 0;
        const char *start = c.p;

        jsonl_skip_blanks(&c);
        if (c.p >= c.end)
        {
            if (c.p != start)
                jsonl_fail(&c, "a line with only white space");
            break;
        }
        if (*c.p == '\n')
        {
            jsonl_fail(&c, "an empty line");
            break;
        }
        if (!jsonl_parse_value(&c, &index))
            break;
        if (f->nrecords == f->caprecords)
        {
            size_t cap = f->caprecords == 0 ? 64 : 2 * f->caprecords;
            size_t *rec = (size_t *) realloc(f->records, cap * sizeof(size_t));

            if (rec == NULL)
            {
                jsonl_fail(&c, "out of memory");
                break;
            }
            f->records = rec;
            f->caprecords = cap;
        }
        f->records[f->nrecords++] = index;
        jsonl_skip_blanks(&c);
        if (c.p < c.end && *c.p != '\n')
        {
            jsonl_fail(&c, "trailing bytes after the value");
            break;
        }
        if (c.p < c.end)
            jsonl_advance(&c, 1);
    }

    /* the buffer is only needed while parsing: every value owns its bytes */
    free(copy);
    if (f->failed)
    {
        if (err != NULL)
            *err = f->err;
        jsonl_close(f);
        return 0;
    }
    *out = f;
    if (err != NULL)
        memset(err, 0, sizeof(*err));
    return 1;
}

int
jsonl_open(const char *path, jsonl_file **out, jsonl_error_t *err)
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
        jsonl_error_set(err, path, 0, 0, "a null argument");
        return 0;
    }
    fp = fopen(path, "rb");
    if (fp == NULL)
    {
        jsonl_error_set(err, path, 0, 0, "cannot open the file for reading");
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
                jsonl_error_set(err, path, 0, 0, "out of memory");
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
        jsonl_error_set(err, path, 0, 0, "error while reading the file");
        return 0;
    }
    fclose(fp);
    ok = jsonl_parse(buf, len, path, out, err);
    free(buf);
    return ok;
}

void
jsonl_close(jsonl_file *f)
{
    size_t i;

    if (f == NULL)
        return;
    for (i = 0; i < f->nitems; i++)
    {
        free(f->items[i].text);
        free(f->items[i].key);
    }
    free(f->items);
    free(f->records);
    free(f->name);
    free(f);
}

size_t
jsonl_count(const jsonl_file *f)
{
    if (f == NULL)
        return 0;
    return f->nrecords;
}

const jsonl_value *
jsonl_record(const jsonl_file *f, size_t index)
{
    if (f == NULL || index >= f->nrecords)
        return NULL;
    return &f->items[f->records[index]];
}

const jsonl_error_t *
jsonl_file_error(const jsonl_file *f)
{
    if (f == NULL || !f->failed)
        return NULL;
    return &f->err;
}

const char *
jsonl_error_message(const jsonl_error_t *err)
{
    static char buf[512];

    if (err == NULL)
        return "(no error)";
    snprintf(buf, sizeof(buf), "%s:%lu:%lu: %s", err->file, err->line, err->column, err->message);
    return buf;
}

jsonl_kind_t
jsonl_kind(const jsonl_value *v)
{
    return v == NULL ? JSONL_NULL : v->kind;
}

const char *
jsonl_kind_name(const jsonl_value *v)
{
    static const char *const names[] = {"null", "bool", "int", "string", "array", "object"};

    if (v == NULL)
        return "(no value)";
    return names[v->kind];
}

int
jsonl_is(const jsonl_value *v, jsonl_kind_t kind)
{
    return v != NULL && v->kind == kind;
}

/* The number of children of an array or an object. The children are a list, so this walks
   it; the values of the vector files have at most four children. */

size_t
jsonl_size(const jsonl_value *v)
{
    size_t i;
    size_t n = 0;

    if (v == NULL)
        return 0;
    for (i = v->first_child; i != JSONL_NO_INDEX; i = v->owner->items[i].next)
        n++;
    return n;
}

const char *
jsonl_key(const jsonl_value *obj, size_t index)
{
    size_t i;

    if (!jsonl_is(obj, JSONL_OBJECT))
        return NULL;
    for (i = obj->first_child; i != JSONL_NO_INDEX; i = obj->owner->items[i].next)
    {
        if (index == 0)
            return obj->owner->items[i].key;
        index--;
    }
    return NULL;
}

const jsonl_value *
jsonl_at(const jsonl_value *v, size_t index, jsonl_error_t *err)
{
    size_t i;

    if (!jsonl_is(v, JSONL_ARRAY) && !jsonl_is(v, JSONL_OBJECT))
    {
        jsonl_error_set(err, v == NULL ? NULL : v->file, 0, 0, "a %s has no elements",
                        jsonl_kind_name(v));
        return NULL;
    }
    for (i = v->first_child; i != JSONL_NO_INDEX; i = v->owner->items[i].next)
    {
        if (index == 0)
            return &v->owner->items[i];
        index--;
    }
    jsonl_error_set(err, v->file, v->line, v->column, "index %lu is out of range",
                    (unsigned long) index);
    return NULL;
}

int
jsonl_field(const jsonl_value *obj, const char *key, const jsonl_value **out, jsonl_error_t *err)
{
    size_t i;

    if (out != NULL)
        *out = NULL;
    if (!jsonl_is(obj, JSONL_OBJECT) || key == NULL)
    {
        jsonl_error_set(err, obj == NULL ? NULL : obj->file, obj == NULL ? 0 : obj->line, 0,
                        "a %s has no member \"%s\"", jsonl_kind_name(obj), key == NULL ? "" : key);
        return 0;
    }
    for (i = obj->first_child; i != JSONL_NO_INDEX; i = obj->owner->items[i].next)
    {
        const jsonl_value *child = &obj->owner->items[i];

        if (strcmp(child->key, key) == 0)
        {
            *out = child;
            return 1;
        }
    }
    jsonl_error_set(err, obj->file, obj->line, obj->column, "the key \"%s\" is missing", key);
    return 0;
}

const char *
jsonl_string(const jsonl_value *v, size_t *len, jsonl_error_t *err)
{
    if (!jsonl_is(v, JSONL_STR))
    {
        jsonl_error_set(err, v == NULL ? NULL : v->file, v == NULL ? 0 : v->line, 0,
                        "expected a string, found a %s", jsonl_kind_name(v));
        return NULL;
    }
    if (len != NULL)
        *len = v->len;
    return v->text;
}

const char *
jsonl_int_text(const jsonl_value *v, jsonl_error_t *err)
{
    if (!jsonl_is(v, JSONL_INT))
    {
        jsonl_error_set(err, v == NULL ? NULL : v->file, v == NULL ? 0 : v->line, 0,
                        "expected an integer, found a %s", jsonl_kind_name(v));
        return NULL;
    }
    return v->text;
}

int
jsonl_int_text_or_string(const jsonl_value *v, const char **text, jsonl_error_t *err)
{
    if (text != NULL)
        *text = NULL;
    if (jsonl_is(v, JSONL_INT))
    {
        if (text != NULL)
            *text = v->text;
        return 1;
    }
    if (jsonl_is(v, JSONL_STR))
    {
        const char *s = v->text;
        size_t i = 0;

        if (*s == '-')
            i = 1;
        if (s[i] == '\0' || s[i] < '0' || s[i] > '9')
        {
            jsonl_error_set(err, v->file, v->line, v->column, "the string is not an integer");
            return 0;
        }
        if (s[i] == '0' && s[i + 1] != '\0')
        {
            jsonl_error_set(err, v->file, v->line, v->column, "the string has a leading zero");
            return 0;
        }
        for (i++; s[i] != '\0'; i++)
        {
            if (s[i] < '0' || s[i] > '9')
            {
                jsonl_error_set(err, v->file, v->line, v->column, "the string is not an integer");
                return 0;
            }
        }
        if (text != NULL)
            *text = s;
        return 1;
    }
    jsonl_error_set(err, v == NULL ? NULL : v->file, v == NULL ? 0 : v->line, 0,
                    "expected an integer or a string, found a %s", jsonl_kind_name(v));
    return 0;
}

int
jsonl_bool(const jsonl_value *v, int *out, jsonl_error_t *err)
{
    if (!jsonl_is(v, JSONL_BOOL))
    {
        jsonl_error_set(err, v == NULL ? NULL : v->file, v == NULL ? 0 : v->line, 0,
                        "expected a boolean, found a %s", jsonl_kind_name(v));
        return 0;
    }
    *out = v->boolean;
    return 1;
}

int
jsonl_is_null(const jsonl_value *v, jsonl_error_t *err)
{
    if (!jsonl_is(v, JSONL_NULL))
    {
        jsonl_error_set(err, v == NULL ? NULL : v->file, v == NULL ? 0 : v->line, 0,
                        "expected null, found a %s", jsonl_kind_name(v));
        return 0;
    }
    return 1;
}

const char *
jsonl_file_of(const jsonl_value *v)
{
    return v == NULL ? NULL : v->file;
}

unsigned long
jsonl_line_of(const jsonl_value *v)
{
    return v == NULL ? 0 : v->line;
}
