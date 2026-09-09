#ifndef __MVIM_WINDOW_H__
#define __MVIM_WINDOW_H__

#include <ncurses.h>

struct winpar;
typedef struct winpar winpar;
struct window;
typedef struct window window;

struct window {
    void* wd;
    winpar* wp;

    WINDOW* area;

    void (*window_type) (window* wd, unsigned key, int isspec);
    void (*window_refresh) (window* wd);
    int (*window_command) (window* wd, const char* com);
    void (*window_free) (window* wd);

    int state;
};

#define MVIM_WINDOW_STATE_NULL 0
#define MVIM_WINDOW_STATE_NORMAL 1
#define MVIM_WINDOW_STATE_INSERT 2

struct winpar {
    void* par;
    void (*end) (window* wd);
};

#endif
