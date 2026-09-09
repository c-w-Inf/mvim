#include "tchar.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

void utf8_to_tchar_cb (void (*callback) (void*, const tchar_t*), void* ctx, const char* src, size_t len) {
    mbstate_t mbs = {};

    tchar_t tc;
    size_t clus_id = 0;

#define flush_clus                                                                   \
    do {                                                                             \
        if (clus_id > 0) tc.clus[clus_id] = L'\0', callback (ctx, &tc), clus_id = 0; \
    } while (0)

    for (size_t i = 0; i < len;) {
        wchar_t wc;
        int conv_len = mbrtowc (&wc, src + i, len - i, &mbs);
        if (conv_len == -1 || conv_len == -2) {
            flush_clus;
            tc.clus[0] = L'?', tc.clus[1] = L'\0';
            tc.stat = -1;
            callback (ctx, &tc);

            memset (&mbs, 0, sizeof (mbs));
            ++i;
            continue;
        }

        if (conv_len == 0) conv_len = 1, wc = 0;

        i += conv_len;

        if (wc < 32 || wc == 127) {
            flush_clus;
            tc.clus[0] = L'^', tc.clus[1] = (wc ^ 0x40), tc.clus[2] = L'\0';
            tc.stat = -1;
            callback (ctx, &tc);
        } else {
            int width = wcwidth (wc);

            if (width > 0) {
                flush_clus;
                tc.clus[clus_id++] = wc;
                tc.stat = width;
            } else if (width == 0) {
                if (clus_id > 0) {
                    tc.clus[clus_id++] = wc;
                    if (clus_id == CLUSTER_MAX - 1) flush_clus;
                } else {
                    tc.clus[0] = L'?', tc.clus[1] = L'\0';
                    tc.stat = -conv_len;
                    callback (ctx, &tc);
                }
            } else {
                flush_clus;
                tc.clus[0] = L'?', tc.clus[1] = L'\0';
                tc.stat = -conv_len;
                callback (ctx, &tc);
            }
        }
    }

    flush_clus;
#undef flush_clus
}
size_t utf8_to_tchar (tchar_t* dest, const char* src, size_t len) {
    tchar_t* org_dest = dest;
    mbstate_t mbs = {};

    size_t clus_id = 0;

#define flush_clus                                                     \
    do {                                                               \
        if (clus_id > 0) (dest++)->clus[clus_id] = L'\0', clus_id = 0; \
    } while (0)

    for (size_t i = 0; i < len;) {
        wchar_t wc;
        int conv_len = mbrtowc (&wc, src + i, len - i, &mbs);
        if (conv_len == -1 || conv_len == -2) {
            flush_clus;
            dest->clus[0] = L'?', dest->clus[1] = L'\0';
            (dest++)->stat = -1;

            memset (&mbs, 0, sizeof (mbs));
            ++i;
            continue;
        }

        if (conv_len == 0) conv_len = 1, wc = 0;

        i += conv_len;

        if (wc < 32 || wc == 127) {
            flush_clus;
            dest->clus[0] = L'^', dest->clus[1] = (wc ^ 0x40), dest->clus[2] = L'\0';
            (dest++)->stat = -1;
        } else {
            int width = wcwidth (wc);

            if (width > 0) {
                flush_clus;
                dest->clus[clus_id++] = wc;
                dest->stat = width;
            } else if (width == 0) {
                if (clus_id > 0) {
                    dest->clus[clus_id++] = wc;
                    if (clus_id == CLUSTER_MAX - 1) flush_clus;
                } else {
                    dest->clus[0] = L'?', dest->clus[1] = L'\0';
                    (dest++)->stat = -conv_len;
                }
            } else {
                flush_clus;
                dest->clus[0] = L'?', dest->clus[1] = L'\0';
                (dest++)->stat = -conv_len;
            }
        }
    }

    flush_clus;
    dest->clus[0] = L'\0';
    dest->stat = 0;
    return dest - org_dest;
#undef flush_clus
}

size_t tchar_len (const tchar_t* ptc) {
    if (ptc->stat > 0) {
        return wcstombs (NULL, ptc->clus, 0);
    } else if (ptc->stat < 0) {
        return -ptc->stat;
    } else {
        return 0;
    }
}
size_t tchar_wid (const tchar_t* ptc) {
    if (ptc->stat > 0) {
        return ptc->stat;
    } else if (ptc->stat < 0) {
        return wcslen (ptc->clus);
    } else {
        return 0;
    }
}

size_t cast_pos (const tchar_t* ts, size_t screen_pos, size_t* tchar_pos, size_t* cursor_pos) {
    size_t buf = 0, scr = 0;
    const tchar_t* cur = ts;

    for (; cur->stat; ++cur) {
        size_t len = tchar_len (cur), wid = tchar_wid (cur);

        if (scr + wid > screen_pos) break;

        buf += len, scr += wid;
    }

    if (tchar_pos) *tchar_pos = cur - ts;
    if (cursor_pos) *cursor_pos = scr;
    return buf;
}

size_t get_pos (const tchar_t* ts, size_t buf_pos, size_t* tchar_pos) {
    size_t buf = 0, scr = 0;
    const tchar_t* cur = ts;

    for (; cur->stat; ++cur) {
        size_t len = tchar_len (cur), wid = tchar_wid (cur);

        if (buf + len > buf_pos) break;

        buf += len, scr += wid;
    }

    if (tchar_pos) *tchar_pos = cur - ts;
    return scr;
}
