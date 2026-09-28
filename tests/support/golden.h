/* tests/support/golden.h: a reader for the golden files under tests/golden/ (C11, libc only).

   The format is that of docs/conventions.md section 11.1: ASCII, LF line ends, one vector per
   line, `input<TAB>expected`, a line starting with `#` is a comment, an empty line is ignored.
   In `input` a backslash starts an escape: `\\` backslash, `\t` TAB, `\n` LF, `\r` CR, `\xHH`
   the byte with that hexadecimal value. Any other backslash sequence is an error in the file.
   An input of the form `@gen:PREFIX|UNIT|COUNT|SUFFIX` stands for `PREFIX` followed by `COUNT`
   copies of `UNIT` followed by `SUFFIX`; the four parts carry the same escapes, and a literal
   `|` inside a part is written `\x7c`. That is how the files carry an input of 1048577 bytes
   (tests/golden/dispatch.tsv) on one line.

   `expected` is not escaped: it is the canonical output text, or `!` followed by a status name
   (conventions.md 11.1). The reader tells the two apart and puts the name, without the `!`, in
   record->status.

   A decoded input may hold a NUL in the middle (tests/golden/adele.tsv has `(1 ; 0)\x00`) and
   bytes at and above 0x80 (the UTF-8 minus sign `\xc2\xb1`). So every string is a pointer and
   a length; the pointer is NUL-terminated as well, for the callers that want a C string, but a
   caller that reads an input must use input_len. The reader does not check UTF-8.

   `COUNT` copies of `UNIT` are materialised, so a line that would expand past max_input bytes
   is refused before anything is allocated. GOLDEN_DEFAULT_MAX_INPUT is above the largest
   vector in the files (1048577). An error is reported with the file, the line and the column
   of the offending byte, and never ignored: a failing golden_open or golden_parse sets *out to
   NULL.

   Use:

       golden_error_t err;
       golden_file *f;
       const golden_record *r;

       if (!golden_open("tests/golden/rat.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err))
           printf("%s\n", golden_error_message(&err));
       for (i = 0; i < golden_count(f); i++)
       {
           r = golden_record_at(f, i);
           if (r->is_status)
               ...;                          // r->status is the name without the '!'
           else
               ...;                          // r->expected, r->expected_len
       }
       golden_close(f);

   The pointers of a record stay valid until golden_close. */

#ifndef ADF_TESTS_SUPPORT_GOLDEN_H
#define ADF_TESTS_SUPPORT_GOLDEN_H

#include <stddef.h>

/* Above the largest expansion in tests/golden/ (1048577 bytes) and below a size that a hostile
   line cannot reach with the count of digits it has. */

#define GOLDEN_DEFAULT_MAX_INPUT ((size_t) 1 << 26)

/* Long enough for NOT_DETERMINED, the longest status name of conventions.md 10.4. */

#define GOLDEN_STATUS_MAX 24

/* As in tests/support/jsonl.h: the name is a copy, so the error of a failed read stays
   readable after the reader has been closed. */

#define GOLDEN_FILE_MAX 256

typedef struct
{
    char file[GOLDEN_FILE_MAX];
    unsigned long line;
    unsigned long column;
    char message[200];
} golden_error_t;

typedef struct
{
    unsigned long line;  /* the line of the file, 1 for the first */
    char *input;         /* the decoded input: input_len bytes and a terminating NUL */
    size_t input_len;
    char *expected;      /* the expected field as it stands, with a terminating NUL */
    size_t expected_len;
    int is_status;       /* 1 if the expected field is `!NAME`, 0 if it is output text */
    char status[GOLDEN_STATUS_MAX];
    int generated;       /* 1 if the input came from an @gen: line */
} golden_record;

typedef struct golden_file golden_file;

int golden_open(const char *path, size_t max_input, golden_file **out, golden_error_t *err);

int golden_parse(const char *text, size_t len, const char *name, size_t max_input,
                 golden_file **out, golden_error_t *err);

void golden_close(golden_file *f);

size_t golden_count(const golden_file *f);
const golden_record *golden_record_at(const golden_file *f, size_t index);
const golden_error_t *golden_file_error(const golden_file *f);
const char *golden_error_message(const golden_error_t *err);

/* The comment lines and the empty lines are not records and are not counted. */

size_t golden_comment_lines(const golden_file *f);
size_t golden_blank_lines(const golden_file *f);

#endif /* ADF_TESTS_SUPPORT_GOLDEN_H */
