#include "buffer.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

static void linebuf_resize (linebuf* lb, size_t newcap) {
    lb->buf = realloc (lb->buf, newcap);
    lb->bufcap = newcap;
}

void linebuf_init (linebuf* lb) {
    lb->buf = NULL;
    lb->nbuf = lb->bufcap = 0;
}
void linebuf_nstrinit (linebuf* lb, const char* s, size_t slen) {
    lb->nbuf = slen, lb->bufcap = MVIM_BUFFER_UPSIZE (slen);
    lb->buf = malloc (lb->bufcap);
    memcpy (lb->buf, s, slen);
}
void linebuf_copyinit (linebuf* lb, const linebuf* s) {
    if (s->buf) {
        lb->nbuf = s->nbuf, lb->bufcap = MVIM_BUFFER_UPSIZE (s->nbuf);
        lb->buf = malloc (lb->bufcap);
        memcpy (lb->buf, s->buf, s->nbuf);
    } else {
        lb->buf = NULL;
        lb->nbuf = lb->bufcap = 0;
    }
}
void linebuf_free (linebuf* lb) { free (lb->buf); }

void linebuf_insert (linebuf* lb, size_t pos, const char* s) { linebuf_ninsert (lb, pos, s, strlen (s)); }
void linebuf_ninsert (linebuf* lb, size_t pos, const char* s, size_t slen) {
    if (lb->nbuf + slen > lb->bufcap) linebuf_resize (lb, MVIM_BUFFER_UPSIZE (lb->nbuf + slen));

    memmove (lb->buf + pos + slen, lb->buf + pos, lb->nbuf - pos);
    memcpy (lb->buf + pos, s, slen);
    lb->nbuf += slen;
}
void linebuf_erase (linebuf* lb, size_t pos, size_t len) {
    if (len) {
        memmove (lb->buf + pos, lb->buf + pos + len, lb->nbuf - pos - len);
        lb->nbuf -= len;
    }
}

static void buffer_resize (buffer* buf, size_t newcap) {
    buf->lines = realloc (buf->lines, sizeof (linebuf) * newcap);
    buf->linescap = newcap;
}

void buffer_init (buffer* buf) {
    buf->lines = malloc (sizeof (linebuf) * MVIM_BUFFER_SIZE);
    linebuf_init (&buf->lines[0]);
    buf->nlines = 1, buf->linescap = MVIM_BUFFER_SIZE;
}
void buffer_strinit (buffer* buf, const char* s) {
    buf->lines = malloc (sizeof (linebuf) * MVIM_BUFFER_SIZE);
    buf->nlines = 0, buf->linescap = MVIM_BUFFER_SIZE;

    for (const char *cur = s, *newline;; cur = newline + 1) {
        newline = strchr (cur, '\n');
        size_t len = newline ? (size_t)(newline - cur) : strlen (cur);

        linebuf_nstrinit (&buf->lines[buf->nlines++], cur, len);
        if (!newline) break;

        if (buf->nlines == buf->linescap) buffer_resize (buf, MVIM_BUFFER_UPSIZE (buf->nlines));
    }
}
void buffer_free (buffer* buf) {
    for (size_t i = 0; i < buf->nlines; ++i) linebuf_free (&buf->lines[i]);
    free (buf->lines);
}

void buffer_insert (buffer* buf, buffer_pos pos, const buffer* s) {
    if (s->nlines == 1) {
        linebuf_ninsert (&buf->lines[pos.row], pos.col, s->lines[0].buf, s->lines[0].nbuf);
    } else {
        if (buf->nlines + s->nlines - 1 > buf->linescap)
            buffer_resize (buf, MVIM_BUFFER_UPSIZE (buf->nlines + s->nlines));

        memmove (
            buf->lines + pos.row + s->nlines, buf->lines + pos.row + 1, sizeof (linebuf) * (buf->nlines - pos.row - 1));
        buf->nlines += s->nlines - 1;
        for (size_t i = 1; i < s->nlines; ++i) linebuf_copyinit (&buf->lines[pos.row + i], &s->lines[i]);

        linebuf* ins_head = &buf->lines[pos.row + s->nlines - 1];
        linebuf* ins_tail = &buf->lines[pos.row];

        linebuf_ninsert (ins_head, ins_head->nbuf, ins_tail->buf + pos.col, ins_tail->nbuf - pos.col);
        ins_tail->nbuf = pos.col;
        linebuf_ninsert (ins_tail, pos.col, s->lines[0].buf, s->lines[0].nbuf);
    }
}
void buffer_insert_str (buffer* buf, buffer_pos pos, const char* s) {
    linebuf_insert (&buf->lines[pos.row], pos.col, s);
}
void buffer_insert_nstr (buffer* buf, buffer_pos pos, const char* s, size_t slen) {
    linebuf_ninsert (&buf->lines[pos.row], pos.col, s, slen);
}
void buffer_split (buffer* buf, buffer_pos pos) {
    if (buf->nlines == buf->linescap) buffer_resize (buf, MVIM_BUFFER_UPSIZE (buf->nlines));

    memmove (buf->lines + pos.row + 2, buf->lines + pos.row + 1, sizeof (linebuf) * (buf->nlines - pos.row - 1));
    ++buf->nlines;

    linebuf* split = &buf->lines[pos.row];
    linebuf_nstrinit (split + 1, split->buf + pos.col, split->nbuf - pos.col);
    split->nbuf = pos.col;
}
void buffer_erase (buffer* buf, buffer_pos beg, buffer_pos end) {
    if (beg.row == end.row) {
        linebuf_erase (&buf->lines[beg.row], beg.col, end.col - beg.col);
    } else {
        buf->lines[beg.row].nbuf = beg.col;
        linebuf_ninsert (
            &buf->lines[beg.row], beg.col, buf->lines[end.row].buf + end.col, buf->lines[end.row].nbuf - end.col);

        for (size_t i = beg.row + 1; i <= end.row; ++i) linebuf_free (&buf->lines[i]);
        memmove (buf->lines + beg.row + 1, buf->lines + end.row + 1, sizeof (linebuf) * (buf->nlines - end.row - 1));
        buf->nlines -= end.row - beg.row;
    }
}

void buffer_write (buffer* buf, const char* file) {
    FILE* f = fopen (file, "w");
    fwrite (buf->lines->buf, 1, buf->lines->nbuf, f);
    for (size_t i = 1; i < buf->nlines; ++i) {
        fwrite ("\n", 1, 1, f);
        fwrite (buf->lines[i].buf, 1, buf->lines[i].nbuf, f);
    }
    fwrite ("\n", 1, 1, f);
    fclose (f);
}
