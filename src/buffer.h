#ifndef __MVIM_BUFFER_H__
#define __MVIM_BUFFER_H__

#include <stddef.h>

#define MVIM_BUFFER_SIZE 64
#define MVIM_BUFFER_UPSIZE(x) (((x) / MVIM_BUFFER_SIZE + 2) * MVIM_BUFFER_SIZE)

// line buffer

typedef struct {
    char* buf;
    size_t nbuf, bufcap;
} linebuf;

void linebuf_init (linebuf* lb);
void linebuf_nstrinit (linebuf* lb, const char* s, size_t slen);
void linebuf_copyinit (linebuf* lb, const linebuf* s);
void linebuf_free (linebuf* lb);

void linebuf_insert (linebuf* lb, size_t pos, const char* s);
void linebuf_ninsert (linebuf* lb, size_t pos, const char* s, size_t slen);
void linebuf_erase (linebuf* lb, size_t pos, size_t len);

// buffer

typedef struct {
    linebuf* lines;
    size_t nlines, linescap;
} buffer;

void buffer_init (buffer* buf);
void buffer_strinit (buffer* buf, const char* s);
void buffer_free (buffer* buf);

typedef struct {
    size_t row, col;
} buffer_pos;

void buffer_insert (buffer* buf, buffer_pos pos, const buffer* s);
void buffer_insert_str (buffer* buf, buffer_pos pos, const char* s);
void buffer_insert_nstr (buffer* buf, buffer_pos pos, const char* s, size_t slen);
void buffer_split (buffer* buf, buffer_pos pos);
void buffer_erase (buffer* buf, buffer_pos beg, buffer_pos end);

void buffer_write (buffer* buf, const char* file);

#endif
