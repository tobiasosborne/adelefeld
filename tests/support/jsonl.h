/* tests/support/jsonl.h: a strict reader for the JSON-lines vector files under
   tests/ref/vectors/ (C11, libc only; FLINT is not needed to read a file).

   Each line of a vector file is one JSON object (tests/ref/README.md, "Vector format"). The
   reader is deliberately narrow: it accepts exactly what those files use, and rejects
   everything else with the file name, the line and the column, so that no error can pass
   unnoticed. The rules, in full:

   - One value per line. Inside a value the only white space is space and tab: a newline ends
     the record, and a newline inside a value is an error. After the value only spaces and
     tabs may follow, then LF or end of file. A blank line is an error, not a skipped line,
     and so is a file with no line at all.
   - The value may be an object, an array, a string, a number, `true`, `false` or `null`.
   - A number is an integer literal: `-? (0 | [1-9][0-9]*)`. A fraction, an exponent, a
     leading `+` and a leading zero are errors, because no vector file has one and a vector
     file with one is not the fixture the tests were written against.
   - Inside a string, `\" \\ \/ \b \f \n \r \t \uXXXX` are the escapes; `\uXXXX` is encoded as
     UTF-8 and a surrogate pair is joined. A raw byte below 0x20 is an error. Bytes at and
     above 0x80 are passed through; the reader does not check UTF-8.
   - Keys of an object are strings and are unique; a repeated key is an error.
   - Nesting deeper than JSONL_MAX_DEPTH is an error, so that a hostile file cannot make the
     recursive descent run out of stack.
   - Trailing bytes after the value on the line are an error.

   Numbers are kept as text, not as a machine integer: `jsonl_int_text` returns the literal
   as it stands, and the caller passes it to `fmpz_set_str(x, text, 10)`. A vector file that
   carried a thousand-digit integer would then be read without loss. The same accessor
   `jsonl_int_text_or_string` also accepts an integer written as a JSON string, which some
   generators use for very large values; it rejects a string that is not an integer literal.

   Use:

       jsonl_error_t err;
       jsonl_file *f;
       const jsonl_value *rec, *v;

       if (!jsonl_open("tests/ref/vectors/add.jsonl", &f, &err))
           printf("%s\n", jsonl_error_message(&err));
       for (i = 0; i < jsonl_count(f); i++)
       {
           rec = jsonl_record(f, i);
           if (!jsonl_field(rec, "a", &v, &err)) { ... }
           ...
       }
       jsonl_close(f);

   Every accessor that can fail takes a `jsonl_error_t *` and fills it; a caller that passes
   NULL throws the message away, which the tests of tests/test_support.c never do. A failing
   jsonl_open or jsonl_parse sets *out to NULL and returns 0.

   Pointers into a jsonl_file stay valid until jsonl_close, and not one moment longer. */

#ifndef ADF_TESTS_SUPPORT_JSONL_H
#define ADF_TESTS_SUPPORT_JSONL_H

#include <stddef.h>

/* The deepest nesting the reader accepts. The vector files nest three levels. */

#define JSONL_MAX_DEPTH 64

typedef enum
{
    JSONL_NULL,
    JSONL_BOOL,
    JSONL_INT,
    JSONL_STR,
    JSONL_ARRAY,
    JSONL_OBJECT
} jsonl_kind_t;

/* Where a value stands in the file, and what went wrong there. file is the name given to the
   reader (the path for jsonl_open) and stays valid as long as the file is open. */

/* The name of a path is at most this many bytes; a longer one is cut, since the error is for
   a person to read. The name is a copy, not a pointer into the reader, so that the error of a
   failed read stays readable after the reader has been closed. */

#define JSONL_FILE_MAX 256

typedef struct
{
    char file[JSONL_FILE_MAX];
    unsigned long line;
    unsigned long column;
    char message[200];
} jsonl_error_t;

typedef struct jsonl_file jsonl_file;
typedef struct jsonl_value jsonl_value;

/* Read a whole vector file. Returns 1 on success, 0 with *out set to NULL and err filled on
   any error at all, including one on the last line. */

int jsonl_open(const char *path, jsonl_file **out, jsonl_error_t *err);

/* The same, on a buffer of len bytes, with name as the file name in the messages. This is
   what the fuzzer calls. */

int jsonl_parse(const char *text, size_t len, const char *name, jsonl_file **out, jsonl_error_t *err);

void jsonl_close(jsonl_file *f);

/* The number of lines, that is of records, of an open file; 0 if none. */

size_t jsonl_count(const jsonl_file *f);

/* Record index, counted from 0. NULL if index is out of range. */

const jsonl_value *jsonl_record(const jsonl_file *f, size_t index);

/* The error of a failed open, or of a successful one: NULL. */

const jsonl_error_t *jsonl_file_error(const jsonl_file *f);

/* "file:line:column: message", in a static buffer. The buffer is overwritten by the next
   call. A NULL error gives "(no error)". */

const char *jsonl_error_message(const jsonl_error_t *err);

/* The kind of a value, and its name for a message. Never fails. */

jsonl_kind_t jsonl_kind(const jsonl_value *v);
const char *jsonl_kind_name(const jsonl_value *v);

/* 1 if the value has that kind, 0 otherwise, with no error: this is the question one asks
   before deciding what to do. */

int jsonl_is(const jsonl_value *v, jsonl_kind_t kind);

/* The number of members of an object or of elements of an array; 0 for a scalar. */

size_t jsonl_size(const jsonl_value *v);

/* The key of member index of an object, and the element at index of an array or object.
   Out of range gives NULL and fills err for jsonl_at. */

const char *jsonl_key(const jsonl_value *obj, size_t index);
const jsonl_value *jsonl_at(const jsonl_value *v, size_t index, jsonl_error_t *err);

/* The member of an object with that key. Returns 1 and writes *out, or returns 0 with err
   filled: a missing key or a value that is not an object is an error, never a silent zero. */

int jsonl_field(const jsonl_value *obj, const char *key, const jsonl_value **out, jsonl_error_t *err);

/* The bytes of a string, unescaped, NUL-terminated (a decoded NUL is a byte in the middle, so
   *len is needed) and owned by the file. Returns NULL with err filled if not a string. */

const char *jsonl_string(const jsonl_value *v, size_t *len, jsonl_error_t *err);

/* The literal of an integer, for example "-17"; fmpz_set_str(x, text, 10) reads it.
   Returns NULL with err filled if not an integer. */

const char *jsonl_int_text(const jsonl_value *v, jsonl_error_t *err);

/* As jsonl_int_text, but a JSON string holding an integer literal is accepted too. Returns 1
   and writes *text, or returns 0 with err filled. */

int jsonl_int_text_or_string(const jsonl_value *v, const char **text, jsonl_error_t *err);

/* The value of a boolean. Returns 1 and writes *out, or 0 with err filled. */

int jsonl_bool(const jsonl_value *v, int *out, jsonl_error_t *err);

/* 1 if the value is null, 0 if it is not (err filled), as a null is a value in these files:
   `status: null` is a claim the caller has to notice. */

int jsonl_is_null(const jsonl_value *v, jsonl_error_t *err);

/* Where a value stands: the name of the file and its line, 1 for the first. */

const char *jsonl_file_of(const jsonl_value *v);
unsigned long jsonl_line_of(const jsonl_value *v);

#endif /* ADF_TESTS_SUPPORT_JSONL_H */
