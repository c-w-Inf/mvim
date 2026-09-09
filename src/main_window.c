#include "main_window.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <wctype.h>

#include "buffer.h"
#include "buffer_window.h"
#include "tchar.h"
#include "window.h"

typedef struct {
    window wd;

    WINDOW* focus_area;
    winpar focus_wp;
    window* focus;

    int iscommand;
    linebuf com;
    size_t com_pos;
} mainwindow;

static void mainwindow_type (window* wd, unsigned key, int isspec);
static void mainwindow_refresh (window* wd);
static int mainwindow_command (window* wd, const char* com);
static void mainwindow_free (window* wd);

static void mainwindow_subw_end (window* wd);

window* mainwindow_create (WINDOW* area, winpar* wp) {
    mainwindow* mw = malloc (sizeof (mainwindow));
    mw->wd.wd = mw;
    mw->wd.wp = wp;
    mw->wd.area = area;
    mw->wd.window_type = mainwindow_type, mw->wd.window_refresh = mainwindow_refresh,
    mw->wd.window_command = mainwindow_command, mw->wd.window_free = mainwindow_free;

    mw->wd.state = MVIM_WINDOW_STATE_NULL;

    mw->focus_wp.par = mw;
    mw->focus_wp.end = mainwindow_subw_end;

    mw->focus_area = derwin (area, getmaxy (area) - 1, getmaxx (area), 0, 0);
    mw->focus = bufferwindow_create (mw->focus_area, &mw->focus_wp);

    mw->iscommand = 0;
    linebuf_init (&mw->com);
    mw->com_pos = 0;

    return &mw->wd;
}
void mainwindow_free (window* wd) {
    mainwindow* mw = wd->wd;

    mw->focus->window_free (mw->focus);
    delwin (mw->focus_area);

    free (mw);
}

static int mainwindow_command (window* wd, const char* com) {
    mainwindow* mw = wd->wd;

    int n = 0;

    if ((sscanf (com, " q! %n", &n), n > 0) || (sscanf (com, " quit! %n", &n), n > 0)) {
        mw->wd.wp->end (&mw->wd);
        return 0;
    } else if (sscanf (com, " wq %n", &n), n > 0) {
        int ret = mw->focus->window_command (mw->focus, com);
        if (ret) return ret;
        mw->wd.wp->end (&mw->wd);
        return 0;
    } else {
        return 492;
    }
}

void mainwindow_type (window* wd, unsigned key, int isspec) {
    mainwindow* mw = wd->wd;

    if (mw->iscommand) {
        if (iswprint (key) && !isspec) {
            mbstate_t mbs = {};
            char bytes[MB_CUR_MAX];
            size_t len = wcrtomb (bytes, (wchar_t)key, &mbs);

            linebuf_ninsert (&mw->com, mw->com_pos, bytes, len);
            mw->com_pos += len;
        } else if ((key == '\n' || key == '\r') && !isspec) {
            linebuf_ninsert (&mw->com, mw->com.nbuf, "", 1);
            mainwindow_command (wd, mw->com.buf);
            mw->com.nbuf = 0, mw->com_pos = 0, mw->iscommand = 0;
        } else if (key == 27 && !isspec) {
            mw->com.nbuf = 0, mw->com_pos = 0, mw->iscommand = 0;
        }
    } else if (mw->focus->state == MVIM_WINDOW_STATE_NORMAL && key == L':' && !isspec) {
        mw->iscommand = 1;
    } else {
        mw->focus->window_type (mw->focus, key, isspec);
    }
}

void mainwindow_refresh (window* wd) {
    mainwindow* mw = wd->wd;
    WINDOW* area = wd->area;

    if (!mw->iscommand) {
        mvwprintw (
            area, getmaxy (area) - 1, 0, "%s", (mw->focus->state == MVIM_WINDOW_STATE_INSERT ? "-- INSERT --" : ""));
        wclrtoeol (area);
    }

    mw->focus->window_refresh (mw->focus);

    if (mw->iscommand) {
        tchar_t* tbuf = malloc (sizeof (tchar_t) * (mw->com.nbuf + 1));
        utf8_to_tchar (tbuf, mw->com.buf, mw->com.nbuf);
        int cur_pos = get_pos (tbuf, mw->com_pos, NULL) + 1;

        mvwprintw (area, getmaxy (area) - 1, 0, ":");
        for (tchar_t* tc = tbuf; tc->stat; ++tc) wprintw (area, "%ls", tc->clus);
        wmove (area, getmaxy (area) - 1, cur_pos);
    }

    wsyncup (mw->wd.area);
}

void mainwindow_subw_end (window* wd) {
    mainwindow* mw = wd->wp->par;

    mw->wd.wp->end (&mw->wd);
}
