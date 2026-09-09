#include "mvim.h"

#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>

#include "main_window.h"
#include "window.h"

static void mvim_subw_end (window* wd);

void mvim_init (mvim* mv) {
    setlocale (LC_ALL, "");
    initscr ();
    raw ();
    noecho ();
    keypad (stdscr, TRUE);
    nodelay (stdscr, TRUE);
    set_escdelay (25);

    mv->ended = 0;

    mv->wp.par = mv;
    mv->wp.end = mvim_subw_end;

    mv->main = mainwindow_create (stdscr, &mv->wp);
}
void mvim_free (mvim* mv) {
    mv->main->window_free (mv->main->wd);

    endwin ();
}

void mvim_refresh (mvim* mv) {
    int stat;
    for (wint_t key;;) {
        stat = get_wch (&key);
        if (stat == ERR) break;

#ifdef DEBUG
        if (stat == OK) {
            if (iswprint (key)) {
                fprintf (stderr, "Printable(U+%04X) %lc\n", (unsigned)key, (wchar_t)key);
            } else if (key == L'\n' || key == L'\r') {
                fprintf (stderr, "Enter\n");
            } else {
                fprintf (stderr, "Control(%d)\n", (int)key);
            }
        } else if (stat == KEY_CODE_YES) {
            fprintf (stderr, "Special(%d) %s\n", (int)key, keyname (key));
        }
#endif

        if (stat == OK && key == ('q' & 0x1f)) mv->ended = 1;

        mv->main->window_type (mv->main->wd, key, (stat == OK ? 0 : 1));
    }

    mv->main->window_refresh (mv->main->wd);
    refresh ();
}

void mvim_subw_end (window* wd) {
    mvim* mv = wd->wp->par;
    mv->ended = 1;
    (void)wd;
}
