#include <unistd.h>

#include "mvim.h"

int main () {
    mvim mv;

    mvim_init (&mv);

    while (!mv.ended) {
        mvim_refresh (&mv);

        usleep (10000);
    }

    mvim_free (&mv);

    return 0;
}
