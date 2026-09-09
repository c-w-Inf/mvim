#ifndef __MVIM_BUFFER_WINDOW_H__
#define __MVIM_BUFFER_WINDOW_H__

#include <ncurses.h>

#include "window.h"

window* bufferwindow_create (WINDOW* area, winpar* wp);

#endif
