#ifndef __MVIM_MAIN_WINDOW_H__
#define __MVIM_MAIN_WINDOW_H__

#include <ncurses.h>

#include "window.h"

window* mainwindow_create (WINDOW* area, winpar* wp);

#endif
