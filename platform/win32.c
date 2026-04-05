/* win32.c — Win32 Console platform layer (reserved, not yet implemented) */

#include "../src/term_internal.h"

int  term_platform_init(void)              { return -1; }
void term_platform_cleanup(void)           {}
void  term_platform_get_size(int *w, int *h)  { *w = 80; *h = 24; }
float term_platform_cell_aspect(void)          { return 2.0f; }
int  term_platform_read(char *buf, int buf_size, int timeout_ms) {
    (void)buf; (void)buf_size; (void)timeout_ms;
    return -1;
}
