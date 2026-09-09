#ifndef __MVIM_MVIM_H__
#define __MVIM_MVIM_H__

#include "window.h"

typedef struct {
    window* main;
    winpar wp;

    int ended;
} mvim;

void mvim_init (mvim* mv);
void mvim_free (mvim* mv);

void mvim_refresh (mvim* mv);

#endif
