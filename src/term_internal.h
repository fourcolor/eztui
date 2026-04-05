#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "eztui.h"

/* ---- write buffer ---- */
void term_buf_flush(void);
void term_buf_write(const char *data, int len);
void term_buf_puts(const char *s);

/* ---- cursor ---- */
void term_cursor_move(int x, int y);
void term_cursor_hide(void);
void term_cursor_show(void);

/* ---- screen ---- */
void term_screen_clear(void);
void term_screen_alt_enter(void);
void term_screen_alt_leave(void);

/* ---- mouse ---- */
void term_mouse_enable(void);
void term_mouse_disable(void);

/* ---- SGR ---- */
void term_sgr_reset(void);
void term_sgr_attr(int attr);
void term_sgr_fg(uint32_t color);
void term_sgr_bg(uint32_t color);

/* ---- platform (posix.c / win32.c) ---- */
int   term_platform_init(void);
void  term_platform_cleanup(void);
void  term_platform_get_size(int *w, int *h);
float term_platform_cell_aspect(void);
/* Read raw bytes from stdin; timeout_ms: 0=nonblocking, -1=forever.
 * Returns number of bytes read, 0 on timeout, -1 on error. */
int  term_platform_read(char *buf, int buf_size, int timeout_ms);
