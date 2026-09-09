#ifndef __MVIM_TCHAR_H__
#define __MVIM_TCHAR_H__

#include <wchar.h>

#define CLUSTER_MAX 16

typedef struct {
    wchar_t clus[CLUSTER_MAX];
    int stat;
    // INFO:
    // for normal clusters, stat = width of base unicode codepoint,
    // the length of char* is the length of clus;
    // for unknown/unprintable clusters, stat = - length of char*,
    // the width is the length of clus (in this case clus consists of ASCII printable characters);
    // for null clusters, stat = 0.
} tchar_t;

size_t tchar_len (const tchar_t* ptc);
size_t tchar_wid (const tchar_t* ptc);

size_t utf8_to_tchar (tchar_t* dest, const char* src, size_t len);
void utf8_to_tchar_cb (void (*callback) (void*, const tchar_t*), void* ctx, const char* src, size_t len);
size_t cast_pos (const tchar_t* ts, size_t screen_pos, size_t* tchar_pos, size_t* cursor_pos);
size_t get_pos (const tchar_t* ts, size_t buf_pos, size_t* tchar_pos);

#endif
