#include "buffer_window.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include "buffer.h"
#include "tchar.h"
#include "window.h"

typedef struct {
    window wd;

    int iswelcome;

    buffer buf;
    tchar_t* tchar_buf;
    size_t tchar_bufsz, tchar_buflen;

    buffer_pos buf_pos;
    // INFO: pos on buf
    size_t screen_x;
    // INFO: on which col of the screen
    size_t tchar_x;
    // INFO: on which tchar
    int cursor_row;
    // INFO: on which row of the window
} bufferwindow;

static void load_tchar (bufferwindow* bw, linebuf* lb) {
    if (bw->tchar_bufsz <= lb->nbuf) {
        bw->tchar_bufsz = MVIM_BUFFER_UPSIZE (lb->nbuf);
        free (bw->tchar_buf);
        bw->tchar_buf = malloc (sizeof (tchar_t) * bw->tchar_bufsz);
    }
    bw->tchar_buflen = utf8_to_tchar (bw->tchar_buf, lb->buf, lb->nbuf);
}

static void bufferwindow_type (window* wd, unsigned key, int isspec);
static void bufferwindow_refresh (window* wd);
static int bufferwindow_command (window* wd, const char* com);
static void bufferwindow_free (window* wd);

window* bufferwindow_create (WINDOW* area, winpar* wp) {
    bufferwindow* bw = malloc (sizeof (bufferwindow));
    bw->wd.wd = bw;
    bw->wd.wp = wp;
    bw->wd.area = area;
    bw->wd.window_type = bufferwindow_type, bw->wd.window_refresh = bufferwindow_refresh,
    bw->wd.window_command = bufferwindow_command, bw->wd.window_free = bufferwindow_free;

    bw->wd.state = MVIM_WINDOW_STATE_NORMAL;

    bw->iswelcome = 1;

    buffer_init (&bw->buf);
    bw->tchar_bufsz = 0;
    bw->tchar_buf = NULL;
    load_tchar (bw, bw->buf.lines);

    bw->buf_pos = (buffer_pos){0, 0};
    bw->screen_x = 0;
    bw->tchar_x = 0;
    bw->cursor_row = 0;

    return &bw->wd;
}
void bufferwindow_free (window* wd) {
    bufferwindow* bw = wd->wd;

    buffer_free (&bw->buf);
    free (bw->tchar_buf);

    free (bw);
}

static void cursor_cast (bufferwindow* bw) {
    bw->screen_x = 0;
    bw->buf_pos.col = 0;
    for (size_t i = 0; i < bw->tchar_x; ++i) {
        bw->screen_x += tchar_wid (bw->tchar_buf + i);
        bw->buf_pos.col += tchar_len (bw->tchar_buf + i);
    }
}

void bufferwindow_type (window* wd, unsigned key, int isspec) {
    bufferwindow* bw = wd->wd;
    WINDOW* area = wd->area;

    linebuf* curline = &bw->buf.lines[bw->buf_pos.row];

#define keyprintable (iswprint (key) && !isspec)
#define keyis(x) (key == (x) && !isspec)
#define keyisspec(x) (key == (x) && isspec)
#define KEY_CTRL(x) ((x) & 0x1f)
#define KEY_ESC 27

    if (bw->wd.state == MVIM_WINDOW_STATE_NORMAL) {
        if (keyis (KEY_ESC)) {
        } else if (keyis ('a')) {
            if (bw->tchar_buf->stat) ++bw->tchar_x;
            cursor_cast (bw);
            bw->wd.state = MVIM_WINDOW_STATE_INSERT;
        } else if (keyis ('h')) {
            if (bw->tchar_x > 0) {
                --bw->tchar_x;
                cursor_cast (bw);
            }
        } else if (keyis ('i')) {
            cursor_cast (bw);
            bw->wd.state = MVIM_WINDOW_STATE_INSERT;
        } else if (keyis ('j')) {
            if (bw->buf_pos.row < bw->buf.nlines - 1) {
                ++bw->buf_pos.row;
                if (bw->cursor_row < getmaxy (area) - 1) ++bw->cursor_row;

                load_tchar (bw, &bw->buf.lines[bw->buf_pos.row]);
                bw->buf_pos.col = cast_pos (bw->tchar_buf, bw->screen_x, &bw->tchar_x, NULL);
                if (bw->tchar_x == bw->tchar_buflen && bw->tchar_buflen > 0) {
                    --bw->tchar_x;
                    bw->buf_pos.col -= tchar_len (bw->tchar_buf + bw->tchar_x);
                }
            }
        } else if (keyis ('k')) {
            if (bw->buf_pos.row > 0) {
                --bw->buf_pos.row;
                if (bw->cursor_row > 0) --bw->cursor_row;

                load_tchar (bw, &bw->buf.lines[bw->buf_pos.row]);
                bw->buf_pos.col = cast_pos (bw->tchar_buf, bw->screen_x, &bw->tchar_x, NULL);
                if (bw->tchar_x == bw->tchar_buflen && bw->tchar_buflen > 0) {
                    --bw->tchar_x;
                    bw->buf_pos.col -= tchar_len (bw->tchar_buf + bw->tchar_x);
                }
            }
        } else if (keyis ('l')) {
            if (bw->tchar_x < bw->tchar_buflen - 1) {
                ++bw->tchar_x;
                cursor_cast (bw);
            }
        } else if (keyis ('0')) {
            bw->buf_pos.col = 0;
            bw->tchar_x = 0;
            bw->screen_x = 0;
        } else if (keyis ('$')) {
            bw->tchar_x = (bw->tchar_buflen > 0 ? bw->tchar_buflen - 1 : 0);
            cursor_cast (bw);
            bw->screen_x = (size_t)-1;
        }

    } else if (bw->wd.state == MVIM_WINDOW_STATE_INSERT) {
        if (keyprintable) {
            bw->iswelcome = 0;
            cursor_cast (bw);

            mbstate_t mbs = {};
            char bytes[MB_CUR_MAX];
            size_t len = wcrtomb (bytes, (wchar_t)key, &mbs);

            linebuf_ninsert (curline, bw->buf_pos.col, bytes, len);
            bw->buf_pos.col += len;

            load_tchar (bw, curline);
            bw->screen_x = get_pos (bw->tchar_buf, bw->buf_pos.col, &bw->tchar_x);
        } else if (keyis ('\n') || keyis ('\r')) {
            bw->iswelcome = 0;

            buffer_split (&bw->buf, bw->buf_pos);
            ++bw->buf_pos.row, bw->buf_pos.col = 0, bw->tchar_x = 0, bw->screen_x = 0;
            if (bw->cursor_row < getmaxy (area) - 1) ++bw->cursor_row;
        } else if (keyis (KEY_ESC)) {
            if (bw->tchar_x > 0) --bw->tchar_x;
            cursor_cast (bw);
            bw->wd.state = MVIM_WINDOW_STATE_NORMAL;
        } else if (keyisspec (KEY_BACKSPACE) || keyis (KEY_CTRL ('h'))) {
            if (bw->tchar_x > 0) {
                --bw->tchar_x;
                size_t xlen = tchar_len (bw->tchar_buf + bw->tchar_x);
                bw->buf_pos.col -= xlen;
                linebuf_erase (curline, bw->buf_pos.col, xlen);
                bw->screen_x -= tchar_wid (bw->tchar_buf + bw->tchar_x);
                memmove (
                    bw->tchar_buf + bw->tchar_x, bw->tchar_buf + bw->tchar_x + 1,
                    sizeof (tchar_t) * (bw->tchar_buflen - bw->tchar_x));
            } else if (bw->buf_pos.row > 0) {
                --bw->buf_pos.row;
                linebuf* newcurline = &bw->buf.lines[bw->buf_pos.row];
                bw->buf_pos.col = newcurline->nbuf;
                buffer_erase (&bw->buf, bw->buf_pos, (buffer_pos){bw->buf_pos.row + 1, 0});
                load_tchar (bw, newcurline);
                bw->screen_x = get_pos (bw->tchar_buf, bw->buf_pos.col, &bw->tchar_x);

                if (bw->cursor_row > 0) --bw->cursor_row;
            }
        }
    }
}

static void print_tchar (void* ctx, const tchar_t* ptc) { wprintw ((WINDOW*)ctx, "%ls", ptc->clus); }

void bufferwindow_refresh (window* wd) {
    bufferwindow* bw = wd->wd;
    WINDOW* area = wd->area;

    wclear (area);

    size_t row_bias = bw->buf_pos.row - bw->cursor_row;
    for (int i = 0; i < getmaxy (area) && row_bias + i < bw->buf.nlines; ++i) {
        linebuf* lb = &bw->buf.lines[row_bias + i];

        wmove (area, i, 0);
        utf8_to_tchar_cb (print_tchar, area, lb->buf, lb->nbuf);
    }

    if (bw->iswelcome) {
        const char* hello = "Hello, MiniVIM!";
        mvwprintw (area, (getmaxy (area) - 1) / 2, (getmaxx (area) - strlen (hello)) / 2, "%s", hello);
        wmove (area, 0, 0);
    }

    int cursor_col = 0;
    for (size_t i = 0; i < bw->tchar_x; ++i) {
        cursor_col += tchar_wid (bw->tchar_buf + i);
    }
    wmove (area, bw->cursor_row, cursor_col);
    wsyncup (area);
    wcursyncup (area);
}

int bufferwindow_command (window* wd, const char* com) {
    bufferwindow* bw = wd->wd;

    int n = 0;
    char* buf = malloc (strlen (com) + 1);
    int ret = 0;
    if (sscanf (com, " wq%*[ \t]%[^\n]%n", buf, &n), n > 0) {
        buffer_write (&bw->buf, buf);
    } else {
        ret = 492;
    }

    free (buf);
    return ret;
}
